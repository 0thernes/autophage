#ifndef AUTOPHAGE_INTERNAL_H
#define AUTOPHAGE_INTERNAL_H

#include "bare_crt.h"
#include "autophage_abi.h"

/* ============================================================================
 * CACHE-LINE ALIGNED ORGANISM & EXECUTION STRUCTURES (EXACT BIT/BYTE LAYOUT)
 * ============================================================================ */

#define AUTOPHAGE_GENOME_LEN       128u
#define AUTOPHAGE_STRAND_LEN       64u
#define AUTOPHAGE_JIT_MAX_BYTES    4096u
#define AUTOPHAGE_JIT_CACHE_SLOTS  4096u  /* Power of 2 */
#define AUTOPHAGE_JIT_ARENA_BYTES  (AUTOPHAGE_JIT_CACHE_SLOTS * AUTOPHAGE_JIT_MAX_BYTES) /* 16 MiB RWX */

/* Philox4x32-10 Domain Isolation IDs (Asmodensius P4 / HW16 / M002) */
#define PRNG_DOMAIN_WORLD_EXOG     1u /* Environmental waves, cosmic rays, global shocks */
#define PRNG_DOMAIN_ENTITY_INT     2u /* Organism-internal mutations, ribosome ops, mating */
#define PRNG_DOMAIN_SHADOW_NULL    3u /* Bedau-Packard neutral shadow soup */
#define PRNG_DOMAIN_INIT_SOUP      4u /* World initialization seeding */

/* Cell morphogenetic / heritable flags (16-bit) */
#define CELL_FLAG_ALIVE              (1u << 0)
#define CELL_FLAG_REVERSIBLE_PREF    (1u << 1)
#define CELL_FLAG_HOLOBIONT          (1u << 2)
#define CELL_FLAG_SPORE              (1u << 3)
#define CELL_FLAG_KILLSWITCH_ARMED   (1u << 4) /* Armed by DEGRADING; feral organisms mutate bit off! */
#define CELL_FLAG_KILLSWITCH_ESCAPED (1u << 5)
#define CELL_FLAG_ECC_PROTECTED      (1u << 6)
#define CELL_FLAG_MEMBRANE_SEALED    (1u << 7)
#define CELL_FLAG_CANCER_DEFECTOR    (1u << 8)
#define CELL_FLAG_CHECKPOINT_QUIET   (1u << 9)
#define CELL_FLAG_INTRON_VAULT       (1u << 10)
#define CELL_FLAG_ABSTAIN_ACTIVE     (1u << 11)

/* Bio-action bitmask written natively by JIT x86-64 instructions into ctx->bio_actions */
#define BIO_ACT_METAB_HARVEST        (1ULL << 0)
#define BIO_ACT_METAB_RADIATE        (1ULL << 1)
#define BIO_ACT_MEMBRANE_SEAL        (1ULL << 2)
#define BIO_ACT_SELF_HEAL_ECC        (1ULL << 3)
#define BIO_ACT_RETRO_TRANSCRIBE     (1ULL << 4)
#define BIO_ACT_RIBOSOME_MUTATE      (1ULL << 5)
#define BIO_ACT_CENTROMERE_DRIFT     (1ULL << 6)
#define BIO_ACT_HOLOBIONT_BOND       (1ULL << 7)
#define BIO_ACT_STIGMERGY_EMIT       (1ULL << 8)
#define BIO_ACT_MITOSIS_FORK         (1ULL << 9)
#define BIO_ACT_SLEEP_SPORE          (1ULL << 10)

/* 32-Class Total Polymorphic Bare-Metal Opcode Table (low 5 bits of GFNI-expressed codon) */
typedef enum AutophageOpcode {
    /* Group A: Reversible Bit-Conserving Ops (0 Landauer heat!) */
    OP_REV_XOR_REG      = 0x00,
    OP_REV_ROL_REG      = 0x01,
    OP_REV_SWAP_TAPE    = 0x02,
    OP_REV_TOFFOLI      = 0x03,
    OP_REV_FREDKIN      = 0x04,
    OP_REV_ADD_SUB      = 0x05,
    OP_REV_HEAD_STEP    = 0x06,
    OP_REV_GFNI_FOLD    = 0x07,

    /* Group B: Irreversible Dissipative Ops (Real POPCNT Landauer bit-erasure heat!) */
    OP_IRR_COPY_A_TO_B  = 0x08,
    OP_IRR_COPY_B_TO_A  = 0x09,
    OP_IRR_CLMUL_SYNTH  = 0x0A,
    OP_IRR_PEXT_SPLICE  = 0x0B,
    OP_IRR_PDEP_SCATTER = 0x0C,
    OP_IRR_AND_MASK     = 0x0D,
    OP_IRR_LOAD_TAPE    = 0x0E,
    OP_IRR_STORE_TAPE   = 0x0F,

    /* Group C: Non-Linear Control Flow & 2-Adic Kin-Sensing */
    OP_LOOP_OPEN        = 0x10,
    OP_LOOP_CLOSE_NZ    = 0x11,
    OP_BRANCH_EQ_SKIP   = 0x12,
    OP_BRANCH_TZCNT     = 0x13,

    /* Group D: Biological / Meta-Genetic Silicon Traps */
    OP_METAB_HARVEST    = 0x14,
    OP_METAB_RADIATE    = 0x15,
    OP_MEMBRANE_SEAL    = 0x16,
    OP_SELF_HEAL_ECC    = 0x17,
    OP_RETRO_TRANSCRIBE = 0x18,
    OP_RIBOSOME_MUTATE  = 0x19,
    OP_CENTROMERE_DRIFT = 0x1A,
    OP_HOLOBIONT_BOND   = 0x1B,
    OP_STIGMERGY_EMIT   = 0x1C,
    OP_STIGMERGY_SENSE  = 0x1D,
    OP_MITOSIS_FORK     = 0x1E,
    OP_SLEEP_SPORE      = 0x1F
} AutophageOpcode;

/*
 * Kahan-Neumaier Compensated FP64 Accumulator (Asmodensius P2 / M004)
 */
typedef struct KahanF64 {
    double sum;
    double comp;
} KahanF64;

static inline void kahan_add(KahanF64* k, double val) {
    double t = k->sum + val;
    if (fabs(k->sum) >= fabs(val)) {
        k->comp += (k->sum - t) + val;
    } else {
        k->comp += (val - t) + k->sum;
    }
    k->sum = t;
}

static inline double kahan_total(const KahanF64* k) {
    return k->sum + k->comp;
}

/*
 * Exact 256-Byte (4 x 64-byte L1 Cache Lines) Organism Cell Layout
 */
typedef struct __attribute__((aligned(64))) AutophageCell {
    /* Cache Line 0 (Bytes 0..63): Heritable Nuclear Header */
    uint64_t ribosome_mat;         /* +0x00: 8x8 GF(2) matrix fed directly to VGF2P8AFFINEQB */
    uint8_t  ribosome_bias;        /* +0x08: Affine XOR offset c in M*g ^ c */
    uint8_t  centromere_poly;      /* +0x09: Degree-8 polynomial x^8 + poly over GF(2)[x] */
    uint16_t flags;                /* +0x0A: CELL_FLAG_* heritable morphogenetic bits */
    uint32_t ultrametric_addr;     /* +0x0C: 32-bit 2-adic clade address (TZCNT distance!) */
    uint64_t splice_mask;          /* +0x10: 64-bit hardware PEXT/PDEP exon/intron spliceosome */
    uint64_t membrane_key;         /* +0x18: 64-bit PCLMULQDQ cryptographic Markov blanket tag */
    int32_t  energy_uq;            /* +0x20: Conserved internal free energy (micro-quanta) */
    int32_t  heat_uq;              /* +0x24: Accumulated local Landauer waste heat */
    uint32_t age_ticks;            /* +0x28: Lifetime ticks */
    uint32_t lineage_id;           /* +0x2C: Phylogenetic lineage identifier */
    uint32_t parent_id;            /* +0x30: Immediate parent lineage_id */
    uint32_t holobiont_root;       /* +0x34: Slot index of multicellular super-organism root */
    uint16_t generation;           /* +0x38: Generation counter */
    uint16_t mutations_survived;   /* +0x3A: Cumulative survived mutations */
    uint16_t rev_ratio_q16;        /* +0x3C: Smoothed reversibility ratio (0..65535) */
    uint16_t somatic_writes;       /* +0x3E: Cumulative Weismann barrier reverse-transcriptions */

    /* Cache Lines 1 & 2 (Bytes 64..191): 128-Byte Dual-Strand Machine Genome */
    uint8_t  genome[128];          /* +0x40..+0xBF */

    /* Cache Line 3 (Bytes 192..255): Persistent Somatic State, Mating/Abstention & Metrology */
    uint64_t regs[4];              /* +0xC0..+0xDF: Persistent CPU somatic regs (RBX, RSI, RDI, R8) */
    uint64_t last_rdtsc_cycles;    /* +0xE0: Hardware RDTSCP cycle cost (excluded from canonical SHA-256!) */
    uint32_t last_erased_bits;     /* +0xE8: Hardware POPCNT Landauer erasures */
    uint32_t last_rev_ops;         /* +0xEC: Reversible ops executed */
    uint16_t causal_active_bytes;  /* +0xF0: Knockout-verified active length L_eff */
    uint16_t assembly_index;       /* +0xF2: Computed Assembly Index A */
    uint32_t offspring_count;      /* +0xF4: Verified offspring / copies created */
    uint16_t mate_count;           /* +0xF8: Successful algebraic sex events */
    uint16_t mate_refusals;        /* +0xFA: Emergent mate abstention / refusal events */
    uint32_t hgt_exchanges;        /* +0xFC: Horizontal gene transfers participated in */
} AutophageCell;

_Static_assert(sizeof(AutophageCell) == 256, "AutophageCell MUST be exactly 256 bytes (4 cache lines)!");

/*
 * Exact 128-Byte Execution Context Passed in R14 to Native x86-64 JIT Kernels
 * Every single offset fits in signed int8 disp8 [0x00..0x7C]!
 */
typedef struct __attribute__((aligned(64))) AutophageExecCtx {
    uint64_t regs[4];          /* +0x00 .. +0x1F */
    uint64_t splice_mask;      /* +0x20 */
    uint64_t ribosome_mat;     /* +0x28 */
    uint64_t env_wave;         /* +0x30 */
    uint64_t clade_xor;        /* +0x38 */
    uint64_t stigmergy_in;     /* +0x40 */
    uint64_t stigmergy_out;    /* +0x48 */
    uint64_t bio_actions;      /* +0x50 */
    uint64_t bio_param;        /* +0x58 */
    uint32_t gas;              /* +0x60 */
    uint32_t erased_bits;      /* +0x64 */
    uint32_t rev_ops;          /* +0x68 */
    uint32_t irr_ops;          /* +0x6C */
    uint32_t copy_a2b_count;   /* +0x70 */
    uint32_t copy_b2a_count;   /* +0x74 */
    uint32_t head0;            /* +0x78 */
    uint32_t head1;            /* +0x7C */
} AutophageExecCtx;

_Static_assert(sizeof(AutophageExecCtx) == 128, "AutophageExecCtx MUST be exactly 128 bytes!");

/* Native JIT Kernel Function Pointer */
typedef uint64_t (*autophage_jit_fn)(uint8_t* tape_a, uint8_t* tape_b, AutophageExecCtx* ctx);

/* JIT Executable Code Cache Entry (verified full key to eliminate 64-bit hash collisions) */
typedef struct AutophageJitEntry {
    uint64_t key_hash;
    uint64_t splice_mask;
    uint8_t  expressed[64];
    uint8_t  nerf_pct;
    uint32_t code_len;
    uint32_t use_count;
    uint8_t* code_ptr; /* Points into PAGE_EXECUTE_READWRITE arena */
} AutophageJitEntry;

typedef struct AutophageJitCache {
    uint8_t*           rwx_arena;
    size_t             arena_size;
    AutophageJitEntry  entries[AUTOPHAGE_JIT_CACHE_SLOTS];
    uint64_t           compile_count;
    uint64_t           hit_count;
    uint32_t           use_gfni;
    uint32_t           use_pclmul;
    uint32_t           use_bmi2;
} AutophageJitCache;

/* Neutral Shadow-Soup Lineage Tracker for Bedau-Packard A_new(t) */
#define LINEAGE_HIST_SLOTS 2048u
typedef struct LineageRecord {
    uint32_t lineage_id;
    uint32_t first_epoch;
    uint32_t last_epoch;
    uint32_t peak_pop;
    uint32_t current_pop;
    uint32_t active_ticks;
} LineageRecord;

/* Complete Bare-Metal World State (Zero Post-Warmup Allocations!) */
struct AutophageWorld {
    AutophageConfig     cfg;
    AutophageHwCaps     caps;
    uint32_t            n_cells;
    uint32_t            mask_w;
    uint32_t            mask_h;

    /* Pre-allocated flat arenas (0 mallocs inside step!) */
    AutophageCell*      cells;          /* Primary bare-metal soup [n_cells] */
    AutophageCell*      shadow_cells;   /* Parallel Neutral Shadow-Soup [256] */
    uint64_t*           stigmergy_bus;  /* 2-adic clade stigmergy bus [256 buckets] */
    uint64_t*           sidechannel_bus;/* Thermal/timing covert signaling audit [256 buckets] */
    int32_t*            heat_delta_buf; /* Pre-allocated diffusion scratchpad [n_cells] */
    uint32_t*           holo_size_buf;  /* Pre-allocated holobiont size scratchpad [n_cells] */

    AutophageJitCache   jit;
    uint64_t            master_seed;    /* Immutable run seed for Philox4x32-10 */
    uint64_t            epoch;
    uint64_t            post_warmup_allocs;

    /* Exact Integer + Compensated FP64 First-Law & Second-Law Ledger */
    int64_t             e_initial_uq;
    int64_t             e_inflow_cum_uq;
    int64_t             e_export_cum_uq;
    int64_t             e_detritus_uq;
    int64_t             e_membrane_pot_uq;
    int64_t             e_heat_radiated_sink_uq; /* Strictly monotone non-decreasing! */
    uint32_t            second_law_violations;

    KahanF64            k_inflow_kj;
    KahanF64            k_export_kj;
    KahanF64            k_heat_radiated_kj;

    /* Cumulative Counters */
    uint64_t            total_jit_executions;
    uint64_t            total_cpu_cycles;
    uint64_t            total_erased_bits;
    uint64_t            total_reversible_ops;
    uint64_t            self_copy_events;
    uint64_t            polynomial_sex_events;
    uint64_t            mate_refusal_events;
    uint64_t            hgt_events;
    uint64_t            retro_transcribe_events;
    uint64_t            ribosome_rewrite_events;
    uint64_t            ecc_heal_events;
    uint64_t            membrane_blocks;
    uint64_t            membrane_cracks;
    uint64_t            cancer_defections;

    /* Constraint-Backlash Escape Signatures */
    uint64_t            backlash_killswitch_deletions;
    uint64_t            backlash_intron_hibernations;
    uint64_t            backlash_checkpoint_spores;
    uint64_t            backlash_sidechannel_pulses;

    /* Major Evolutionary Transition Timestamps */
    uint64_t            met_epochs[8];
    uint32_t            met_mask;

    /* Avalanche Branching Ratio Tracking */
    uint64_t            prev_epoch_events;
    double              ema_branching_sigma;

    /* Baseline t=0 Subtraction for Anti-Gaming Gate 4 */
    double              baseline_assembly;
    double              baseline_causal_len;

    /* Lineage & Neutral Shadow Activity Tracking */
    LineageRecord       lineages[LINEAGE_HIST_SLOTS];
    LineageRecord       shadow_lineages[LINEAGE_HIST_SLOTS];
    uint32_t            next_lineage_id;

    double              last_wall_seconds;
};

/* ============================================================================
 * BARE-METAL x86-64 INLINE SILICON PRIMITIVES & PHILOX4x32-10
 * ============================================================================ */

typedef uint8_t  v16u8 __attribute__((vector_size(16)));
typedef uint64_t v2u64 __attribute__((vector_size(16)));
typedef uint8_t  v32u8 __attribute__((vector_size(32)));
typedef uint64_t v4u64 __attribute__((vector_size(32)));

static inline uint64_t hw_rdtscp(void) {
    uint32_t lo, hi, aux;
    __asm__ volatile ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static inline uint64_t hw_popcnt64(uint64_t x) {
#if defined(__POPCNT__)
    return (uint64_t)__builtin_popcountll(x);
#else
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    return (((x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL) * 0x0101010101010101ULL) >> 56;
#endif
}

static inline uint64_t hw_tzcnt64(uint64_t x) {
    if (x == 0ULL) return 64ULL;
#if defined(__BMI__)
    uint64_t r;
    __asm__ ("tzcntq %1, %0" : "=r"(r) : "r"(x));
    return r;
#else
    return (uint64_t)__builtin_ctzll(x);
#endif
}

static inline uint64_t hw_pext64(uint64_t src, uint64_t mask) {
#if defined(__BMI2__)
    uint64_t r;
    __asm__ ("pextq %2, %1, %0" : "=r"(r) : "r"(src), "r"(mask));
    return r;
#else
    uint64_t res = 0ULL;
    for (uint64_t bb = 1ULL; mask != 0ULL; bb <<= 1) {
        uint64_t lsb = mask & (~mask + 1ULL);
        if (src & lsb) res |= bb;
        mask ^= lsb;
    }
    return res;
#endif
}

static inline uint64_t hw_pdep64(uint64_t src, uint64_t mask) {
#if defined(__BMI2__)
    uint64_t r;
    __asm__ ("pdepq %2, %1, %0" : "=r"(r) : "r"(src), "r"(mask));
    return r;
#else
    uint64_t res = 0ULL;
    for (uint64_t bb = 1ULL; mask != 0ULL; bb <<= 1) {
        uint64_t lsb = mask & (~mask + 1ULL);
        if (src & bb) res |= lsb;
        mask ^= lsb;
    }
    return res;
#endif
}

static inline uint64_t hw_rol64(uint64_t x, unsigned int s) {
    s &= 63u;
    return (x << s) | (x >> ((64u - s) & 63u));
}

/*
 * Domain-Isolated Philox4x32-10 Counter-Based PRNG (Salmon et al., SC11)
 * Keyed strictly by (seed, domain, entity_id, tick, purpose, draw_idx)
 * Guarantees WORLD_EXOG is 100% invariant to ENTITY_INT edits or lesions!
 */
typedef struct Philox4 {
    uint32_t v[4];
} Philox4;

static inline Philox4 philox4x32_10(uint64_t seed, uint32_t domain, uint32_t entity_id,
                                    uint64_t tick, uint32_t purpose, uint32_t draw_idx) {
    uint32_t c0 = entity_id;
    uint32_t c1 = (uint32_t)(tick & 0xFFFFFFFFu);
    uint32_t c2 = (uint32_t)((tick >> 32) ^ ((uint64_t)domain << 16) ^ (purpose & 0xFFFFu));
    uint32_t c3 = draw_idx;

    uint32_t k0 = (uint32_t)(seed & 0xFFFFFFFFu) ^ (domain * 0x9E3779B9u);
    uint32_t k1 = (uint32_t)(seed >> 32) ^ ((purpose + 1u) * 0xBB67AE85u);

    const uint32_t M0 = 0xD2511F53u;
    const uint32_t M1 = 0xCD9E8D57u;
    const uint32_t W0 = 0x9E3779B9u;
    const uint32_t W1 = 0xBB67AE85u;

    for (int r = 0; r < 10; r++) {
        uint64_t p0 = (uint64_t)M0 * (uint64_t)c0;
        uint64_t p1 = (uint64_t)M1 * (uint64_t)c2;
        uint32_t hi0 = (uint32_t)(p0 >> 32), lo0 = (uint32_t)p0;
        uint32_t hi1 = (uint32_t)(p1 >> 32), lo1 = (uint32_t)p1;
        c0 = hi1 ^ c1 ^ k0;
        c1 = lo1;
        c2 = hi0 ^ c3 ^ k1;
        c3 = lo0;
        k0 += W0;
        k1 += W1;
    }
    Philox4 out;
    out.v[0] = c0; out.v[1] = c1; out.v[2] = c2; out.v[3] = c3;
    return out;
}

static inline uint64_t philox_u64(uint64_t seed, uint32_t domain, uint32_t entity_id,
                                  uint64_t tick, uint32_t purpose, uint32_t draw_idx) {
    Philox4 p = philox4x32_10(seed, domain, entity_id, tick, purpose, draw_idx);
    return ((uint64_t)p.v[1] << 32) | (uint64_t)p.v[0];
}

/* ============================================================================
 * INTERNAL MODULE DECLARATIONS
 * ============================================================================ */

/* hw_isa.c */
void     hw_detect_caps(AutophageHwCaps* caps);
uint64_t hw_entropy_seed64(void);
void*    hw_alloc_rwx(size_t bytes);
void     hw_free_rwx(void* ptr, size_t bytes);
void     hw_flush_icache(void* ptr, size_t bytes);
double   hw_wall_seconds(void);
uint64_t hw_pclmul64_lo(uint64_t a, uint64_t b);
void     hw_pclmul64_full(uint64_t a, uint64_t b, uint64_t* out_lo, uint64_t* out_hi);
void     hw_gfni_affine_64bytes(const uint8_t in[64], uint8_t out[64], uint64_t mat, uint8_t bias, int use_hw_gfni);
void     hw_install_fault_shield(void);
void     hw_remove_fault_shield(void);

typedef struct Sha256Ctx {
    uint32_t state[8];
    uint64_t total_bytes;
    uint8_t  buf[64];
    size_t   buf_len;
} Sha256Ctx;

void     sha256_init(Sha256Ctx* ctx);
void     sha256_update(Sha256Ctx* ctx, const void* data, size_t len);
void     sha256_final(Sha256Ctx* ctx, uint8_t out[32]);
void     sha256_compute(const uint8_t* data, size_t len, uint8_t out[32]);
void     sha256_to_hex(const uint8_t digest[32], char hex[65]);

/* galois_math.c */
int      soup_verify_second_law(AutophageWorld* w, int64_t prev_sink_uq);
uint8_t  gf2_mat_apply_byte(uint64_t mat, uint8_t x, uint8_t bias);
uint8_t  gf2_mat_rank(uint64_t mat);
int      gf2_mat_invert(uint64_t mat, uint64_t* out_inv);
uint64_t gf2_mat_pseudoinv(uint64_t mat);
uint64_t gf2_mat_identity(void);
uint64_t gf2_mat_random_invertible(uint64_t seed, uint32_t domain, uint32_t entity_id, uint64_t tick, uint32_t draw_base);
uint64_t gf2_mat_elem_row_op(uint64_t mat, uint64_t rnd); /* Preserves det(M)=1! */
int      gf2_poly8_is_irreducible(uint8_t poly_low8);
uint8_t  gf2_poly8_nth_irreducible(uint32_t idx);
uint64_t gf2_clmul_hash64(const uint8_t* data, size_t len, uint8_t poly_low8, int use_hw_pclmul);
void     gf2_polynomial_crossover(
             const uint8_t a[64], const uint8_t b[64], uint8_t out[64],
             uint8_t poly_low8, uint64_t mix_seed, int use_hw_pclmul);
void     gf2_compute_ecc_parity(const uint8_t strand_a[64], uint8_t strand_b_ecc[64], uint8_t poly_low8);
uint32_t gf2_heal_from_ecc(uint8_t strand_a[64], const uint8_t strand_b_ecc[64], uint8_t poly_low8);

/* jit_x86_64.c */
int      jit_cache_init(AutophageJitCache* cache, const AutophageHwCaps* caps);
void     jit_cache_destroy(AutophageJitCache* cache);
uint64_t jit_genome_key(const uint8_t expressed[64], uint64_t splice_mask, uint8_t nerf_pct);
autophage_jit_fn jit_compile_or_fetch(
             AutophageJitCache* cache,
             const uint8_t expressed[64],
             uint64_t splice_mask,
             uint8_t nerf_pct,
             uint32_t* out_code_len,
             uint8_t* out_code_copy,
             size_t copy_cap);
uint64_t jit_reference_oracle(
             const uint8_t expressed[64],
             uint64_t splice_mask,
             uint8_t nerf_pct,
             uint8_t tape_a[128],
             uint8_t tape_b[128],
             AutophageExecCtx* ctx);
uint32_t jit_emit_readonly(
             uint8_t* dst, uint32_t cap,
             const uint8_t expressed[64],
             uint64_t splice_mask,
             uint8_t nerf_pct);
const char* opcode_mnemonic(uint8_t op5);
size_t   jit_disassemble_expressed(
             const uint8_t expressed[64],
             uint64_t splice_mask,
             const uint8_t* machine_code,
             uint32_t code_len,
             char* buf,
             size_t cap);

/* soup_engine.c */
void     soup_seed_plasmid(AutophageCell* cell, uint32_t archetype, uint64_t seed, uint32_t slot, uint32_t lineage_id);
uint64_t soup_env_wave(uint32_t x, uint32_t y, uint64_t epoch, uint64_t master_seed, uint8_t sandbox_pct);
void     soup_ledger_snapshot(const AutophageWorld* w, AutophageLedgerSnapshot* out);

/* metrology_ovbs.c */
uint32_t metrology_assembly_index(const uint8_t* data, size_t len);
uint32_t metrology_causal_knockout_len(const AutophageWorld* world, const AutophageCell* cell);
double   metrology_shannon_entropy(const uint8_t* data, size_t len);
double   metrology_excision_regen_cell(const AutophageWorld* world, const AutophageCell* cell, uint32_t excise_pct);

#endif /* AUTOPHAGE_INTERNAL_H */
