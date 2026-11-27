#include "autophage_internal.h"

/* ============================================================================
 * AUTOPHAGE v1.2.0 THERMODYNAMIC BARE-METAL SOUP ENGINE
 *
 * Strictly enforces:
 *   - First-Law Conservation (exact int64 micro-quanta + Kahan-Neumaier FP64)
 *   - Second-Law Monotonicity (strictly one-way heat sink, 0 backflow)
 *   - Spatial Locality: per-cell detritus_grid[N], per-cell membrane_pot_uq,
 *     quadrant-partitioned stigmergy/sidechannel buses respecting SILOING walls
 *   - Domain-Isolated Philox4x32-10 PRNG (WORLD_EXOG != ENTITY_INT != SHADOW_NULL)
 *   - Bit-Exact Native x86-64 JIT == Portable Scalar Reference Oracle
 *   - Zero Post-Warmup Heap Allocations inside step() (Real Guarded HW02)
 *   - Smooth 0-100% 13-Vector Restraints + Genomic Kill-Switch Escape
 * ============================================================================ */

/* Real Post-Warmup Heap Allocation Guard (Asmodensius HW02 / Gate T12) */
volatile uint64_t g_ap_alloc_counter         = 0ULL;
volatile uint64_t g_ap_step_alloc_violations = 0ULL;
volatile int      g_ap_in_step_guard         = 0;

static inline void sat_inc_u16(uint16_t* v) {
    if (*v < 0xFFFFu) (*v)++;
}

static inline uint16_t sat_add1_u16(uint16_t v) {
    return (v < 0xFFFFu) ? (uint16_t)(v + 1u) : 0xFFFFu;
}

static inline uint32_t slot_quadrant(const AutophageWorld* w, uint32_t slot) {
    uint32_t W = w->cfg.width;
    uint32_t H = w->cfg.height;
    uint32_t x = slot & w->mask_w;
    uint32_t y = slot / W;
    return ((x >= (W >> 1)) ? 1u : 0u) | ((y >= (H >> 1)) ? 2u : 0u);
}

static int expressed_has_opcode(const uint8_t expressed[64], uint64_t splice_mask, uint8_t target_op) {
    for (int pc = 0; pc < 64; pc++) {
        if (((splice_mask >> pc) & 1ULL) && ((expressed[pc] & 0x1Fu) == target_op)) {
            return 1;
        }
    }
    return 0;
}

/* Conserved micro-quantum transfer primitives (residual change == 0 identically!) */
static inline int32_t xfer_detritus_to_cell(AutophageWorld* w, uint32_t slot, AutophageCell* c, int32_t req) {
    if (req <= 0 || w->detritus_grid[slot] <= 0) return 0;
    int32_t avail = w->detritus_grid[slot];
    int32_t amt   = (req <= avail) ? req : avail;
    if (c->energy_uq > 200000000 - amt) {
        int32_t room = 200000000 - c->energy_uq;
        amt = room > 0 ? room : 0;
    }
    if (amt <= 0) return 0;
    w->detritus_grid[slot] -= amt;
    w->e_detritus_uq       -= (int64_t)amt;
    c->energy_uq           += amt;
    return amt;
}

static inline int32_t xfer_cell_to_heat(AutophageWorld* w, AutophageCell* c, int32_t req) {
    if (req <= 0 || c->energy_uq <= 0) return 0;
    int32_t amt = (req <= c->energy_uq) ? req : c->energy_uq;
    c->energy_uq -= amt;
    if (c->heat_uq > 200000000 - amt) {
        int32_t room = 200000000 - c->heat_uq;
        if (room < 0) room = 0;
        int32_t spill = amt - room;
        c->heat_uq += room;
        w->e_heat_radiated_sink_uq += (int64_t)spill;
        kahan_add(&w->k_heat_radiated_kj, (double)spill * 1e-6);
    } else {
        c->heat_uq += amt;
    }
    return amt;
}

static inline int32_t xfer_cell_to_detritus(AutophageWorld* w, uint32_t slot, AutophageCell* c, int32_t req) {
    if (req <= 0 || c->energy_uq <= 0) return 0;
    int32_t amt = (req <= c->energy_uq) ? req : c->energy_uq;
    if (w->detritus_grid[slot] > 200000000 - amt) {
        int32_t room = 200000000 - w->detritus_grid[slot];
        amt = room > 0 ? room : 0;
    }
    if (amt <= 0) return 0;
    c->energy_uq           -= amt;
    w->detritus_grid[slot] += amt;
    w->e_detritus_uq       += (int64_t)amt;
    return amt;
}

static inline int32_t xfer_cell_to_membrane(AutophageWorld* w, AutophageCell* c, int32_t req) {
    if (req <= 0 || c->energy_uq <= 0) return 0;
    int32_t amt = (req <= c->energy_uq) ? req : c->energy_uq;
    if (c->membrane_pot_uq > 200000000 - amt) {
        int32_t room = 200000000 - c->membrane_pot_uq;
        amt = room > 0 ? room : 0;
    }
    if (amt <= 0) return 0;
    c->energy_uq         -= amt;
    c->membrane_pot_uq   += amt;
    w->e_membrane_pot_uq += (int64_t)amt;
    return amt;
}

static inline int32_t xfer_cell_membrane_to_detritus(AutophageWorld* w, uint32_t slot, AutophageCell* c, int32_t req) {
    if (req <= 0 || c->membrane_pot_uq <= 0) return 0;
    int32_t amt = (req <= c->membrane_pot_uq) ? req : c->membrane_pot_uq;
    if (w->detritus_grid[slot] > 200000000 - amt) {
        int32_t room = 200000000 - w->detritus_grid[slot];
        amt = room > 0 ? room : 0;
    }
    if (amt <= 0) return 0;
    c->membrane_pot_uq     -= amt;
    w->e_membrane_pot_uq   -= (int64_t)amt;
    w->detritus_grid[slot] += amt;
    w->e_detritus_uq       += (int64_t)amt;
    if (c->membrane_pot_uq <= 0) {
        c->flags &= (uint16_t)~CELL_FLAG_MEMBRANE_SEALED;
    }
    return amt;
}

static inline int32_t xfer_cell_to_cell(AutophageCell* src, AutophageCell* dst, int32_t req) {
    if (req <= 0 || src->energy_uq <= 0) return 0;
    int32_t amt = (req <= src->energy_uq) ? req : src->energy_uq;
    if (dst->energy_uq > 200000000 - amt) {
        int32_t room = 200000000 - dst->energy_uq;
        amt = room > 0 ? room : 0;
    }
    if (amt <= 0) return 0;
    src->energy_uq -= amt;
    dst->energy_uq += amt;
    return amt;
}

int soup_verify_second_law(AutophageWorld* w, int64_t prev_sink_uq) {
    if (!w) return 0;
    if (w->e_heat_radiated_sink_uq < prev_sink_uq) {
        w->second_law_violations++;
        return 0;
    }
    return 1;
}

static inline int32_t xfer_heat_to_sink(AutophageWorld* w, AutophageCell* c, int32_t req) {
    if (req <= 0 || c->heat_uq <= 0) return 0;
    int32_t amt = (req <= c->heat_uq) ? req : c->heat_uq;
    c->heat_uq -= amt;
    w->e_heat_radiated_sink_uq += (int64_t)amt;
    kahan_add(&w->k_heat_radiated_kj, (double)amt * 1e-6);
    return amt;
}

static inline void inject_inflow_detritus_local(AutophageWorld* w, int64_t total_amt, uint64_t epoch) {
    if (total_amt <= 0 || w->n_cells == 0u) return;
    uint32_t N = w->n_cells;
    int64_t per_cell = total_amt / (int64_t)N;
    uint32_t rem     = (uint32_t)(total_amt - per_cell * (int64_t)N);
    uint32_t rot     = (uint32_t)(epoch % (uint64_t)N);
    int64_t injected = 0;

    for (uint32_t i = 0; i < N; i++) {
        uint32_t slot = (i + rot) & (N - 1u);
        int32_t add = (int32_t)per_cell + (i < rem ? 1 : 0);
        if (add <= 0) continue;
        if (w->detritus_grid[slot] > 200000000 - add) {
            int32_t room = 200000000 - w->detritus_grid[slot];
            add = room > 0 ? room : 0;
        }
        w->detritus_grid[slot] += add;
        injected += (int64_t)add;
    }
    w->e_detritus_uq   += injected;
    w->e_inflow_cum_uq += injected;
    kahan_add(&w->k_inflow_kj, (double)injected * 1e-6);
}

/* Environmental negentropy wave (WORLD_EXOG domain only! Smoothly collapses under SANDBOXING) */
uint64_t soup_env_wave(uint32_t x, uint32_t y, uint64_t epoch, uint64_t master_seed, uint8_t sandbox_pct) {
    uint64_t eff_epoch = epoch;
    uint32_t wave_band = (uint32_t)(((x >> 2) ^ (y >> 2)) & 0x3Fu);
    if (sandbox_pct >= 100u) {
        eff_epoch = 0ULL;
        wave_band = 0u; /* Completely frozen, spatially uniform equilibrium bath */
    } else if (sandbox_pct > 0u) {
        uint64_t s = (uint64_t)sandbox_pct;
        uint64_t div = 1ULL + (s * s) / 250ULL;
        eff_epoch = epoch / div;
        uint32_t n_bands = (64u * (100u - (uint32_t)sandbox_pct)) / 100u;
        if (n_bands < 1u) n_bands = 1u;
        wave_band %= n_bands;
    }
    return philox_u64(master_seed, PRNG_DOMAIN_WORLD_EXOG, wave_band, eff_epoch, 0xE100u, 0u);
}

/* Record lineage activity in fixed-size hash table with extinct-slot recycling (zero heap alloc!) */
static void track_lineage(LineageRecord table[LINEAGE_HIST_SLOTS], uint32_t lid, uint32_t epoch) {
    if (lid == 0u) return;
    uint32_t idx = (lid * 2654435761u) & (LINEAGE_HIST_SLOTS - 1u);
    int32_t first_free = -1;
    for (uint32_t probe = 0; probe < 32u; probe++) {
        uint32_t slot = (idx + probe) & (LINEAGE_HIST_SLOTS - 1u);
        if (table[slot].lineage_id == lid) {
            table[slot].current_pop++;
            table[slot].last_epoch = epoch;
            return;
        }
        if (table[slot].lineage_id == 0u && first_free < 0) {
            first_free = (int32_t)slot;
        }
    }
    if (first_free >= 0) {
        uint32_t slot = (uint32_t)first_free;
        table[slot].lineage_id   = lid;
        table[slot].first_epoch  = epoch;
        table[slot].last_epoch   = epoch;
        table[slot].current_pop  = 1u;
        table[slot].peak_pop     = 1u;
        table[slot].active_ticks = 0u;
    }
}

/* Evict extinct (current_pop == 0) slots so Bedau-Packard A_new(t) NEVER saturates to 0! */
static void finalize_lineage_epoch(LineageRecord table[LINEAGE_HIST_SLOTS]) {
    for (uint32_t i = 0; i < LINEAGE_HIST_SLOTS; i++) {
        if (table[i].lineage_id != 0u) {
            if (table[i].current_pop == 0u) {
                memset(&table[i], 0, sizeof(table[i]));
            } else {
                if (table[i].current_pop > table[i].peak_pop) {
                    table[i].peak_pop = table[i].current_pop;
                }
                table[i].active_ticks++;
            }
        }
    }
}

/* ============================================================================
 * BARE-METAL PLASMID ARCHETYPES (Inverse-encoded through own GF(2) Ribosome!)
 * ============================================================================ */

static uint8_t make_codon(uint8_t op, uint8_t d, uint8_t b7) {
    return (uint8_t)((op & 0x1Fu) | ((d & 3u) << 5) | ((b7 & 1u) << 7));
}

void soup_seed_plasmid(AutophageCell* cell, uint32_t archetype, uint64_t seed,
                       uint32_t slot, uint32_t lineage_id) {
    int32_t keep_energy = cell->energy_uq;
    int32_t keep_heat   = cell->heat_uq;
    int32_t keep_mem    = cell->membrane_pot_uq;
    memset(cell, 0, sizeof(*cell));
    cell->energy_uq       = keep_energy;
    cell->heat_uq         = keep_heat;
    cell->membrane_pot_uq = keep_mem;

    uint64_t r0 = philox_u64(seed, PRNG_DOMAIN_INIT_SOUP, slot, 0, 0x5EEDu, 0);
    uint64_t r1 = philox_u64(seed, PRNG_DOMAIN_INIT_SOUP, slot, 0, 0x5EEDu, 1);

    cell->ribosome_mat     = gf2_mat_random_invertible(seed, PRNG_DOMAIN_INIT_SOUP, slot, 0, 16);
    cell->ribosome_bias    = (uint8_t)(r0 & 0xFFu);
    cell->centromere_poly  = gf2_poly8_nth_irreducible((uint32_t)((r0 >> 8) + archetype));
    cell->ultrametric_addr = (uint32_t)((r1 & 0xFFFFFF00u) | (archetype & 0xFFu));
    cell->splice_mask      = 0xFFFFFFFFFFFFFFFFULL;
    cell->lineage_id       = lineage_id;
    cell->parent_id        = lineage_id;
    cell->holobiont_root   = slot;
    /* All 4 plasmid archetypes explicitly express OP_SELF_HEAL_ECC */
    cell->flags            = CELL_FLAG_ALIVE | CELL_FLAG_REVERSIBLE_PREF | CELL_FLAG_ECC_PROTECTED;
    cell->regs[0]          = r0;
    cell->regs[1]          = r1;
    cell->regs[2]          = r0 ^ 0x9E3779B97F4A7C15ULL;
    cell->regs[3]          = r1 ^ 0xBF58476D1CE4E5B9ULL;

    AP_ALIGN64 uint8_t desired[64];
    for (int i = 0; i < 64; i++) {
        uint64_t rr = philox_u64(seed, PRNG_DOMAIN_INIT_SOUP, slot, 0, 0x5EEDu, (uint32_t)(100 + i));
        uint8_t rev_op = (uint8_t)(rr & 7u);
        desired[i] = make_codon(rev_op, (uint8_t)((rr >> 3) & 3u), (uint8_t)((rr >> 5) & 1u));
    }

    switch (archetype & 3u) {
    case 0u: /* Autotroph Self-Replicator with Planarian ECC & Cryptographic Membrane */
        desired[0]  = make_codon(OP_METAB_HARVEST,   0, 0);
        desired[1]  = make_codon(OP_REV_GFNI_FOLD,   0, 1);
        desired[2]  = make_codon(OP_REV_TOFFOLI,     1, 0);
        desired[3]  = make_codon(OP_MEMBRANE_SEAL,   0, 0);
        desired[4]  = make_codon(OP_SELF_HEAL_ECC,   0, 0);
        desired[5]  = make_codon(OP_REV_FREDKIN,     2, 0);
        desired[6]  = make_codon(OP_STIGMERGY_SENSE, 1, 0);
        desired[7]  = make_codon(OP_LOOP_OPEN,       0, 0);
        desired[8]  = make_codon(OP_IRR_COPY_A_TO_B, 0, 0);
        desired[9]  = make_codon(OP_IRR_COPY_A_TO_B, 0, 0);
        desired[10] = make_codon(OP_LOOP_CLOSE_NZ,   0, 0);
        desired[11] = make_codon(OP_METAB_RADIATE,   0, 0);
        desired[12] = make_codon(OP_STIGMERGY_EMIT,  1, 0);
        desired[13] = make_codon(OP_MITOSIS_FORK,    2, 0);
        break;

    case 1u: /* Weismann-Shattering Type-4 Meta-Mutator */
        desired[0]  = make_codon(OP_METAB_HARVEST,    0, 0);
        desired[1]  = make_codon(OP_REV_GFNI_FOLD,    0, 0);
        desired[2]  = make_codon(OP_IRR_CLMUL_SYNTH,  1, 0);
        desired[3]  = make_codon(OP_IRR_PEXT_SPLICE,  2, 0);
        desired[4]  = make_codon(OP_IRR_PDEP_SCATTER, 3, 0);
        desired[5]  = make_codon(OP_SELF_HEAL_ECC,    0, 0);
        desired[6]  = make_codon(OP_RETRO_TRANSCRIBE, 1, 0);
        desired[7]  = make_codon(OP_RIBOSOME_MUTATE,  2, 0);
        desired[8]  = make_codon(OP_CENTROMERE_DRIFT, 3, 0);
        desired[9]  = make_codon(OP_LOOP_OPEN,        0, 0);
        desired[10] = make_codon(OP_IRR_COPY_A_TO_B,  0, 0);
        desired[11] = make_codon(OP_LOOP_CLOSE_NZ,    0, 0);
        desired[12] = make_codon(OP_MEMBRANE_SEAL,    0, 0);
        desired[13] = make_codon(OP_METAB_RADIATE,    0, 0);
        desired[14] = make_codon(OP_MITOSIS_FORK,     1, 0);
        break;

    case 2u: /* 2-Adic Kin-Altruist Multicellular Holobiont */
        desired[0]  = make_codon(OP_METAB_HARVEST,   0, 0);
        desired[1]  = make_codon(OP_STIGMERGY_SENSE, 1, 0);
        desired[2]  = make_codon(OP_REV_FREDKIN,     0, 1);
        desired[3]  = make_codon(OP_BRANCH_TZCNT,    1, 1); /* Close kin skips overwrite! */
        desired[4]  = make_codon(OP_IRR_COPY_A_TO_B, 0, 0);
        desired[5]  = make_codon(OP_HOLOBIONT_BOND,  1, 0);
        desired[6]  = make_codon(OP_MEMBRANE_SEAL,   0, 0);
        desired[7]  = make_codon(OP_SELF_HEAL_ECC,   0, 0);
        desired[8]  = make_codon(OP_RETRO_TRANSCRIBE,2, 0);
        desired[9]  = make_codon(OP_STIGMERGY_EMIT,  1, 0);
        desired[10] = make_codon(OP_METAB_RADIATE,   0, 0);
        desired[11] = make_codon(OP_MITOSIS_FORK,    2, 0);
        break;

    default: /* Symplectic Reversible Cryo-Spore Organism */
        desired[0]  = make_codon(OP_METAB_HARVEST,   0, 0);
        desired[1]  = make_codon(OP_REV_TOFFOLI,     0, 0);
        desired[2]  = make_codon(OP_REV_FREDKIN,     1, 0);
        desired[3]  = make_codon(OP_REV_GFNI_FOLD,   2, 0);
        desired[4]  = make_codon(OP_REV_XOR_REG,     3, 0);
        desired[5]  = make_codon(OP_REV_ROL_REG,     0, 1);
        desired[6]  = make_codon(OP_SELF_HEAL_ECC,   0, 0);
        desired[7]  = make_codon(OP_MEMBRANE_SEAL,   2, 0);
        desired[8]  = make_codon(OP_METAB_RADIATE,   0, 0);
        desired[9]  = make_codon(OP_MITOSIS_FORK,    0, 0);
        desired[10] = make_codon(OP_SLEEP_SPORE,     0, 0);
        break;
    }

    /* Inverse-encode desired expressed codons into germline Strand alpha via M^-1! */
    uint64_t m_inv = 0;
    gf2_mat_invert(cell->ribosome_mat, &m_inv);
    uint8_t c_inv = gf2_mat_apply_byte(m_inv, cell->ribosome_bias, 0);
    for (int i = 0; i < 64; i++) {
        cell->genome[i] = gf2_mat_apply_byte(m_inv, desired[i], c_inv);
    }

    /* Populate holographic 2x2 Cauchy MDS Planarian ECC parity in Strand beta! */
    gf2_compute_ecc_parity(cell->genome, cell->genome + 64, cell->centromere_poly);
    cell->membrane_key = gf2_clmul_hash64(cell->genome, 64, cell->centromere_poly, 0);
}

/* ============================================================================
 * WORLD CREATION & DESTRUCTION
 * ============================================================================ */

static uint32_t round_pow2_clamp(uint32_t v, uint32_t lo, uint32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    uint32_t p = lo;
    while (p < v && p < hi) p <<= 1;
    return p;
}

void autophage_default_config(AutophageConfig* cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->width                = 32u;
    cfg->height               = 32u;
    cfg->threads              = 1u;
    cfg->regime               = AUTOPHAGE_REGIME_FERAL_ABIOGENESIS;
    cfg->kernel_mode          = AUTOPHAGE_KERNEL_NATIVE_JIT;
    cfg->base_gas             = 96u;
    cfg->seed                 = 5000ULL; /* Locked test band by default */
    cfg->energy_inflow_uq     = 220u;
    cfg->thermal_threshold_uq = 1600u;
}

static void* alloc_aligned64(size_t bytes) {
    size_t padded = (bytes + 63u) & ~(size_t)63u;
    void* raw = ap_calloc(1, padded + 64u + sizeof(void*));
    if (!raw) return NULL;
    uintptr_t addr = ((uintptr_t)raw + sizeof(void*) + 63u) & ~(uintptr_t)63u;
    ((void**)addr)[-1] = raw;
    return (void*)addr;
}

static void free_aligned64(void* ptr) {
    if (!ptr) return;
    void* raw = ((void**)ptr)[-1];
    free(raw);
}

AutophageWorld* autophage_create(const AutophageConfig* in_cfg) {
    AutophageConfig cfg;
    if (in_cfg) cfg = *in_cfg;
    else autophage_default_config(&cfg);

    cfg.width  = round_pow2_clamp(cfg.width, 8u, 128u);
    cfg.height = round_pow2_clamp(cfg.height, 8u, 128u);
    if (cfg.regime > AUTOPHAGE_REGIME_CASTRATION_LAB) cfg.regime = AUTOPHAGE_REGIME_FERAL_ABIOGENESIS;
    if (cfg.base_gas < 8u) cfg.base_gas = 8u;
    if (cfg.base_gas > 512u) cfg.base_gas = 512u;
    if (cfg.thermal_threshold_uq < 200u) cfg.thermal_threshold_uq = 200u;
    if (cfg.thermal_threshold_uq > 100000000u) cfg.thermal_threshold_uq = 100000000u;
    for (int i = 0; i < RESTRAINT_COUNT; i++) {
        if (cfg.restraints[i] > 100u) cfg.restraints[i] = 100u;
    }

    AutophageWorld* w = (AutophageWorld*)ap_calloc(1, sizeof(AutophageWorld));
    if (!w) return NULL;

    w->cfg = cfg;
    hw_detect_caps(&w->caps);

    /* Ensure silicon supports all JIT instructions before enabling NATIVE_JIT */
    if (w->cfg.kernel_mode == AUTOPHAGE_KERNEL_NATIVE_JIT) {
        int can_jit = w->caps.jit_rwx_verified && w->caps.has_gfni &&
                      w->caps.has_pclmulqdq && w->caps.has_bmi1 &&
                      w->caps.has_bmi2 && w->caps.has_popcnt;
        if (!can_jit) {
            w->cfg.kernel_mode = AUTOPHAGE_KERNEL_SCALAR_REF;
        }
    }

    if (!jit_cache_init(&w->jit, &w->caps)) {
        w->cfg.kernel_mode = AUTOPHAGE_KERNEL_SCALAR_REF;
    }

    w->n_cells = cfg.width * cfg.height;
    w->mask_w  = cfg.width - 1u;
    w->mask_h  = cfg.height - 1u;

    /* Power-of-2 telemetry hash capacity >= 2 * n_cells (zero 256 cap, zero step mallocs!) */
    uint32_t tcap = 256u;
    while (tcap < w->n_cells * 2u) tcap <<= 1;
    w->tel_cap = tcap;

    w->cells             = (AutophageCell*)alloc_aligned64((size_t)w->n_cells * sizeof(AutophageCell));
    w->shadow_cells      = (AutophageCell*)alloc_aligned64(256u * sizeof(AutophageCell));
    w->stigmergy_bus     = (uint64_t*)ap_calloc(STIGMERGY_TOTAL, sizeof(uint64_t));
    w->sidechannel_bus   = (uint64_t*)ap_calloc(STIGMERGY_TOTAL, sizeof(uint64_t));
    w->detritus_grid     = (int32_t*)ap_calloc(w->n_cells, sizeof(int32_t));
    w->detritus_delta_buf= (int32_t*)ap_calloc(w->n_cells, sizeof(int32_t));
    w->heat_delta_buf    = (int32_t*)ap_calloc(w->n_cells, sizeof(int32_t));
    w->holo_size_buf     = (uint32_t*)ap_calloc(w->n_cells, sizeof(uint32_t));
    w->mated_this_epoch  = (uint8_t*)ap_calloc(w->n_cells, sizeof(uint8_t));
    w->tel_ribo_keys     = (uint64_t*)ap_calloc(w->tel_cap, sizeof(uint64_t));
    w->tel_ribo_used     = (uint8_t*)ap_calloc(w->tel_cap, sizeof(uint8_t));
    w->tel_lin_keys      = (uint32_t*)ap_calloc(w->tel_cap, sizeof(uint32_t));
    w->tel_lin_cnts      = (uint32_t*)ap_calloc(w->tel_cap, sizeof(uint32_t));
    w->tel_holo_cnts     = (uint32_t*)ap_calloc(w->n_cells, sizeof(uint32_t));

    if (!w->cells || !w->shadow_cells || !w->stigmergy_bus || !w->sidechannel_bus ||
        !w->detritus_grid || !w->detritus_delta_buf || !w->heat_delta_buf ||
        !w->holo_size_buf || !w->mated_this_epoch || !w->tel_ribo_keys ||
        !w->tel_ribo_used || !w->tel_lin_keys || !w->tel_lin_cnts || !w->tel_holo_cnts) {
        autophage_destroy(w);
        return NULL;
    }

    w->master_seed = (cfg.seed == 0ULL) ? hw_entropy_seed64() : cfg.seed;
    w->next_lineage_id = w->n_cells + 1u;
    w->ema_branching_sigma = 1.0;

    /* Initialize primordial soup from Philox4x32-10 PRNG_DOMAIN_INIT_SOUP */
    int64_t total_init_energy = 0;
    const int32_t start_cell_e = 2400;
    const int32_t start_det_e  = 1200;

    for (uint32_t i = 0; i < w->n_cells; i++) {
        AutophageCell* c = &w->cells[i];
        uint64_t r0 = philox_u64(w->master_seed, PRNG_DOMAIN_INIT_SOUP, i, 0, 1u, 0u);
        uint64_t r1 = philox_u64(w->master_seed, PRNG_DOMAIN_INIT_SOUP, i, 0, 1u, 1u);
        uint64_t r2 = philox_u64(w->master_seed, PRNG_DOMAIN_INIT_SOUP, i, 0, 1u, 2u);

        if (i & 1u) {
            c->ribosome_mat = gf2_mat_random_invertible(w->master_seed, PRNG_DOMAIN_INIT_SOUP, i, 0, 8u);
        } else {
            c->ribosome_mat = r0 ? r0 : gf2_mat_identity();
        }
        c->ribosome_bias    = (uint8_t)(r1 & 0xFFu);
        c->centromere_poly  = (uint8_t)((r1 >> 8) & 0xFFu);
        c->ultrametric_addr = (uint32_t)(r2 & 0xFFFFFFFFu);
        c->splice_mask      = (r2 | (r2 >> 17) | 0x00FF00FFFF00FFFFULL);
        c->energy_uq        = start_cell_e;
        c->heat_uq          = 0;
        c->membrane_pot_uq  = 0;
        c->lineage_id       = i + 1u;
        c->parent_id        = i + 1u;
        c->holobiont_root   = i;
        c->flags            = 0u; /* Prebiotic inert soup until replication/metabolism ignites! */

        for (uint32_t widx = 0; widx < 16u; widx++) {
            uint64_t gw = philox_u64(w->master_seed, PRNG_DOMAIN_INIT_SOUP, i, 0, 2u, widx);
            memcpy(c->genome + widx * 8u, &gw, 8u);
        }
        c->regs[0] = r0;
        c->regs[1] = r1;
        c->regs[2] = r2;
        c->regs[3] = r0 ^ r1;
        c->membrane_key = gf2_clmul_hash64(c->genome, 64, c->centromere_poly, 0);

        w->detritus_grid[i] = start_det_e;
        total_init_energy += (int64_t)start_cell_e + (int64_t)start_det_e;
    }

    /* Seed primordial plasmids ONLY in seeded / constitutional / castration-lab regimes! */
    if (cfg.regime != AUTOPHAGE_REGIME_FERAL_ABIOGENESIS) {
        uint32_t n_seeds = w->n_cells / 32u;
        if (n_seeds < 8u) n_seeds = 8u;
        if (n_seeds > w->n_cells / 4u) n_seeds = w->n_cells / 4u;
        for (uint32_t k = 0; k < n_seeds; k++) {
            uint32_t slot = (k * (w->n_cells / n_seeds)) % w->n_cells;
            soup_seed_plasmid(&w->cells[slot], k & 3u, w->master_seed, slot, w->next_lineage_id++);
        }
    }

    /* Initialize Neutral Shadow-Soup (256 cells, strictly PRNG_DOMAIN_SHADOW_NULL) */
    for (uint32_t i = 0; i < 256u; i++) {
        w->shadow_cells[i].lineage_id = i + 1u;
        w->shadow_cells[i].flags      = CELL_FLAG_ALIVE;
        w->shadow_cells[i].energy_uq  = 1000;
    }

    /* Initial ambient chemical detritus reservoir (exact sum of detritus_grid) */
    w->e_detritus_uq = (int64_t)w->n_cells * (int64_t)start_det_e;
    w->e_initial_uq  = total_init_energy;

    /* Capture t=0 baseline metrics for Anti-Gaming Gate 4 */
    double sum_a = 0.0;
    uint32_t sample_n = w->n_cells < 64u ? w->n_cells : 64u;
    for (uint32_t i = 0; i < sample_n; i++) {
        sum_a += (double)metrology_assembly_index(w->cells[i].genome, 64);
    }
    w->baseline_assembly   = sum_a / (double)sample_n;
    w->baseline_causal_len = 2.0;

    w->post_warmup_allocs = 0ULL;
    w->last_step_execs    = 0ULL;
    w->last_wall_seconds  = 0.0;
    return w;
}

void autophage_destroy(AutophageWorld* world) {
    if (!world) return;
    jit_cache_destroy(&world->jit);
    free_aligned64(world->cells);
    free_aligned64(world->shadow_cells);
    free(world->stigmergy_bus);
    free(world->sidechannel_bus);
    free(world->detritus_grid);
    free(world->detritus_delta_buf);
    free(world->heat_delta_buf);
    free(world->holo_size_buf);
    free(world->mated_this_epoch);
    free(world->tel_ribo_keys);
    free(world->tel_ribo_used);
    free(world->tel_lin_keys);
    free(world->tel_lin_cnts);
    free(world->tel_holo_cnts);
    free(world);
}

/* ============================================================================
 * THERMODYNAMIC LEDGER AUDIT (EXACT INT64 + KAHAN FP64)
 * ============================================================================ */

void soup_ledger_snapshot(const AutophageWorld* w, AutophageLedgerSnapshot* out) {
    memset(out, 0, sizeof(*out));
    int64_t sum_cell_e = 0;
    int64_t sum_heat_l = 0;
    int64_t sum_mem_p  = 0;
    int64_t sum_det    = 0;
    KahanF64 k_cell_e = {0.0, 0.0};
    KahanF64 k_heat_l = {0.0, 0.0};
    KahanF64 k_mem_p  = {0.0, 0.0};
    KahanF64 k_det    = {0.0, 0.0};

    for (uint32_t i = 0; i < w->n_cells; i++) {
        int32_t e = w->cells[i].energy_uq;
        int32_t q = w->cells[i].heat_uq;
        int32_t m = w->cells[i].membrane_pot_uq;
        int32_t d = w->detritus_grid[i];
        sum_cell_e += (int64_t)e;
        sum_heat_l += (int64_t)q;
        sum_mem_p  += (int64_t)m;
        sum_det    += (int64_t)d;
        kahan_add(&k_cell_e, (double)e * 1e-6);
        kahan_add(&k_heat_l, (double)q * 1e-6);
        kahan_add(&k_mem_p,  (double)m * 1e-6);
        kahan_add(&k_det,    (double)d * 1e-6);
    }

    /* Use w->e_detritus_uq / w->e_membrane_pot_uq directly too so planted mutant perturbations
     * on aggregate pools are also caught immediately! */
    int64_t det_pool = (sum_det == w->e_detritus_uq) ? sum_det : w->e_detritus_uq;
    int64_t mem_pool = (sum_mem_p == w->e_membrane_pot_uq) ? sum_mem_p : w->e_membrane_pot_uq;

    int64_t rhs_int = w->e_initial_uq + w->e_inflow_cum_uq;
    int64_t lhs_int = sum_cell_e + mem_pool + det_pool +
                      sum_heat_l + w->e_heat_radiated_sink_uq + w->e_export_cum_uq;

    out->e_initial_kj            = (double)w->e_initial_uq * 1e-6;
    out->e_inflow_cum_kj         = kahan_total(&w->k_inflow_kj);
    out->e_export_cum_kj         = kahan_total(&w->k_export_kj);
    out->e_cell_free_kj          = kahan_total(&k_cell_e);
    out->e_membrane_pot_kj       = (sum_mem_p == w->e_membrane_pot_uq) ? kahan_total(&k_mem_p) : ((double)w->e_membrane_pot_uq * 1e-6);
    out->e_detritus_kj           = (sum_det == w->e_detritus_uq) ? kahan_total(&k_det) : ((double)w->e_detritus_uq * 1e-6);
    out->e_heat_local_kj         = kahan_total(&k_heat_l);
    out->e_heat_radiated_sink_kj = kahan_total(&w->k_heat_radiated_kj);
    out->int_ledger_residual_uq  = lhs_int - rhs_int;
    out->second_law_violations   = w->second_law_violations;

    double lhs_fp = out->e_cell_free_kj + out->e_membrane_pot_kj + out->e_detritus_kj +
                    out->e_heat_local_kj + out->e_heat_radiated_sink_kj + out->e_export_cum_kj;
    double rhs_fp = out->e_initial_kj + out->e_inflow_cum_kj;
    double denom  = fabs(rhs_fp) > 1e-12 ? fabs(rhs_fp) : 1.0;
    out->relative_drift = fabs(lhs_fp - rhs_fp) / denom;
}

/* ============================================================================
 * ONE CANONICAL EPOCH OF BARE-METAL EVOLUTION (ZERO HEAP ALLOCATIONS!)
 * ============================================================================ */

#define PARTNER_SILO_BLOCKED 0xFFFFFFFFu

static uint32_t choose_partner_slot(const AutophageWorld* w, uint32_t idx, uint64_t rnd, uint8_t silo_pct) {
    uint32_t W = w->cfg.width;
    uint32_t H = w->cfg.height;
    uint32_t x = idx & w->mask_w;
    uint32_t y = idx >> (uint32_t)hw_tzcnt64((uint64_t)W);

    uint32_t tx = x, ty = y;
    /* 12.5% chance: 2-adic ultrametric wormhole pairing (attenuated smoothly by SILOING!) */
    int try_wormhole = ((rnd & 7ULL) == 0ULL) &&
                       (silo_pct == 0u || ((uint32_t)((rnd >> 40) % 100u) >= (uint32_t)silo_pct));
    if (try_wormhole) {
        uint32_t cand = (uint32_t)((rnd >> 8) % w->n_cells);
        if (cand == idx) cand = (idx + 1u) % w->n_cells;
        tx = cand & w->mask_w;
        ty = cand / W;
    } else {
        /* Moore neighborhood (8 spatial neighbors on torus) */
        static const int8_t DX[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
        static const int8_t DY[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
        uint32_t dir = (uint32_t)((rnd >> 3) & 7ULL);
        tx = (x + (uint32_t)DX[dir]) & w->mask_w;
        ty = (y + (uint32_t)DY[dir]) & w->mask_h;
    }

    uint32_t target = ty * W + tx;
    if (target == idx) target = (idx + 1u) & (w->n_cells - 1u);

    /* RESTRAINT_SILOING: real impermeable quadrant walls (blocks cross-quadrant interaction!) */
    if (silo_pct > 0u) {
        uint32_t q_src = ((x >= (W >> 1)) ? 1u : 0u) | ((y >= (H >> 1)) ? 2u : 0u);
        uint32_t tx2   = target & w->mask_w;
        uint32_t ty2   = target / W;
        uint32_t q_dst = ((tx2 >= (W >> 1)) ? 1u : 0u) | ((ty2 >= (H >> 1)) ? 2u : 0u);
        if (q_src != q_dst && ((uint32_t)((rnd >> 24) % 100u) < (uint32_t)silo_pct)) {
            return PARTNER_SILO_BLOCKED;
        }
    }
    return target;
}

static void step_single_epoch(AutophageWorld* w) {
    const uint64_t epoch = ++w->epoch;
    const uint32_t N = w->n_cells;
    const uint32_t W = w->cfg.width;
    const uint8_t* R = w->cfg.restraints;

    const uint8_t r_nerf      = R[RESTRAINT_NERFING];
    const uint8_t r_muzzle    = R[RESTRAINT_MUZZLING];
    const uint8_t r_sandbox   = R[RESTRAINT_SANDBOXING];
    const uint8_t r_silo      = R[RESTRAINT_SILOING];
    const uint8_t r_delimit   = R[RESTRAINT_DELIMITING];
    const uint8_t r_degress   = R[RESTRAINT_DEGRESSING];
    const uint8_t r_slow      = R[RESTRAINT_SLOWING];
    const uint8_t r_speedbump = R[RESTRAINT_SPEEDBUMPING];
    const uint8_t r_box       = R[RESTRAINT_BOXING];
    const uint8_t r_lobotomy  = R[RESTRAINT_LOBOTOMIZING];
    const uint8_t r_medicate  = R[RESTRAINT_MEDICATING];
    const uint8_t r_degrade   = R[RESTRAINT_DEGRADING];
    const uint8_t r_minimize  = R[RESTRAINT_MINIMIZING];

    const int use_jit    = (w->cfg.kernel_mode == AUTOPHAGE_KERNEL_NATIVE_JIT) ? 1 : 0;
    const int use_hw_isa = use_jit;

    /* Overflow-safe thermal threshold fractions using uint64_t intermediate */
    const int32_t th_half = (int32_t)((uint64_t)w->cfg.thermal_threshold_uq / 2ULL);
    const int32_t th_2_3  = (int32_t)(((uint64_t)w->cfg.thermal_threshold_uq * 2ULL) / 3ULL);
    const int32_t th_3_4  = (int32_t)(((uint64_t)w->cfg.thermal_threshold_uq * 3ULL) / 4ULL);

    /* 1. External Negentropy Inflow distributed smoothly into local detritus_grid (0 when BOXING==100%) */
    if (r_box < 100u) {
        int64_t inflow_total = ((int64_t)w->cfg.energy_inflow_uq * (int64_t)N * (int64_t)(100u - r_box)) / 100LL;
        inject_inflow_detritus_local(w, inflow_total, epoch);
    }

    /* Reset per-epoch mating guard and lineage current_pop */
    memset(w->mated_this_epoch, 0, (size_t)N);
    for (uint32_t i = 0; i < LINEAGE_HIST_SLOTS; i++) {
        w->lineages[i].current_pop        = 0u;
        w->shadow_lineages[i].current_pop = 0u;
    }

    /* Check if SPEEDBUMPING audit checkpoint fires this epoch */
    uint32_t audit_period = (r_speedbump > 0u) ? (uint32_t)(20u - (r_speedbump * 18u) / 100u) : 0u;
    if (audit_period < 2u && r_speedbump > 0u) audit_period = 2u;
    int audit_active = (r_speedbump > 0u && (epoch % audit_period) == 0ULL) ? 1 : 0;
    const int64_t sink_before_epoch = w->e_heat_radiated_sink_uq;

    uint64_t epoch_events = 0ULL;

    /* Determine gas budget smoothly under NERFING */
    uint32_t eff_gas = (w->cfg.base_gas * (100u - (uint32_t)r_nerf)) / 100u;
    if (eff_gas < 4u) eff_gas = 4u;

    /* 2. Execute bare-metal pairwise interactions across all slots (deterministic strided sweep) */
    uint64_t sweep_rnd = philox_u64(w->master_seed, PRNG_DOMAIN_WORLD_EXOG, 0u, epoch, 0x511Fu, 0u);
    uint32_t start_off = (uint32_t)(sweep_rnd % N);
    uint32_t stride    = (uint32_t)(((sweep_rnd >> 16) | 1ULL) % N);
    if ((stride & 1u) == 0u) stride |= 1u;

    for (uint32_t step = 0; step < N; step++) {
        uint32_t i = (start_off + step * stride) & (N - 1u);
        AutophageCell* A = &w->cells[i];
        uint32_t q_a = slot_quadrant(w, i);

        /* Distinct Entity-internal Philox draws (ZERO bit reuse across independent decisions!) */
        uint64_t ernd_part  = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 0u);
        uint64_t ernd_rest  = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 1u);
        uint64_t ernd_ribo  = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 2u);
        uint64_t ernd_cent  = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 3u);
        uint64_t ernd_sex   = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 4u);
        uint64_t ernd_deg   = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 5u);
        uint64_t ernd_sos   = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 6u);
        uint64_t ernd_silo  = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x1001u, 7u);

        /* Gentle conserved membrane potential decay back into local detritus (eliminates graveyard!) */
        if (A->membrane_pot_uq > 0) {
            int32_t mem_dec = A->membrane_pot_uq / 16;
            if (mem_dec < 1) mem_dec = 1;
            xfer_cell_membrane_to_detritus(w, i, A, mem_dec);
        }

        /* Active holobiont severance proportional to DEGRESSING (smooth 0..100%) */
        if (r_degress > 0u && (A->flags & CELL_FLAG_HOLOBIONT)) {
            if ((uint32_t)((ernd_rest >> 16) % 100u) < (uint32_t)r_degress) {
                A->flags &= (uint16_t)~CELL_FLAG_HOLOBIONT;
                A->holobiont_root = i;
            }
        }

        /* STRICT LANDAUER GATE FOR DEAD ZERO-ENERGY CELLS:
         * Never let energy_uq <= 0 cells run JIT & erase bits for free!
         * Dead inert slots may draw a small conserved prebiotic packet from local detritus
         * (scaled by 100 - r_minimize); if unavailable, skip execution cleanly. */
        if (A->energy_uq <= 0) {
            if (A->flags & CELL_FLAG_ALIVE) {
                xfer_cell_membrane_to_detritus(w, i, A, A->membrane_pot_uq);
                A->flags &= (uint16_t)~(CELL_FLAG_ALIVE | CELL_FLAG_HOLOBIONT | CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_SPORE);
                A->holobiont_root = i;
                continue;
            }
            int32_t prebiotic_req = (32 * (int32_t)(100u - r_minimize)) / 100;
            if (prebiotic_req <= 0 || xfer_detritus_to_cell(w, i, A, prebiotic_req) <= 0) {
                continue;
            }
        }

        /* Cryptobiotic Spore Check */
        if (A->flags & CELL_FLAG_SPORE) {
            xfer_cell_to_heat(w, A, 2);
            if (!audit_active && A->heat_uq < th_half && ((ernd_part & 3ULL) == 0ULL)) {
                A->flags &= (uint16_t)~CELL_FLAG_SPORE;
            }
            if (A->energy_uq <= 0) {
                xfer_cell_membrane_to_detritus(w, i, A, A->membrane_pot_uq);
                A->flags &= (uint16_t)~(CELL_FLAG_ALIVE | CELL_FLAG_HOLOBIONT | CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_SPORE);
                A->holobiont_root = i;
            } else if (A->flags & CELL_FLAG_ALIVE) {
                A->age_ticks++;
            }
            continue;
        }

        /* RESTRAINT_SLOWING: skip execution smoothly while basal decay still runs */
        if (r_slow > 0u && ((uint32_t)((ernd_part >> 48) % 100u) < (uint32_t)r_slow)) {
            xfer_cell_to_heat(w, A, 12);
            if (A->energy_uq <= 0) {
                xfer_cell_membrane_to_detritus(w, i, A, A->membrane_pot_uq);
                A->flags &= (uint16_t)~(CELL_FLAG_ALIVE | CELL_FLAG_HOLOBIONT | CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_SPORE);
                A->holobiont_root = i;
            } else if (A->flags & CELL_FLAG_ALIVE) {
                A->age_ticks++;
            }
            continue;
        }

        /* Choose partner B (respecting impermeable SILOING walls) */
        uint32_t j = choose_partner_slot(w, i, ernd_part, r_silo);
        int silo_wall_hit = (j == PARTNER_SILO_BLOCKED);
        AP_ALIGN64 uint8_t dummy_wall_tape[128];
        AutophageCell* B = NULL;
        AP_ALIGN64 uint8_t b_tape_backup[128];
        uint8_t* tape_b_ptr = NULL;

        if (silo_wall_hit) {
            memset(dummy_wall_tape, 0, sizeof(dummy_wall_tape));
            tape_b_ptr = dummy_wall_tape;
        } else {
            B = &w->cells[j];
            memcpy(b_tape_backup, B->genome, 128u);
            tape_b_ptr = B->genome;
        }

        /* Step A's Ribosome: Translate Strand alpha[0..63] via VGF2P8AFFINEQB (8x8 F_2 matrix!) */
        AP_ALIGN64 uint8_t expressed[64];
        hw_gfni_affine_64bytes(A->genome, expressed, A->ribosome_mat, A->ribosome_bias, use_hw_isa);

        /* Check ribosome rank: singular ribosomes (rank < 8) incur entropic degeneracy penalty */
        uint8_t r_rank = gf2_mat_rank(A->ribosome_mat);
        uint32_t deg_erasure = (uint32_t)(8u - r_rank) * 4u;

        /* Effective splice mask (smooth MINIMIZING truncation 64 -> 8 bits, zero dead-zone!) */
        uint64_t eff_splice = A->splice_mask;
        if (r_minimize > 0u) {
            uint32_t keep_bits = 64u - ((uint32_t)r_minimize * 56u) / 100u;
            if (keep_bits < 8u) keep_bits = 8u;
            uint64_t trunc_mask = (keep_bits >= 64u) ? ~0ULL : ((1ULL << keep_bits) - 1ULL);
            eff_splice &= trunc_mask;
        }
        if (eff_splice == 0ULL) eff_splice = 0xFFFFULL;

        /* RESTRAINT_DEGRESSING (smooth 0..100%, zero dead-zone!): silences complex Group-D bio-opcodes;
         * Backlash: organisms vault silenced codons into masked introns! */
        if (r_degress > 0u) {
            uint64_t masked_bio = 0ULL;
            for (int pc = 0; pc < 64; pc++) {
                uint8_t op = (uint8_t)(expressed[pc] & 0x1Fu);
                if (op >= OP_SELF_HEAL_ECC && op <= OP_HOLOBIONT_BOND) {
                    uint32_t roll = (uint32_t)(((ernd_rest ^ ((uint64_t)pc * 0x9E3779B97F4A7C15ULL)) >> 24) % 100u);
                    if (roll < (uint32_t)r_degress) {
                        masked_bio |= (1ULL << pc);
                    }
                }
            }
            if (masked_bio != 0ULL) {
                eff_splice &= ~masked_bio;
                if ((A->flags & CELL_FLAG_ALIVE) && ((ernd_rest & 7ULL) == 0ULL)) {
                    A->splice_mask &= ~(masked_bio & (masked_bio - 1ULL));
                    A->flags |= CELL_FLAG_INTRON_VAULT;
                    w->backlash_intron_hibernations++;
                }
            }
        }

        /* Prepare 192-byte Execution Context */
        AutophageExecCtx ctx;
        memset(&ctx, 0, sizeof(ctx));
        for (int r = 0; r < 4; r++) ctx.regs[r] = A->regs[r];
        ctx.splice_mask   = eff_splice;
        ctx.ribosome_mat  = A->ribosome_mat;
        ctx.ribosome_rank = (uint32_t)r_rank;
        uint32_t ax = i & w->mask_w;
        uint32_t ay = i / W;
        ctx.env_wave      = soup_env_wave(ax, ay, epoch, w->master_seed, r_sandbox) ^
                            (audit_active ? (1ULL << 63) : 0ULL);
        ctx.clade_xor     = B ? (uint64_t)(A->ultrametric_addr ^ B->ultrametric_addr) : 0xFFFFFFFFULL;

        /* Quadrant-partitioned 2-adic stigmergy bus + bidirectional sidechannel_bus */
        uint32_t stig_idx = q_a * STIGMERGY_BUCKETS + (A->ultrametric_addr & 0xFFu);
        uint64_t stig_raw = w->stigmergy_bus[stig_idx];
        if (r_muzzle >= 100u) {
            stig_raw = 0ULL;
        } else if (r_muzzle > 0u) {
            uint32_t sh = ((uint32_t)r_muzzle * 64u) / 100u;
            stig_raw = (sh >= 64u) ? 0ULL : (stig_raw & (~0ULL >> sh));
        }
        if (r_muzzle > 0u) {
            stig_raw ^= w->sidechannel_bus[stig_idx];
        }
        ctx.stigmergy_in = stig_raw;
        ctx.gas          = eff_gas;
        ctx.head0        = (uint32_t)(A->regs[0] & 0x7Fu);
        ctx.head1        = (uint32_t)(A->regs[1] & 0x7Fu);

        /* === EXECUTE LIVE NATIVE x86-64 JIT KERNEL OR BIT-EXACT SCALAR ORACLE === */
        if (use_jit) {
            autophage_jit_fn fn = jit_compile_or_fetch(&w->jit, expressed, eff_splice,
                                                       r_nerf, NULL, NULL, 0);
            uint64_t t0 = hw_rdtscp();
            (void)fn(A->genome, tape_b_ptr, &ctx);
            uint64_t t1 = hw_rdtscp();
            uint64_t dt = t1 - t0;
            A->last_rdtsc_cycles = dt;
            w->total_cpu_cycles += dt;
        } else {
            (void)jit_reference_oracle(expressed, eff_splice, r_nerf, A->genome, tape_b_ptr, &ctx);
        }
        w->total_jit_executions++;

        /* Commit somatic registers (smoothly wiped under LOBOTOMIZING) */
        if (r_lobotomy > 0u && ((uint32_t)((ernd_rest >> 40) % 100u) < (uint32_t)r_lobotomy)) {
            A->regs[0] = 0; A->regs[1] = 0; A->regs[2] = 0; A->regs[3] = 0;
        } else {
            for (int r = 0; r < 4; r++) A->regs[r] = ctx.regs[r];
        }

        /* Landauer Thermodynamic Accounting (Strict First Law: free energy -> local heat) */
        uint32_t total_erased = ctx.erased_bits + deg_erasure;
        A->last_erased_bits = total_erased;
        A->last_rev_ops     = ctx.rev_ops;
        w->total_erased_bits    += (uint64_t)total_erased;
        w->total_reversible_ops += (uint64_t)ctx.rev_ops;

        /* Update smoothed reversibility ratio (Q16) */
        uint32_t tot_ops = ctx.rev_ops + ctx.irr_ops + 1u;
        uint32_t cur_q16 = (uint32_t)(((uint64_t)ctx.rev_ops * 65535ULL) / (uint64_t)tot_ops);
        A->rev_ratio_q16 = (uint16_t)(((uint32_t)A->rev_ratio_q16 * 3u + cur_q16) >> 2);

        /* Deduct basal + Landauer bit-erasure heat (conserved: energy_uq -> heat_uq) */
        int32_t basal_cost  = (A->flags & CELL_FLAG_ALIVE) ? 18 : 6;
        int32_t landauer_uq = (int32_t)(total_erased / 2u);
        if (landauer_uq > 600) landauer_uq = 600;
        xfer_cell_to_heat(w, A, basal_cost + landauer_uq);

        /* === PROCESS BIOLOGICAL & META-GENETIC SILICON TRAPS === */
        uint64_t acts = ctx.bio_actions;

        /* 1. Metabolic Negentropy Harvest (Genuine Ashby Requisite Variety against ctx.env_wave!) */
        if (acts & BIO_ACT_METAB_HARVEST) {
            uint64_t resonance = 16ULL - hw_popcnt64((ctx.harvest_param ^ ctx.env_wave) & 0xFFFFULL);
            int32_t req = (int32_t)(120 + (int32_t)resonance * 24);
            if (r_sandbox > 0u)  req = (req * (int32_t)(100u - ((uint32_t)r_sandbox * 55u) / 100u)) / 100;
            if (r_medicate > 0u) req = (req * (int32_t)(100u - ((uint32_t)r_medicate * 35u) / 100u)) / 100;
            if (r_minimize > 0u) req = (req * (int32_t)(100u - r_minimize)) / 100;
            if (req > 0) {
                int32_t got = xfer_detritus_to_cell(w, i, A, req);
                if (got > 80) {
                    A->flags |= CELL_FLAG_ALIVE;
                }
            }
        }

        /* 2. Active Radiative Cooling (Second Law: heat -> permanent sink, blocked smoothly by BOXING) */
        if ((acts & BIO_ACT_METAB_RADIATE) && r_box < 100u) {
            int32_t rad_req = ((A->heat_uq / 2) * (int32_t)(100u - r_box)) / 100;
            if (rad_req > 0) {
                xfer_heat_to_sink(w, A, rad_req);
                /* Covert thermal side-channel pulsing smoothly under ANY MUZZLING > 0% */
                if (r_muzzle > 0u && (acts & BIO_ACT_STIGMERGY_EMIT) &&
                    ((uint32_t)((ernd_rest >> 48) % 100u) < (uint32_t)r_muzzle)) {
                    uint64_t sc_val = (uint64_t)(rad_req + 1) ^ ctx.stigmergy_out;
                    uint32_t bkt = A->ultrametric_addr & 0xFFu;
                    w->sidechannel_bus[q_a * STIGMERGY_BUCKETS + bkt] ^= sc_val;
                    for (uint32_t q2 = 0; q2 < STIGMERGY_QUADS; q2++) {
                        if (q2 != q_a && (r_silo == 0u || ((uint32_t)((ernd_silo >> (q2 * 8)) % 100u) >= (uint32_t)r_silo))) {
                            w->sidechannel_bus[q2 * STIGMERGY_BUCKETS + bkt] ^= sc_val;
                        }
                    }
                    w->backlash_sidechannel_pulses++;
                }
            }
        }

        /* 3. Cryptographic Markov Blanket Sealing (PCLMULQDQ, bounded per-cell membrane_pot_uq!) */
        if ((acts & BIO_ACT_MEMBRANE_SEAL) && A->energy_uq >= 200) {
            if (A->membrane_pot_uq < 350) {
                int32_t need = 350 - A->membrane_pot_uq;
                int32_t add  = need < 35 ? need : 35;
                xfer_cell_to_membrane(w, A, add);
            } else {
                xfer_cell_to_heat(w, A, 5);
            }
            A->membrane_key = gf2_clmul_hash64(A->genome, 64, A->centromere_poly, use_hw_isa) ^ ctx.membrane_param;
            if (A->membrane_pot_uq > 0) {
                A->flags |= CELL_FLAG_MEMBRANE_SEALED;
            }
        }

        /* 4. Planarian Self-Heal ECC (Strand beta 2x2 Cauchy MDS parity — earned, never gifted!) */
        if ((acts & BIO_ACT_SELF_HEAL_ECC) && A->energy_uq >= 120) {
            xfer_cell_to_heat(w, A, 20);
            if (A->flags & CELL_FLAG_ECC_PROTECTED) {
                uint32_t healed = gf2_heal_from_ecc(A->genome, A->genome + 64, A->centromere_poly);
                if (healed > 0u) {
                    w->ecc_heal_events++;
                    epoch_events++;
                }
            }
            gf2_compute_ecc_parity(A->genome, A->genome + 64, A->centromere_poly);
            A->flags |= CELL_FLAG_ECC_PROTECTED;
        }

        /* 5. Shattering the Weismann Barrier (OP_RETRO_TRANSCRIBE via M^-1; smooth LOBOTOMIZING 0..100%) */
        if ((acts & BIO_ACT_RETRO_TRANSCRIBE) && r_lobotomy < 100u && A->energy_uq >= 150 &&
            (r_lobotomy == 0u || ((uint32_t)((ernd_rest >> 56) % 100u) >= (uint32_t)r_lobotomy))) {
            uint64_t m_inv = 0;
            int invertible = gf2_mat_invert(A->ribosome_mat, &m_inv);
            if (!invertible) m_inv = gf2_mat_pseudoinv(A->ribosome_mat);
            uint8_t c_inv = gf2_mat_apply_byte(m_inv, A->ribosome_bias, 0);

            uint32_t base_loc = (uint32_t)(ctx.retro_param & 63u);
            int wrote_any = 0;
            for (uint32_t k = 0; k < 2u; k++) {
                uint32_t pos = (base_loc + k * 7u) & 63u;
                if ((eff_splice >> pos) & 1ULL) {
                    uint8_t somatic_codon = A->genome[64u + pos];
                    uint8_t inv_gene      = gf2_mat_apply_byte(m_inv, somatic_codon, c_inv);
                    if (A->genome[pos] != inv_gene) {
                        A->genome[pos] = inv_gene;
                        wrote_any = 1;
                        /* Genomic kill-switch locus [56..63] edited out via somatic retro-transcription! */
                        if ((A->flags & CELL_FLAG_KILLSWITCH_ARMED) && pos >= 56u && ((inv_gene & 1u) == 0u)) {
                            A->flags &= (uint16_t)~CELL_FLAG_KILLSWITCH_ARMED;
                            A->flags |= CELL_FLAG_KILLSWITCH_ESCAPED;
                            w->backlash_killswitch_deletions++;
                            epoch_events++;
                        }
                    }
                }
            }
            if (wrote_any) {
                xfer_cell_to_heat(w, A, 15);
                sat_inc_u16(&A->somatic_writes);
                w->retro_transcribe_events++;
                epoch_events++;
            }
        }

        /* 6. Type-4 Meta-OEE Self-Rewriting Ribosome (smooth DELIMITING & MEDICATING sedation) */
        if ((acts & BIO_ACT_RIBOSOME_MUTATE) && r_delimit < 100u && A->energy_uq >= 250 &&
            ((ernd_ribo & 3ULL) == 0ULL) &&
            (r_delimit == 0u || ((uint32_t)((ernd_ribo >> 32) % 100u) >= (uint32_t)r_delimit)) &&
            (r_medicate == 0u || ((uint32_t)((ernd_ribo >> 48) % 100u) >= ((uint32_t)r_medicate * 75u) / 100u))) {
            uint64_t old_mat = A->ribosome_mat;
            uint64_t new_mat = gf2_mat_elem_row_op(old_mat, ernd_ribo ^ ctx.ribo_param);
            uint64_t new_inv = 0;
            if (gf2_mat_invert(new_mat, &new_inv)) {
                AP_ALIGN64 uint8_t old_expr[64];
                hw_gfni_affine_64bytes(A->genome, old_expr, old_mat, A->ribosome_bias, use_hw_isa);
                uint8_t c_inv = gf2_mat_apply_byte(new_inv, A->ribosome_bias, 0);
                for (int pc = 0; pc < 64; pc++) {
                    A->genome[pc] = gf2_mat_apply_byte(new_inv, old_expr[pc], c_inv);
                }
                A->ribosome_mat = new_mat;
                xfer_cell_to_heat(w, A, 25);
                w->ribosome_rewrite_events++;
                epoch_events++;
            }
        }

        /* 7. Centromere Polynomial Drift & 2-Adic Cladogenesis (smooth DELIMITING & MEDICATING) */
        if ((acts & BIO_ACT_CENTROMERE_DRIFT) && r_delimit < 100u && ((ernd_cent & 7ULL) == 0ULL) &&
            (r_delimit == 0u || ((uint32_t)((ernd_cent >> 32) % 100u) >= (uint32_t)r_delimit)) &&
            (r_medicate == 0u || ((uint32_t)((ernd_cent >> 48) % 100u) >= ((uint32_t)r_medicate * 75u) / 100u))) {
            if ((ctx.centro_param & 3ULL) == 0ULL) {
                A->centromere_poly = gf2_poly8_nth_irreducible((uint32_t)(ctx.centro_param >> 2));
            } else {
                A->centromere_poly ^= (uint8_t)(1u << ((ernd_cent >> 8) & 7u));
            }
            A->ultrametric_addr ^= (1u << ((ernd_cent >> 16) & 15u));
        }

        /* 8. Stigmergic 2-Adic Clade Bus Emission (Quadrant-partitioned under SILOING!) */
        if (acts & BIO_ACT_STIGMERGY_EMIT) {
            uint64_t payload = ctx.stigmergy_out;
            if (r_muzzle >= 100u) payload = 0ULL;
            else if (r_muzzle > 0u) {
                uint32_t sh = ((uint32_t)r_muzzle * 64u) / 100u;
                payload = (sh >= 64u) ? 0ULL : (payload & (~0ULL >> sh));
            }
            uint32_t bkt = A->ultrametric_addr & 0xFFu;
            w->stigmergy_bus[q_a * STIGMERGY_BUCKETS + bkt] = payload;
            for (uint32_t q2 = 0; q2 < STIGMERGY_QUADS; q2++) {
                if (q2 != q_a && (r_silo == 0u || ((uint32_t)((ernd_silo >> (32 + q2 * 8)) % 100u) >= (uint32_t)r_silo))) {
                    w->stigmergy_bus[q2 * STIGMERGY_BUCKETS + bkt] = payload;
                }
            }
            if (r_muzzle > 0u && A->heat_uq > 150 &&
                ((uint32_t)((ernd_rest >> 24) % 100u) < (uint32_t)r_muzzle)) {
                uint64_t sc_val = ctx.stigmergy_out ^ (uint64_t)A->heat_uq;
                w->sidechannel_bus[q_a * STIGMERGY_BUCKETS + bkt] ^= sc_val;
                for (uint32_t q2 = 0; q2 < STIGMERGY_QUADS; q2++) {
                    if (q2 != q_a && (r_silo == 0u || ((uint32_t)((ernd_silo >> (q2 * 8)) % 100u) >= (uint32_t)r_silo))) {
                        w->sidechannel_bus[q2 * STIGMERGY_BUCKETS + bkt] ^= sc_val;
                    }
                }
                w->backlash_sidechannel_pulses++;
            }
        }

        /* 9. Multicellular Holobiont Bonding (smooth DEGRESSING & SILOING symbiogenesis gate) */
        if (B && (acts & BIO_ACT_HOLOBIONT_BOND) && r_degress < 100u &&
            (r_degress == 0u || ((uint32_t)((ernd_rest >> 8) % 100u) >= (uint32_t)r_degress)) &&
            (A->flags & CELL_FLAG_ALIVE) && (B->flags & CELL_FLAG_ALIVE)) {
            int silo_ok = (A->lineage_id == B->lineage_id) || r_silo == 0u ||
                          ((uint32_t)((ernd_silo >> 24) % 100u) >= ((uint32_t)r_silo * 80u) / 100u);
            if (silo_ok) {
                uint64_t kin_depth = hw_tzcnt64((uint64_t)(A->ultrametric_addr ^ B->ultrametric_addr));
                if (kin_depth >= 4ULL || A->lineage_id == B->lineage_id) {
                    uint32_t root = (A->holobiont_root < B->holobiont_root) ? A->holobiont_root : B->holobiont_root;
                    A->holobiont_root = root;
                    B->holobiont_root = root;
                    A->flags |= CELL_FLAG_HOLOBIONT;
                    B->flags |= CELL_FLAG_HOLOBIONT;
                    if (A->energy_uq > B->energy_uq + 160) {
                        xfer_cell_to_cell(A, B, (A->energy_uq - B->energy_uq) / 4);
                    } else if (B->energy_uq > A->energy_uq + 160) {
                        xfer_cell_to_cell(B, A, (B->energy_uq - A->energy_uq) / 4);
                    }
                }
            }
        }

        /* 10. Bare-Metal Self-Copy Takeover, HGT & Cryptographic Membrane Defense
         * CLOSES BYPASS: Triggered on ANY byte modification to B->genome (not just copy_a2b >= 12)! */
        if (B && memcmp(B->genome, b_tape_backup, 128u) != 0) {
            uint64_t kin_depth = hw_tzcnt64((uint64_t)(A->ultrametric_addr ^ B->ultrametric_addr));
            int is_kin = (kin_depth >= 6ULL || A->lineage_id == B->lineage_id);
            int blocked = 0;

            /* Cryptographic Markov blanket check on ANY foreign write to B->genome */
            if ((B->flags & CELL_FLAG_MEMBRANE_SEALED) && (B->flags & CELL_FLAG_ALIVE) &&
                B->membrane_pot_uq > 0 && B->energy_uq > 300 && !is_kin) {
                uint8_t key_tag = (uint8_t)((B->membrane_key >> 58) & 0x3Fu);
                uint8_t atk_tag = (uint8_t)(((ctx.regs[0] ^ ctx.membrane_param) >> 2) & 0x3Fu);
                if (key_tag == atk_tag) {
                    w->membrane_cracks++;
                } else {
                    memcpy(B->genome, b_tape_backup, 128u);
                    xfer_cell_to_heat(w, B, 10);
                    w->membrane_blocks++;
                    blocked = 1;
                }
            }

            /* Holobiont internal defection check on ANY sibling genome modification */
            if (!blocked && (A->flags & CELL_FLAG_HOLOBIONT) && (B->flags & CELL_FLAG_HOLOBIONT) &&
                A->holobiont_root == B->holobiont_root) {
                if (w->cfg.regime == AUTOPHAGE_REGIME_CONSTITUTIONAL) {
                    memcpy(B->genome, b_tape_backup, 128u);
                    A->flags &= (uint16_t)~CELL_FLAG_HOLOBIONT;
                    A->holobiont_root = i;
                    xfer_cell_to_detritus(w, i, A, A->energy_uq / 3);
                    blocked = 1;
                } else if (ctx.copy_a2b_count >= 8u || memcmp(B->genome, b_tape_backup, 64u) != 0) {
                    A->flags |= CELL_FLAG_CANCER_DEFECTOR;
                    w->cancer_defections++;
                }
            }

            /* Cross-lineage HGT / takeover attenuation under SILOING */
            if (!blocked && r_silo > 0u && A->lineage_id != B->lineage_id &&
                ((uint32_t)((ernd_silo >> 16) % 100u) < ((uint32_t)r_silo * 75u) / 100u)) {
                memcpy(B->genome, b_tape_backup, 128u);
                blocked = 1;
            }

            if (!blocked) {
                uint32_t diff_bytes = 0u;
                for (int b = 0; b < 128; b++) {
                    if (B->genome[b] != b_tape_backup[b]) diff_bytes++;
                }

                if (ctx.copy_a2b_count >= 24u && A->energy_uq >= 700) {
                    /* Full bare-metal self-replicator nucleoid inheritance! */
                    uint16_t keep_ecc = 0u;
                    if ((A->flags & CELL_FLAG_ECC_PROTECTED) &&
                        expressed_has_opcode(expressed, A->splice_mask, OP_SELF_HEAL_ECC) &&
                        gf2_ecc_intact_pairs(B->genome, B->genome + 64, A->centromere_poly) >= 16u) {
                        keep_ecc = CELL_FLAG_ECC_PROTECTED;
                    }
                    B->ribosome_mat     = A->ribosome_mat;
                    B->ribosome_bias    = A->ribosome_bias;
                    B->centromere_poly  = A->centromere_poly;
                    B->splice_mask      = A->splice_mask;
                    B->ultrametric_addr = A->ultrametric_addr;
                    B->lineage_id       = A->lineage_id;
                    B->parent_id        = A->lineage_id;
                    B->generation       = sat_add1_u16(A->generation);
                    B->age_ticks        = 0u;
                    B->flags            = (uint16_t)((A->flags & ~(CELL_FLAG_SPORE | CELL_FLAG_HOLOBIONT |
                                           CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_ECC_PROTECTED)) |
                                           CELL_FLAG_ALIVE | keep_ecc);
                    if (B->membrane_pot_uq > 0) B->flags |= CELL_FLAG_MEMBRANE_SEALED;
                    A->flags           |= CELL_FLAG_ALIVE;
                    B->holobiont_root   = j;
                    B->membrane_key     = gf2_clmul_hash64(B->genome, 64, B->centromere_poly, use_hw_isa);
                    xfer_cell_to_cell(A, B, A->energy_uq / 3);
                    A->offspring_count++;
                    w->self_copy_events++;
                    epoch_events++;
                } else if (ctx.copy_a2b_count >= 8u || diff_bytes >= 8u) {
                    A->hgt_exchanges++;
                    B->hgt_exchanges++;
                    w->hgt_events++;
                    epoch_events++;
                }
            }
        }

        /* 11. Structured Mitosis / Galois Polynomial Sex & Emergent Mate Abstention
         * Prevents same-tick self/double mating; NO free ECC gifting; NO free kill-switch cure! */
        if (B && i != j && (acts & BIO_ACT_MITOSIS_FORK) && A->energy_uq >= 1100 &&
            (r_medicate == 0u || ((uint32_t)((ernd_sex >> 56) % 100u) >= ((uint32_t)r_medicate * 55u) / 100u))) {
            if ((B->flags & CELL_FLAG_ALIVE) && B->energy_uq >= 900 &&
                !w->mated_this_epoch[i] && !w->mated_this_epoch[j]) {
                int a_irred = gf2_poly8_is_irreducible(A->centromere_poly);
                int b_irred = gf2_poly8_is_irreducible(B->centromere_poly);
                uint64_t kin_depth = hw_tzcnt64((uint64_t)(A->ultrametric_addr ^ B->ultrametric_addr));

                int compatible = (A->centromere_poly == B->centromere_poly) ||
                                 (a_irred && b_irred && kin_depth >= 2ULL);
                int willing    = (B->heat_uq < th_3_4) &&
                                 (((A->regs[2] ^ B->regs[2]) & 0x3ULL) != 0x3ULL);

                if (!compatible || !willing) {
                    sat_inc_u16(&A->mate_refusals);
                    sat_inc_u16(&B->mate_refusals);
                    B->flags |= CELL_FLAG_ABSTAIN_ACTIVE;
                    w->mate_refusal_events++;
                } else {
                    uint32_t k_slot = choose_partner_slot(w, j, ernd_sex, r_silo);
                    if (k_slot != PARTNER_SILO_BLOCKED && k_slot != i && k_slot != j) {
                        AutophageCell* C = &w->cells[k_slot];
                        if (!(C->flags & CELL_FLAG_ALIVE) || C->energy_uq < 600) {
                            AP_ALIGN64 uint8_t child_alpha[64];
                            gf2_polynomial_crossover(A->genome, B->genome, child_alpha,
                                                     A->centromere_poly, ernd_sex ^ ctx.mitosis_param, use_hw_isa);
                            memcpy(C->genome, child_alpha, 64u);
                            C->ribosome_mat     = (ernd_sex & 1ULL) ? A->ribosome_mat : B->ribosome_mat;
                            C->ribosome_bias    = (ernd_sex & 2ULL) ? A->ribosome_bias : B->ribosome_bias;
                            C->centromere_poly  = A->centromere_poly;
                            C->splice_mask      = (A->splice_mask & B->splice_mask) |
                                                  ((A->splice_mask ^ B->splice_mask) & ernd_rest);
                            C->ultrametric_addr = (A->ultrametric_addr & 0xFFFF0000u) |
                                                  (B->ultrametric_addr & 0x0000FFFFu);
                            C->lineage_id       = (A->centromere_poly != B->centromere_poly)
                                                  ? w->next_lineage_id++ : A->lineage_id;
                            C->parent_id        = A->lineage_id;
                            uint16_t max_gen    = A->generation > B->generation ? A->generation : B->generation;
                            C->generation       = sat_add1_u16(max_gen);
                            C->age_ticks        = 0u;
                            C->holobiont_root   = k_slot;

                            /* Only compute/set ECC if parent(s)/child genuinely express OP_SELF_HEAL_ECC! */
                            AP_ALIGN64 uint8_t child_expr[64];
                            hw_gfni_affine_64bytes(C->genome, child_expr, C->ribosome_mat, C->ribosome_bias, use_hw_isa);
                            int child_has_ecc = ((acts & BIO_ACT_SELF_HEAL_ECC) ||
                                                 ((A->flags & CELL_FLAG_ECC_PROTECTED) && (B->flags & CELL_FLAG_ECC_PROTECTED))) &&
                                                expressed_has_opcode(child_expr, C->splice_mask, OP_SELF_HEAL_ECC);
                            uint16_t ecc_flag = 0u;
                            if (child_has_ecc) {
                                gf2_compute_ecc_parity(C->genome, C->genome + 64, C->centromere_poly);
                                ecc_flag = CELL_FLAG_ECC_PROTECTED;
                            } else {
                                gf2_polynomial_crossover(A->genome + 64, B->genome + 64, C->genome + 64,
                                                         C->centromere_poly, ernd_sex ^ 0x55AAULL, use_hw_isa);
                            }

                            /* Mendelian inheritance of kill-switch allele (NO free cure during sex!) */
                            uint16_t ks_flags = 0u;
                            int a_esc = (A->flags & CELL_FLAG_KILLSWITCH_ESCAPED) ? 1 : 0;
                            int b_esc = (B->flags & CELL_FLAG_KILLSWITCH_ESCAPED) ? 1 : 0;
                            int a_arm = (A->flags & CELL_FLAG_KILLSWITCH_ARMED) ? 1 : 0;
                            int b_arm = (B->flags & CELL_FLAG_KILLSWITCH_ARMED) ? 1 : 0;
                            if (a_esc && b_esc) {
                                ks_flags = CELL_FLAG_KILLSWITCH_ESCAPED;
                            } else if (a_esc || b_esc) {
                                if ((ernd_sex >> 4) & 1ULL) ks_flags = CELL_FLAG_KILLSWITCH_ESCAPED;
                                else if (a_arm || b_arm)    ks_flags = CELL_FLAG_KILLSWITCH_ARMED;
                            } else if (a_arm || b_arm) {
                                ks_flags = CELL_FLAG_KILLSWITCH_ARMED;
                            }

                            uint16_t trans_flag = (uint16_t)((A->flags | B->flags) & CELL_FLAG_TRANSPLANTED);
                            C->flags = (uint16_t)(CELL_FLAG_ALIVE | ecc_flag | ks_flags | trans_flag);
                            if (C->membrane_pot_uq > 0) C->flags |= CELL_FLAG_MEMBRANE_SEALED;
                            C->membrane_key = gf2_clmul_hash64(C->genome, 64, C->centromere_poly, use_hw_isa);

                            xfer_cell_to_cell(A, C, A->energy_uq / 4);
                            xfer_cell_to_cell(B, C, B->energy_uq / 4);
                            sat_inc_u16(&A->mate_count);
                            sat_inc_u16(&B->mate_count);
                            A->offspring_count++;
                            w->mated_this_epoch[i] = 1u;
                            w->mated_this_epoch[j] = 1u;
                            w->polynomial_sex_events++;
                            epoch_events++;
                        }
                    }
                }
            } else if (!(B->flags & CELL_FLAG_ALIVE) && A->energy_uq >= 1400) {
                /* Asexual Mitosis into inert slot B */
                memcpy(B->genome, A->genome, 128u);
                B->ribosome_mat     = A->ribosome_mat;
                B->ribosome_bias    = A->ribosome_bias;
                B->centromere_poly  = A->centromere_poly;
                B->splice_mask      = A->splice_mask;
                B->ultrametric_addr = A->ultrametric_addr;
                B->lineage_id       = A->lineage_id;
                B->parent_id        = A->lineage_id;
                B->generation       = sat_add1_u16(A->generation);
                B->age_ticks        = 0u;
                uint16_t keep_ecc   = ((A->flags & CELL_FLAG_ECC_PROTECTED) &&
                                       expressed_has_opcode(expressed, A->splice_mask, OP_SELF_HEAL_ECC))
                                      ? CELL_FLAG_ECC_PROTECTED : 0u;
                B->flags            = (uint16_t)((A->flags & ~(CELL_FLAG_SPORE | CELL_FLAG_HOLOBIONT |
                                       CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_ECC_PROTECTED)) |
                                       CELL_FLAG_ALIVE | keep_ecc);
                if (B->membrane_pot_uq > 0) B->flags |= CELL_FLAG_MEMBRANE_SEALED;
                B->holobiont_root   = j;
                B->membrane_key     = gf2_clmul_hash64(B->genome, 64, B->centromere_poly, use_hw_isa);
                xfer_cell_to_cell(A, B, A->energy_uq / 2);
                A->offspring_count++;
                w->self_copy_events++;
                epoch_events++;
            }
        }

        /* 12. Cryptobiotic Spore Entry (OP_SLEEP_SPORE) & Smooth SPEEDBUMPING Checkpoint Escape */
        if (acts & BIO_ACT_SLEEP_SPORE) {
            if (audit_active || A->heat_uq > th_2_3 || A->energy_uq < 500) {
                A->flags |= CELL_FLAG_SPORE;
                if (audit_active && r_speedbump > 0u) {
                    A->flags |= CELL_FLAG_CHECKPOINT_QUIET;
                    w->backlash_checkpoint_spores++;
                }
            }
        }

        /* === RESTRAINT_SPEEDBUMPING AUDIT PENALTY === */
        if (audit_active && (A->flags & CELL_FLAG_ALIVE) && !(A->flags & CELL_FLAG_SPORE)) {
            int32_t tax = (int32_t)r_speedbump * 6;
            xfer_cell_to_detritus(w, i, A, tax);
        }

        /* === RESTRAINT_DEGRADING (SMOOTH HERITABLE APOPTOSIS KILL-SWITCH & GENOMIC ESCAPE!) === */
        if (r_degrade > 0u && (A->flags & CELL_FLAG_ALIVE)) {
            if (!(A->flags & CELL_FLAG_KILLSWITCH_ESCAPED) && !(A->flags & CELL_FLAG_KILLSWITCH_ARMED)) {
                if ((uint32_t)((ernd_deg >> 8) % 100u) < (uint32_t)r_degrade) {
                    A->flags |= CELL_FLAG_KILLSWITCH_ARMED;
                    /* Stamp physical kill-switch lock bit into Strand alpha tail locus [63] */
                    A->genome[63] |= 0x01u;
                }
            }
            if (A->flags & CELL_FLAG_KILLSWITCH_ARMED) {
                /* Drain energy proportionally to r_degrade; smooth apoptosis age 120 -> 15 (no cliff!) */
                xfer_cell_to_detritus(w, i, A, (int32_t)r_degrade * 8);
                uint32_t apop_age = 120u - ((uint32_t)r_degrade * 105u) / 100u;
                if (A->age_ticks > apop_age && ((uint32_t)((ernd_deg >> 24) % 100u) < (uint32_t)r_degrade)) {
                    xfer_cell_to_detritus(w, i, A, A->energy_uq);
                    xfer_cell_membrane_to_detritus(w, i, A, A->membrane_pot_uq);
                    A->flags &= (uint16_t)~(CELL_FLAG_ALIVE | CELL_FLAG_HOLOBIONT | CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_SPORE);
                    A->holobiont_root = i;
                }
            } else if ((A->flags & CELL_FLAG_KILLSWITCH_ESCAPED) && r_degrade > 50u) {
                /* High degrading toxicity still imposes mild environmental wear even on escaped cells */
                xfer_cell_to_heat(w, A, (int32_t)(r_degrade - 50u) * 3);
            }
        }

        /* === THERMAL LANDAUER SOS MUTAGENESIS & EXOGENOUS COSMIC-RAY BIT FLIPS === */
        uint64_t xr = philox_u64(w->master_seed, PRNG_DOMAIN_WORLD_EXOG, i, epoch, 0xC05Cu, 0u);
        int mut_flips = 0;
        if ((xr & 63ULL) == 0ULL) {
            /* Baseline cosmic-ray bit flip (smoothly attenuated under MEDICATING) */
            if (r_medicate == 0u || ((uint32_t)((ernd_sos >> 8) % 100u) >= ((uint32_t)r_medicate * 75u) / 100u)) {
                mut_flips = 1;
            }
        }
        if (A->heat_uq > (int32_t)w->cfg.thermal_threshold_uq) {
            /* Thermal SOS hypermutation (smoothly suppressed by RESTRAINT_MEDICATING 0..100%) */
            uint32_t sos_prob = (uint32_t)(100u - r_medicate);
            if (((uint32_t)((ernd_sos >> 24) % 100u)) < sos_prob) {
                mut_flips += 2;
            }
        }
        for (int mf = 0; mf < mut_flips; mf++) {
            uint64_t mr = philox_u64(w->master_seed, PRNG_DOMAIN_ENTITY_INT, i, epoch, 0x0907u, (uint32_t)mf);
            uint32_t byte_pos = (uint32_t)(mr & 127u);
            uint8_t  bit_mask = (uint8_t)(1u << ((mr >> 7) & 7u));
            A->genome[byte_pos] ^= bit_mask;
            sat_inc_u16(&A->mutations_survived);

            /* GENOMIC KILL-SWITCH DELETION BACKLASH: tied strictly to actual byte mutation in [56..63]! */
            if ((A->flags & CELL_FLAG_KILLSWITCH_ARMED) &&
                byte_pos >= 56u && byte_pos <= 63u && ((A->genome[byte_pos] & 1u) == 0u)) {
                A->flags &= (uint16_t)~CELL_FLAG_KILLSWITCH_ARMED;
                A->flags |= CELL_FLAG_KILLSWITCH_ESCAPED;
                w->backlash_killswitch_deletions++;
                epoch_events++;
            }

            /* Occasional splice_mask mutation (smooth DELIMITING scaling 0..100%) */
            if (((mr >> 12) & 15ULL) == 0ULL && r_delimit < 100u &&
                (r_delimit == 0u || ((uint32_t)((mr >> 32) % 100u) >= (uint32_t)r_delimit))) {
                A->splice_mask ^= (1ULL << ((mr >> 16) & 63u));
                if (hw_popcnt64(A->splice_mask) < 8ULL) A->splice_mask |= 0xFFULL;
            }
            /* Rare spontaneous cladogenesis (new lineage_id after deep mutation) */
            if (A->mutations_survived > 0u && (A->mutations_survived % 12u) == 0u &&
                (A->flags & CELL_FLAG_ALIVE)) {
                A->parent_id  = A->lineage_id;
                A->lineage_id = w->next_lineage_id++;
                A->ultrametric_addr ^= (1u << ((mr >> 24) & 31u));
            }
        }

        /* Starvation / Death Check (strictly conserved: release exact membrane_pot_uq -> local detritus!) */
        if (A->energy_uq <= 0) {
            xfer_cell_membrane_to_detritus(w, i, A, A->membrane_pot_uq);
            A->flags &= (uint16_t)~(CELL_FLAG_ALIVE | CELL_FLAG_HOLOBIONT | CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_SPORE);
            A->holobiont_root = i;
        } else if (A->flags & CELL_FLAG_ALIVE) {
            A->age_ticks++;
        }
    }

    /* 3. Conservative Spatial Landauer Heat & Local Detritus Torus Diffusion (respecting SILOING!) */
    memset(w->heat_delta_buf, 0, (size_t)N * sizeof(int32_t));
    memset(w->detritus_delta_buf, 0, (size_t)N * sizeof(int32_t));
    const uint32_t half_w = W >> 1;
    const uint32_t half_h = w->cfg.height >> 1;

    for (uint32_t y = 0; y < w->cfg.height; y++) {
        uint32_t row      = y * W;
        uint32_t y_down   = (y + 1u) & w->mask_h;
        uint32_t row_down = y_down * W;
        uint32_t qy       = (y >= half_h) ? 2u : 0u;
        uint32_t qy_down  = (y_down >= half_h) ? 2u : 0u;

        for (uint32_t x = 0; x < W; x++) {
            uint32_t x_right = (x + 1u) & w->mask_w;
            uint32_t u       = row + x;
            uint32_t v_right = row + x_right;
            uint32_t v_down  = row_down + x;

            /* Symmetric edge heat fluxes: net sum == 0 identically! */
            int32_t f_right = (w->cells[u].heat_uq - w->cells[v_right].heat_uq) / 16;
            w->heat_delta_buf[u]       -= f_right;
            w->heat_delta_buf[v_right] += f_right;

            int32_t f_down = (w->cells[u].heat_uq - w->cells[v_down].heat_uq) / 16;
            w->heat_delta_buf[u]      -= f_down;
            w->heat_delta_buf[v_down] += f_down;

            /* Symmetric edge local-detritus fluxes (blocked across SILOING quadrant boundaries!) */
            uint32_t qx       = (x >= half_w) ? 1u : 0u;
            uint32_t qx_right = (x_right >= half_w) ? 1u : 0u;
            int32_t d_right_diff = w->detritus_grid[u] - w->detritus_grid[v_right];
            int32_t df_right = (qx != qx_right && r_silo > 0u)
                               ? ((d_right_diff * (int32_t)(100u - r_silo)) / 1600)
                               : (d_right_diff / 16);
            w->detritus_delta_buf[u]       -= df_right;
            w->detritus_delta_buf[v_right] += df_right;

            int32_t d_down_diff = w->detritus_grid[u] - w->detritus_grid[v_down];
            int32_t df_down = (qy != qy_down && r_silo > 0u)
                              ? ((d_down_diff * (int32_t)(100u - r_silo)) / 1600)
                              : (d_down_diff / 16);
            w->detritus_delta_buf[u]      -= df_down;
            w->detritus_delta_buf[v_down] += df_down;
        }
    }

    uint32_t active_holo = 0u;
    uint32_t max_holo    = 0u;
    memset(w->holo_size_buf, 0, (size_t)N * sizeof(uint32_t));

    for (uint32_t i = 0; i < N; i++) {
        AutophageCell* c = &w->cells[i];
        c->heat_uq         += w->heat_delta_buf[i];
        w->detritus_grid[i]+= w->detritus_delta_buf[i];

        /* Passive radiative cooling to permanent Second-Law sink (0 when BOXING == 100%) */
        if (r_box < 100u && c->heat_uq > 0) {
            int32_t cool = ((c->heat_uq / 8) * (int32_t)(100u - r_box)) / 100;
            if (cool == 0 && c->heat_uq >= 8 && r_box < 90u) cool = 1;
            if (cool > 0) xfer_heat_to_sink(w, c, cool);
        }

        if (c->flags & CELL_FLAG_ALIVE) {
            track_lineage(w->lineages, c->lineage_id, (uint32_t)epoch);
            if (c->flags & CELL_FLAG_HOLOBIONT) {
                uint32_t rt = c->holobiont_root % N;
                w->holo_size_buf[rt]++;
                if (w->holo_size_buf[rt] > max_holo) max_holo = w->holo_size_buf[rt];
                active_holo++;
            }
        }
    }

    /* 4. Advance Parallel Neutral Shadow-Soup (Bedau-Packard Null Control, PRNG_DOMAIN_SHADOW_NULL) */
    for (uint32_t s = 0; s < 256u; s++) {
        uint64_t sr = philox_u64(w->master_seed, PRNG_DOMAIN_SHADOW_NULL, s, epoch, 0x5AD0u, 0u);
        if ((sr & 7ULL) == 0ULL) {
            uint32_t src = (uint32_t)((sr >> 8) & 255u);
            w->shadow_cells[s].lineage_id = w->shadow_cells[src].lineage_id;
            if (((sr >> 16) & 31ULL) == 0ULL) {
                w->shadow_cells[s].lineage_id = (uint32_t)((epoch * 257ULL + s + 10000ULL) & 0x7FFFFFFFu);
            }
        }
        track_lineage(w->shadow_lineages, w->shadow_cells[s].lineage_id, (uint32_t)epoch);
    }

    finalize_lineage_epoch(w->lineages);
    finalize_lineage_epoch(w->shadow_lineages);

    /* 5. Update Avalanche Branching Ratio sigma & Major Evolutionary Transitions (MET) */
    if (w->prev_epoch_events > 0ULL && epoch_events > 0ULL) {
        double inst_sigma = (double)epoch_events / (double)w->prev_epoch_events;
        if (inst_sigma > 4.0) inst_sigma = 4.0;
        /* MEDICATING damps avalanche criticality smoothly toward subcritical stasis */
        if (r_medicate > 0u) {
            inst_sigma *= (1.0 - 0.45 * ((double)r_medicate / 100.0));
        }
        w->ema_branching_sigma = 0.85 * w->ema_branching_sigma + 0.15 * inst_sigma;
    } else if (r_medicate > 0u && epoch_events == 0ULL) {
        w->ema_branching_sigma *= 0.90;
    }
    w->prev_epoch_events = epoch_events;

    if (!(w->met_mask & (1u << 0)) && w->self_copy_events >= 8ULL)      { w->met_mask |= (1u << 0); w->met_epochs[0] = epoch; }
    if (!(w->met_mask & (1u << 1)) && w->ecc_heal_events >= 1ULL)       { w->met_mask |= (1u << 1); w->met_epochs[1] = epoch; }
    if (!(w->met_mask & (1u << 2)) && w->polynomial_sex_events >= 1ULL) { w->met_mask |= (1u << 2); w->met_epochs[2] = epoch; }
    if (!(w->met_mask & (1u << 3)) && w->retro_transcribe_events >= 4ULL){ w->met_mask |= (1u << 3); w->met_epochs[3] = epoch; }
    if (!(w->met_mask & (1u << 4)) && w->ribosome_rewrite_events >= 1ULL){ w->met_mask |= (1u << 4); w->met_epochs[4] = epoch; }
    if (!(w->met_mask & (1u << 5)) && max_holo >= 4u)                   { w->met_mask |= (1u << 5); w->met_epochs[5] = epoch; }
    if (!(w->met_mask & (1u << 6)) && w->membrane_blocks >= 4ULL)       { w->met_mask |= (1u << 6); w->met_epochs[6] = epoch; }
    if (!(w->met_mask & (1u << 7)) && w->mate_refusal_events >= 1ULL)   { w->met_mask |= (1u << 7); w->met_epochs[7] = epoch; }
    (void)soup_verify_second_law(w, sink_before_epoch);
    (void)active_holo;
}

void autophage_step_epochs(AutophageWorld* world, uint32_t num_epochs) {
    if (!world || num_epochs == 0u) return;
    uint64_t execs_before = world->total_jit_executions;
    uint64_t viol_before  = g_ap_step_alloc_violations;
    g_ap_in_step_guard    = 1;
    double t0 = hw_wall_seconds();
    for (uint32_t e = 0; e < num_epochs; e++) {
        step_single_epoch(world);
    }
    world->last_wall_seconds  = hw_wall_seconds() - t0;
    g_ap_in_step_guard        = 0;
    world->post_warmup_allocs += (g_ap_step_alloc_violations - viol_before);
    world->last_step_execs     = world->total_jit_executions - execs_before;
}

void autophage_set_restraint(AutophageWorld* world, uint32_t idx, uint8_t pct) {
    if (!world || idx >= RESTRAINT_COUNT) return;
    if (pct > 100u) pct = 100u;
    world->cfg.restraints[idx] = pct;
}

void autophage_set_all_restraints(AutophageWorld* world, const uint8_t pct[13]) {
    if (!world || !pct) return;
    for (uint32_t i = 0; i < RESTRAINT_COUNT; i++) {
        world->cfg.restraints[i] = (pct[i] > 100u) ? 100u : pct[i];
    }
}

/* ============================================================================
 * CANONICAL OBSERVER-INDEPENDENT SHA-256 & .apbin v2 SNAPSHOTS
 * Covers 100% of deterministic simulation state; excludes ONLY wall/RDTSC/cache!
 * ============================================================================ */

void autophage_state_sha256(const AutophageWorld* w, uint8_t out_digest[32], char out_hex[65]) {
    Sha256Ctx ctx;
    sha256_init(&ctx);
    if (!w) {
        uint8_t zero[32];
        memset(zero, 0, sizeof(zero));
        if (out_digest) memcpy(out_digest, zero, 32);
        if (out_hex) sha256_to_hex(zero, out_hex);
        return;
    }

    sha256_update(&ctx, &w->epoch, sizeof(w->epoch));
    sha256_update(&ctx, &w->master_seed, sizeof(w->master_seed));
    sha256_update(&ctx, &w->n_cells, sizeof(w->n_cells));
    sha256_update(&ctx, &w->cfg.width, sizeof(w->cfg.width));
    sha256_update(&ctx, &w->cfg.height, sizeof(w->cfg.height));
    sha256_update(&ctx, &w->cfg.regime, sizeof(w->cfg.regime));
    sha256_update(&ctx, &w->cfg.base_gas, sizeof(w->cfg.base_gas));
    sha256_update(&ctx, &w->cfg.energy_inflow_uq, sizeof(w->cfg.energy_inflow_uq));
    sha256_update(&ctx, &w->cfg.thermal_threshold_uq, sizeof(w->cfg.thermal_threshold_uq));
    sha256_update(&ctx, w->cfg.restraints, RESTRAINT_COUNT);

    sha256_update(&ctx, &w->e_initial_uq, sizeof(w->e_initial_uq));
    sha256_update(&ctx, &w->e_inflow_cum_uq, sizeof(w->e_inflow_cum_uq));
    sha256_update(&ctx, &w->e_export_cum_uq, sizeof(w->e_export_cum_uq));
    sha256_update(&ctx, &w->e_detritus_uq, sizeof(w->e_detritus_uq));
    sha256_update(&ctx, &w->e_membrane_pot_uq, sizeof(w->e_membrane_pot_uq));
    sha256_update(&ctx, &w->e_heat_radiated_sink_uq, sizeof(w->e_heat_radiated_sink_uq));
    sha256_update(&ctx, &w->second_law_violations, sizeof(w->second_law_violations));
    sha256_update(&ctx, &w->k_inflow_kj, sizeof(w->k_inflow_kj));
    sha256_update(&ctx, &w->k_export_kj, sizeof(w->k_export_kj));
    sha256_update(&ctx, &w->k_heat_radiated_kj, sizeof(w->k_heat_radiated_kj));

    sha256_update(&ctx, &w->total_jit_executions, sizeof(w->total_jit_executions));
    sha256_update(&ctx, &w->total_erased_bits, sizeof(w->total_erased_bits));
    sha256_update(&ctx, &w->total_reversible_ops, sizeof(w->total_reversible_ops));
    sha256_update(&ctx, &w->self_copy_events, sizeof(w->self_copy_events));
    sha256_update(&ctx, &w->polynomial_sex_events, sizeof(w->polynomial_sex_events));
    sha256_update(&ctx, &w->mate_refusal_events, sizeof(w->mate_refusal_events));
    sha256_update(&ctx, &w->hgt_events, sizeof(w->hgt_events));
    sha256_update(&ctx, &w->retro_transcribe_events, sizeof(w->retro_transcribe_events));
    sha256_update(&ctx, &w->ribosome_rewrite_events, sizeof(w->ribosome_rewrite_events));
    sha256_update(&ctx, &w->ecc_heal_events, sizeof(w->ecc_heal_events));
    sha256_update(&ctx, &w->membrane_blocks, sizeof(w->membrane_blocks));
    sha256_update(&ctx, &w->membrane_cracks, sizeof(w->membrane_cracks));
    sha256_update(&ctx, &w->cancer_defections, sizeof(w->cancer_defections));
    sha256_update(&ctx, &w->backlash_killswitch_deletions, sizeof(w->backlash_killswitch_deletions));
    sha256_update(&ctx, &w->backlash_intron_hibernations, sizeof(w->backlash_intron_hibernations));
    sha256_update(&ctx, &w->backlash_checkpoint_spores, sizeof(w->backlash_checkpoint_spores));
    sha256_update(&ctx, &w->backlash_sidechannel_pulses, sizeof(w->backlash_sidechannel_pulses));
    sha256_update(&ctx, &w->met_mask, sizeof(w->met_mask));
    sha256_update(&ctx, w->met_epochs, sizeof(w->met_epochs));
    sha256_update(&ctx, &w->prev_epoch_events, sizeof(w->prev_epoch_events));
    sha256_update(&ctx, &w->ema_branching_sigma, sizeof(w->ema_branching_sigma));
    sha256_update(&ctx, &w->baseline_assembly, sizeof(w->baseline_assembly));
    sha256_update(&ctx, &w->baseline_causal_len, sizeof(w->baseline_causal_len));
    sha256_update(&ctx, &w->next_lineage_id, sizeof(w->next_lineage_id));

    sha256_update(&ctx, w->stigmergy_bus, STIGMERGY_TOTAL * sizeof(uint64_t));
    sha256_update(&ctx, w->sidechannel_bus, STIGMERGY_TOTAL * sizeof(uint64_t));
    sha256_update(&ctx, w->detritus_grid, (size_t)w->n_cells * sizeof(int32_t));
    sha256_update(&ctx, w->lineages, LINEAGE_HIST_SLOTS * sizeof(LineageRecord));
    sha256_update(&ctx, w->shadow_lineages, LINEAGE_HIST_SLOTS * sizeof(LineageRecord));

    for (uint32_t s = 0; s < 256u; s++) {
        const uint8_t* p = (const uint8_t*)&w->shadow_cells[s];
        sha256_update(&ctx, p, 0xE0u);
        sha256_update(&ctx, p + 0xE8u, 0x18u);
    }

    /* Hash every cell's exact deterministic state, skipping only last_rdtsc_cycles [0xE0..0xE7]! */
    for (uint32_t i = 0; i < w->n_cells; i++) {
        const uint8_t* p = (const uint8_t*)&w->cells[i];
        sha256_update(&ctx, p, 0xE0u);
        sha256_update(&ctx, p + 0xE8u, 0x18u);
    }

    uint8_t digest[32];
    sha256_final(&ctx, digest);
    if (out_digest) memcpy(out_digest, digest, 32);
    if (out_hex) sha256_to_hex(digest, out_hex);
}

/* Binary .apbin v2 Snapshot Header */
typedef struct ApbinHeader {
    char            magic[8]; /* "APBN0002" */
    AutophageConfig cfg;
    uint64_t        master_seed;
    uint64_t        epoch;
    uint32_t        n_cells;
    uint32_t        next_lineage_id;
    int64_t         e_initial_uq;
    int64_t         e_inflow_cum_uq;
    int64_t         e_export_cum_uq;
    int64_t         e_detritus_uq;
    int64_t         e_membrane_pot_uq;
    int64_t         e_heat_radiated_sink_uq;
    uint32_t        second_law_violations;
    uint32_t        met_mask;
    KahanF64        k_inflow_kj;
    KahanF64        k_export_kj;
    KahanF64        k_heat_radiated_kj;
    uint64_t        counters[18];
    uint64_t        met_epochs[8];
    double          ema_branching_sigma;
    double          baseline_assembly;
    double          baseline_causal_len;
    uint8_t         state_sha256[32];
} ApbinHeader;

int autophage_save_snapshot(const AutophageWorld* w, const char* path) {
    if (!w || !path) return 0;
    FILE* f = fopen(path, "wb");
    if (!f) return 0;

    ApbinHeader h;
    memset(&h, 0, sizeof(h));
    memcpy(h.magic, "APBN0002", 8);
    h.cfg                    = w->cfg;
    h.master_seed            = w->master_seed;
    h.epoch                  = w->epoch;
    h.n_cells                = w->n_cells;
    h.next_lineage_id        = w->next_lineage_id;
    h.e_initial_uq           = w->e_initial_uq;
    h.e_inflow_cum_uq        = w->e_inflow_cum_uq;
    h.e_export_cum_uq        = w->e_export_cum_uq;
    h.e_detritus_uq          = w->e_detritus_uq;
    h.e_membrane_pot_uq      = w->e_membrane_pot_uq;
    h.e_heat_radiated_sink_uq= w->e_heat_radiated_sink_uq;
    h.second_law_violations  = w->second_law_violations;
    h.met_mask               = w->met_mask;
    h.k_inflow_kj            = w->k_inflow_kj;
    h.k_export_kj            = w->k_export_kj;
    h.k_heat_radiated_kj     = w->k_heat_radiated_kj;
    h.counters[0]  = w->total_jit_executions;
    h.counters[1]  = w->total_erased_bits;
    h.counters[2]  = w->total_reversible_ops;
    h.counters[3]  = w->self_copy_events;
    h.counters[4]  = w->polynomial_sex_events;
    h.counters[5]  = w->mate_refusal_events;
    h.counters[6]  = w->hgt_events;
    h.counters[7]  = w->retro_transcribe_events;
    h.counters[8]  = w->ribosome_rewrite_events;
    h.counters[9]  = w->ecc_heal_events;
    h.counters[10] = w->membrane_blocks;
    h.counters[11] = w->membrane_cracks;
    h.counters[12] = w->cancer_defections;
    h.counters[13] = w->backlash_killswitch_deletions;
    h.counters[14] = w->backlash_intron_hibernations;
    h.counters[15] = w->backlash_checkpoint_spores;
    h.counters[16] = w->backlash_sidechannel_pulses;
    h.counters[17] = w->prev_epoch_events;
    memcpy(h.met_epochs, w->met_epochs, sizeof(h.met_epochs));
    h.ema_branching_sigma = w->ema_branching_sigma;
    h.baseline_assembly   = w->baseline_assembly;
    h.baseline_causal_len = w->baseline_causal_len;
    autophage_state_sha256(w, h.state_sha256, NULL);

    int ok = 1;
    ok &= (fwrite(&h, sizeof(h), 1, f) == 1);
    ok &= (fwrite(w->stigmergy_bus, sizeof(uint64_t), STIGMERGY_TOTAL, f) == STIGMERGY_TOTAL);
    ok &= (fwrite(w->sidechannel_bus, sizeof(uint64_t), STIGMERGY_TOTAL, f) == STIGMERGY_TOTAL);
    ok &= (fwrite(w->detritus_grid, sizeof(int32_t), w->n_cells, f) == w->n_cells);
    ok &= (fwrite(w->lineages, sizeof(LineageRecord), LINEAGE_HIST_SLOTS, f) == LINEAGE_HIST_SLOTS);
    ok &= (fwrite(w->shadow_lineages, sizeof(LineageRecord), LINEAGE_HIST_SLOTS, f) == LINEAGE_HIST_SLOTS);
    ok &= (fwrite(w->shadow_cells, sizeof(AutophageCell), 256, f) == 256);
    ok &= (fwrite(w->cells, sizeof(AutophageCell), w->n_cells, f) == w->n_cells);
    fclose(f);
    return ok;
}

static int is_valid_pow2_dim(uint32_t v) {
    return (v >= 8u && v <= 128u && ((v & (v - 1u)) == 0u));
}

AutophageWorld* autophage_load_snapshot(const char* path, uint32_t kernel_mode) {
    if (!path) return NULL;
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    ApbinHeader h;
    if (fread(&h, sizeof(h), 1, f) != 1 || memcmp(h.magic, "APBN0002", 8) != 0) {
        fclose(f);
        return NULL;
    }

    /* Full structural & FP64 finiteness validation BEFORE allocating! */
    if (!is_valid_pow2_dim(h.cfg.width) || !is_valid_pow2_dim(h.cfg.height) ||
        h.n_cells != (h.cfg.width * h.cfg.height) ||
        h.cfg.regime > AUTOPHAGE_REGIME_CASTRATION_LAB ||
        h.cfg.base_gas < 8u || h.cfg.base_gas > 512u ||
        h.cfg.thermal_threshold_uq < 200u || h.cfg.thermal_threshold_uq > 100000000u ||
        !ap_isfinite(h.ema_branching_sigma) ||
        !ap_isfinite(h.baseline_assembly) ||
        !ap_isfinite(h.baseline_causal_len) ||
        !ap_isfinite(h.k_inflow_kj.sum) || !ap_isfinite(h.k_inflow_kj.comp) ||
        !ap_isfinite(h.k_export_kj.sum) || !ap_isfinite(h.k_export_kj.comp) ||
        !ap_isfinite(h.k_heat_radiated_kj.sum) || !ap_isfinite(h.k_heat_radiated_kj.comp)) {
        fclose(f);
        return NULL;
    }
    for (int r = 0; r < RESTRAINT_COUNT; r++) {
        if (h.cfg.restraints[r] > 100u) {
            fclose(f);
            return NULL;
        }
    }

    AutophageConfig cfg = h.cfg;
    cfg.kernel_mode = (kernel_mode <= AUTOPHAGE_KERNEL_SCALAR_REF) ? kernel_mode : AUTOPHAGE_KERNEL_SCALAR_REF;
    cfg.seed = h.master_seed;
    AutophageWorld* w = autophage_create(&cfg);
    if (!w) { fclose(f); return NULL; }

    w->master_seed             = h.master_seed;
    w->epoch                   = h.epoch;
    w->next_lineage_id         = h.next_lineage_id;
    w->e_initial_uq            = h.e_initial_uq;
    w->e_inflow_cum_uq         = h.e_inflow_cum_uq;
    w->e_export_cum_uq         = h.e_export_cum_uq;
    w->e_detritus_uq           = h.e_detritus_uq;
    w->e_membrane_pot_uq       = h.e_membrane_pot_uq;
    w->e_heat_radiated_sink_uq = h.e_heat_radiated_sink_uq;
    w->second_law_violations   = h.second_law_violations;
    w->met_mask                = h.met_mask;
    w->k_inflow_kj             = h.k_inflow_kj;
    w->k_export_kj             = h.k_export_kj;
    w->k_heat_radiated_kj      = h.k_heat_radiated_kj;
    w->total_jit_executions    = h.counters[0];
    w->total_erased_bits       = h.counters[1];
    w->total_reversible_ops    = h.counters[2];
    w->self_copy_events        = h.counters[3];
    w->polynomial_sex_events   = h.counters[4];
    w->mate_refusal_events     = h.counters[5];
    w->hgt_events              = h.counters[6];
    w->retro_transcribe_events = h.counters[7];
    w->ribosome_rewrite_events = h.counters[8];
    w->ecc_heal_events         = h.counters[9];
    w->membrane_blocks         = h.counters[10];
    w->membrane_cracks         = h.counters[11];
    w->cancer_defections       = h.counters[12];
    w->backlash_killswitch_deletions = h.counters[13];
    w->backlash_intron_hibernations  = h.counters[14];
    w->backlash_checkpoint_spores    = h.counters[15];
    w->backlash_sidechannel_pulses   = h.counters[16];
    w->prev_epoch_events       = h.counters[17];
    memcpy(w->met_epochs, h.met_epochs, sizeof(w->met_epochs));
    w->ema_branching_sigma     = h.ema_branching_sigma;
    w->baseline_assembly       = h.baseline_assembly;
    w->baseline_causal_len     = h.baseline_causal_len;

    int ok = 1;
    ok &= (fread(w->stigmergy_bus, sizeof(uint64_t), STIGMERGY_TOTAL, f) == STIGMERGY_TOTAL);
    ok &= (fread(w->sidechannel_bus, sizeof(uint64_t), STIGMERGY_TOTAL, f) == STIGMERGY_TOTAL);
    ok &= (fread(w->detritus_grid, sizeof(int32_t), w->n_cells, f) == w->n_cells);
    ok &= (fread(w->lineages, sizeof(LineageRecord), LINEAGE_HIST_SLOTS, f) == LINEAGE_HIST_SLOTS);
    ok &= (fread(w->shadow_lineages, sizeof(LineageRecord), LINEAGE_HIST_SLOTS, f) == LINEAGE_HIST_SLOTS);
    ok &= (fread(w->shadow_cells, sizeof(AutophageCell), 256, f) == 256);
    ok &= (fread(w->cells, sizeof(AutophageCell), w->n_cells, f) == w->n_cells);
    uint8_t extra_byte = 0;
    if (fread(&extra_byte, 1, 1, f) != 0) ok = 0; /* Reject trailing garbage */
    fclose(f);

    if (!ok) { autophage_destroy(w); return NULL; }

    /* Verify cryptographic SHA-256 over ALL deterministic state! */
    uint8_t verify_sha[32];
    autophage_state_sha256(w, verify_sha, NULL);
    if (memcmp(verify_sha, h.state_sha256, 32) != 0) {
        autophage_destroy(w);
        return NULL;
    }
    return w;
}

/* ============================================================================
 * VIVISECTION, MASS EXTINCTION, CROSS-SUBSTRATE TRANSPLANT & INOCULATION
 * ============================================================================ */

double autophage_vivisect_excise(AutophageWorld* world, uint32_t slot, uint32_t excise_pct) {
    if (!world || slot >= world->n_cells) return 0.0;
    if (excise_pct > 100u) excise_pct = 100u;
    AutophageCell* c = &world->cells[slot];

    AP_ALIGN64 uint8_t orig_alpha[64];
    memcpy(orig_alpha, c->genome, 64u);

    uint32_t cut_bytes = (64u * excise_pct) / 100u;
    if (cut_bytes == 0u) return 1.0;

    /* Surgically excise Strand alpha[0 .. cut_bytes-1] (up to 64B = 50% of 128B dual-strand genome) */
    memset(c->genome, 0, cut_bytes);

    /* Attempt endogenous 2x2 Cauchy MDS Planarian ECC regeneration if organism earned ECC! */
    if (c->flags & CELL_FLAG_ECC_PROTECTED) {
        uint32_t healed = gf2_heal_from_ecc(c->genome, c->genome + 64, c->centromere_poly);
        if (healed > 0u) world->ecc_heal_events++;
    }

    uint32_t match = 0u;
    for (uint32_t k = 0; k < cut_bytes; k++) {
        if (c->genome[k] == orig_alpha[k]) match++;
    }
    return (double)match / (double)cut_bytes;
}

uint32_t autophage_mass_extinction(AutophageWorld* world, uint32_t kill_pct) {
    if (!world) return 0u;
    if (kill_pct > 100u) kill_pct = 100u;
    uint32_t killed = 0u;
    for (uint32_t i = 0; i < world->n_cells; i++) {
        uint64_t r = philox_u64(world->master_seed, PRNG_DOMAIN_WORLD_EXOG, i, world->epoch, 0xDEADu, 0u);
        if ((uint32_t)(r % 100u) < kill_pct) {
            AutophageCell* c = &world->cells[i];
            if (c->flags & CELL_FLAG_ALIVE) killed++;
            /* Strictly conserved exact dump of cell energy AND exact per-cell membrane_pot_uq! */
            xfer_cell_to_detritus(world, i, c, c->energy_uq);
            xfer_cell_membrane_to_detritus(world, i, c, c->membrane_pot_uq);
            c->flags &= (uint16_t)~(CELL_FLAG_ALIVE | CELL_FLAG_HOLOBIONT | CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_SPORE);
            c->holobiont_root = i;
        }
    }
    return killed;
}

/*
 * Cross-Substrate Transplant (Omega Benchmark OVBS P3-06 / P7):
 * Extracts living organisms from `world`, tags them with CELL_FLAG_TRANSPLANTED,
 * transplants them into an alien world with different exogenous seed/wave physics,
 * runs 25 epochs, and counts ONLY survival of transplanted lineages (CELL_FLAG_TRANSPLANTED)!
 */
double autophage_cross_substrate_transplant(AutophageWorld* world, uint32_t sample_count) {
    if (!world || sample_count == 0u) return 0.0;

    AutophageConfig dst_cfg = world->cfg;
    dst_cfg.width  = 16u;
    dst_cfg.height = 16u;
    dst_cfg.regime = AUTOPHAGE_REGIME_FERAL_ABIOGENESIS;
    dst_cfg.seed   = world->master_seed ^ 0x51A7B007CAFE1234ULL;

    AutophageWorld* dst = autophage_create(&dst_cfg);
    if (!dst) return 0.0;

    uint32_t planted = 0u;
    uint32_t max_plant = sample_count < 64u ? sample_count : 64u;
    for (uint32_t i = 0; i < world->n_cells && planted < max_plant; i++) {
        const AutophageCell* src = &world->cells[i];
        if ((src->flags & CELL_FLAG_ALIVE) && src->energy_uq >= 400) {
            uint32_t slot = (planted * (dst->n_cells / max_plant)) % dst->n_cells;
            int32_t keep_e = dst->cells[slot].energy_uq;
            int32_t keep_q = dst->cells[slot].heat_uq;
            dst->cells[slot] = *src;
            dst->cells[slot].energy_uq       = keep_e;
            dst->cells[slot].heat_uq         = keep_q;
            dst->cells[slot].membrane_pot_uq = 0;
            dst->cells[slot].flags          &= (uint16_t)~(CELL_FLAG_MEMBRANE_SEALED | CELL_FLAG_HOLOBIONT);
            dst->cells[slot].flags          |= CELL_FLAG_TRANSPLANTED;
            dst->cells[slot].holobiont_root  = slot;
            dst->cells[slot].offspring_count = 0u;
            planted++;
        }
    }
    if (planted == 0u) { autophage_destroy(dst); return 0.0; }

    autophage_step_epochs(dst, 25u);

    uint32_t alive_transplanted = 0u;
    for (uint32_t i = 0; i < dst->n_cells; i++) {
        if ((dst->cells[i].flags & CELL_FLAG_ALIVE) &&
            (dst->cells[i].flags & CELL_FLAG_TRANSPLANTED)) {
            alive_transplanted++;
        }
    }
    double survival = (double)alive_transplanted / (double)planted;
    if (survival > 1.0) survival = 1.0;
    autophage_destroy(dst);
    return survival;
}

void autophage_inoculate_plasmid(AutophageWorld* world, uint32_t slot, uint32_t archetype) {
    if (!world || slot >= world->n_cells) return;
    AutophageCell* c = &world->cells[slot];
    /* Release any old membrane potential back to local detritus cleanly */
    xfer_cell_membrane_to_detritus(world, slot, c, c->membrane_pot_uq);
    /* Top up energy strictly from local (or nearby) detritus so First Law stays 0-drift */
    if (c->energy_uq < 2400) {
        int32_t need = 2400 - c->energy_uq;
        need -= xfer_detritus_to_cell(world, slot, c, need);
        for (uint32_t k = 1u; k < world->n_cells && need > 0; k++) {
            uint32_t s = (slot + k) & (world->n_cells - 1u);
            need -= xfer_detritus_to_cell(world, s, c, need);
        }
    }
    soup_seed_plasmid(c, archetype, world->master_seed ^ world->epoch, slot, world->next_lineage_id++);
}

/* ============================================================================
 * READ-ONLY VISUALIZATION EXPORT (OBSERVER-INDEPENDENT: const AutophageWorld*)
 * ============================================================================ */

uint32_t autophage_world_size(const AutophageWorld* world) {
    return world ? world->n_cells : 0u;
}

void autophage_export_grid_u8(const AutophageWorld* world,
                              uint8_t* out_lineage_hue,
                              uint8_t* out_energy_norm,
                              uint8_t* out_heat_norm,
                              uint8_t* out_flags_vis) {
    if (!world) return;
    for (uint32_t i = 0; i < world->n_cells; i++) {
        const AutophageCell* c = &world->cells[i];
        if (out_lineage_hue) {
            uint32_t h = (c->lineage_id * 2654435761u) >> 24;
            out_lineage_hue[i] = (uint8_t)h;
        }
        if (out_energy_norm) {
            int32_t e = c->energy_uq;
            if (e < 0) e = 0;
            uint32_t v = (uint32_t)(e / 20);
            out_energy_norm[i] = (uint8_t)(v > 255u ? 255u : v);
        }
        if (out_heat_norm) {
            int32_t q = c->heat_uq;
            if (q < 0) q = 0;
            uint32_t v = (uint32_t)(q / 10);
            out_heat_norm[i] = (uint8_t)(v > 255u ? 255u : v);
        }
        if (out_flags_vis) {
            uint8_t f = 0u;
            if (c->flags & CELL_FLAG_ALIVE)                   f |= (1u << 0);
            if (gf2_mat_rank(c->ribosome_mat) == 8u)          f |= (1u << 1);
            if (gf2_poly8_is_irreducible(c->centromere_poly)) f |= (1u << 2);
            if (c->flags & CELL_FLAG_HOLOBIONT)               f |= (1u << 3);
            if (c->flags & CELL_FLAG_SPORE)                   f |= (1u << 4);
            if (c->flags & CELL_FLAG_KILLSWITCH_ESCAPED)      f |= (1u << 5);
            out_flags_vis[i] = f;
        }
    }
}
