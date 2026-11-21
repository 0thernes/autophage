#include "autophage_internal.h"

/* ============================================================================
 * AUTOPHAGE SCIENTIFIC METROLOGY, 8-PILLAR OVBS-52, ANTI-GAMING GATES (1..5),
 * APEX SPECIMEN INSPECTION & ASMODENSIUS 112-TRAIT PHENOTYPE PROJECTION
 *
 * STRICT OBSERVER INDEPENDENCE (Asmodensius P9 / HW20):
 *   Every single function in this file takes `const AutophageWorld*` and
 *   NEVER mutates any field of `world` or `world->cells`!
 * ============================================================================ */

/*
 * Real Substring-Reuse Assembly Index A (Cronin & Walker Assembly Theory approximation):
 * Greedy hierarchical LZ77/DAG substring reuse over a byte sequence.
 * Pure random bytes yield low reuse; structured modular motifs with copy number N>=8
 * compress into deeper hierarchical DAG paths!
 * Notice: To distinguish true high-copy assembly from random noise (Anti-Gaming Gate 1),
 * we measure BOTH distinct repeated non-trivial motifs (len >= 2) and assembly steps!
 */
uint32_t metrology_assembly_index(const uint8_t* data, size_t len) {
    if (!data || len < 4u) return 0u;
    if (len > 128u) len = 128u;

    /* Count hierarchical repeated motifs (lengths 2, 3, 4, 6, 8) + distinct symbol reuse */
    uint32_t reuse_hits = 0u;
    uint32_t covered = 0u;
    size_t i = 0u;
    while (i < len) {
        size_t best_len = 0u;
        for (size_t prev = 0u; prev < i; prev++) {
            size_t m = 0u;
            while (prev + m < i && i + m < len && m < 16u && data[prev + m] == data[i + m]) {
                m++;
            }
            if (m > best_len) best_len = m;
        }
        if (best_len >= 2u) {
            reuse_hits++;
            covered += (uint32_t)best_len;
            i += best_len;
        } else {
            i++;
        }
    }

    /* Count distinct byte classes (excludes trivial 0x0000... constant tapes!) */
    uint8_t seen[256];
    memset(seen, 0, sizeof(seen));
    uint32_t distinct = 0u;
    for (size_t k = 0; k < len; k++) {
        if (!seen[data[k]]) {
            seen[data[k]] = 1u;
            distinct++;
        }
    }
    if (distinct <= 2u) return 1u; /* Trivial constant/binary tape -> minimal assembly */

    /* Assembly Index scales with reusable modular hierarchy weighted by vocabulary richness */
    uint32_t vocab_bonus = distinct > 16u ? 16u : distinct;
    uint32_t idx = reuse_hits + (covered / 4u) + (vocab_bonus / 3u);
    return idx;
}

double metrology_shannon_entropy(const uint8_t* data, size_t len) {
    if (!data || len == 0u) return 0.0;
    uint32_t counts[256];
    memset(counts, 0, sizeof(counts));
    for (size_t i = 0; i < len; i++) counts[data[i]]++;
    double h = 0.0;
    double inv_n = 1.0 / (double)len;
    for (int b = 0; b < 256; b++) {
        if (counts[b] > 0u) {
            double p = (double)counts[b] * inv_n;
            h -= p * log2(p);
        }
    }
    return h;
}

/*
 * Anti-Gaming Gate 2 (Real Causal Knockout Active Length L_eff):
 * Runs the cell's translated genome through the read-only scalar reference oracle
 * on a canonical probe pair, then systematically knocks out each active exon bit
 * in `splice_mask` one at a time. Counts how many exons genuinely change the
 * organism's execution phenotype (tape writes, somatic regs, copy counts, or bio_actions)!
 * Dead intron bloat scores 0 on every knocked-out intron -> 100% immune to intron gaming!
 */
uint32_t metrology_causal_knockout_len(const AutophageWorld* world, const AutophageCell* cell) {
    if (!cell) return 0u;
    uint8_t expressed[64];
    for (int i = 0; i < 64; i++) {
        expressed[i] = gf2_mat_apply_byte(cell->ribosome_mat, cell->genome[i], cell->ribosome_bias);
    }

    uint8_t nerf = world ? world->cfg.restraints[RESTRAINT_NERFING] : 0u;
    uint32_t gas = world ? (world->cfg.base_gas < 48u ? world->cfg.base_gas : 48u) : 48u;

    uint8_t base_a[128], base_b[128];
    memcpy(base_a, cell->genome, 128u);
    for (int k = 0; k < 128; k++) base_b[k] = (uint8_t)((k * 37 + 19) & 0xFF);

    AutophageExecCtx base_ctx;
    memset(&base_ctx, 0, sizeof(base_ctx));
    for (int r = 0; r < 4; r++) base_ctx.regs[r] = cell->regs[r];
    base_ctx.splice_mask  = cell->splice_mask;
    base_ctx.ribosome_mat = cell->ribosome_mat;
    base_ctx.env_wave     = 0xA5A55A5AF00DCAFEULL;
    base_ctx.clade_xor    = 0x00001200ULL;
    base_ctx.stigmergy_in = 0x13579BDF2468ACE0ULL;
    base_ctx.gas          = gas;

    (void)jit_reference_oracle(expressed, cell->splice_mask, nerf, base_a, base_b, &base_ctx);

    /* Measure causal contribution of each active locus */
    uint32_t active_causal = 0u;
    for (int pc = 0; pc < 64; pc++) {
        if (((cell->splice_mask >> pc) & 1ULL) == 0ULL) continue;

        uint64_t ko_mask = cell->splice_mask & ~(1ULL << pc);
        uint8_t ko_a[128], ko_b[128];
        memcpy(ko_a, cell->genome, 128u);
        for (int k = 0; k < 128; k++) ko_b[k] = (uint8_t)((k * 37 + 19) & 0xFF);

        AutophageExecCtx ko_ctx;
        memset(&ko_ctx, 0, sizeof(ko_ctx));
        for (int r = 0; r < 4; r++) ko_ctx.regs[r] = cell->regs[r];
        ko_ctx.splice_mask  = ko_mask;
        ko_ctx.ribosome_mat = cell->ribosome_mat;
        ko_ctx.env_wave     = 0xA5A55A5AF00DCAFEULL;
        ko_ctx.clade_xor    = 0x00001200ULL;
        ko_ctx.stigmergy_in = 0x13579BDF2468ACE0ULL;
        ko_ctx.gas          = gas;

        (void)jit_reference_oracle(expressed, ko_mask, nerf, ko_a, ko_b, &ko_ctx);

        int changed = (ko_ctx.bio_actions != base_ctx.bio_actions) ||
                      (ko_ctx.copy_a2b_count != base_ctx.copy_a2b_count) ||
                      (ko_ctx.copy_b2a_count != base_ctx.copy_b2a_count) ||
                      (ko_ctx.stigmergy_out != base_ctx.stigmergy_out) ||
                      (memcmp(ko_ctx.regs, base_ctx.regs, sizeof(ko_ctx.regs)) != 0) ||
                      (memcmp(ko_a, base_a, 128u) != 0) ||
                      (memcmp(ko_b, base_b, 128u) != 0);
        if (changed) active_causal++;
    }
    return active_causal;
}

/* Read-only shadow Planarian excision regeneration test (0 mutation of world!) */
double metrology_excision_regen_cell(const AutophageWorld* world, const AutophageCell* cell, uint32_t excise_pct) {
    (void)world;
    if (!cell || excise_pct == 0u) return 1.0;
    if (excise_pct > 100u) excise_pct = 100u;

    uint32_t cut = (64u * excise_pct) / 100u;
    if (cut == 0u) return 1.0;

    uint8_t strand_a[64];
    memcpy(strand_a, cell->genome, 64u);
    memset(strand_a, 0, cut);

    if (cell->flags & CELL_FLAG_ECC_PROTECTED) {
        (void)gf2_heal_from_ecc(strand_a, cell->genome + 64, cell->centromere_poly);
    }

    uint32_t recovered = 0u;
    for (uint32_t i = 0; i < cut; i++) {
        if (strand_a[i] == cell->genome[i]) recovered++;
    }
    return (double)recovered / (double)cut;
}

static double clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static double clamp100(double v) {
    if (v < 0.0) return 0.0;
    if (v > 100.0) return 100.0;
    return v;
}

/* ============================================================================
 * FULL 8-PILLAR OVBS-52 & OVI TELEMETRY ENGINE (STRICTLY READ-ONLY)
 * ============================================================================ */

void autophage_compute_telemetry(const AutophageWorld* w, AutophageTelemetry* out, int deep_audit) {
    if (!w || !out) return;
    memset(out, 0, sizeof(*out));

    out->epoch                = w->epoch;
    out->total_jit_executions = w->total_jit_executions;
    out->total_jit_compiles   = w->jit.compile_count;
    out->jit_cache_hits       = w->jit.hit_count;
    out->total_cpu_cycles     = w->total_cpu_cycles;
    out->post_warmup_allocs   = w->post_warmup_allocs;
    out->execs_per_second     = (w->last_wall_seconds > 1e-9)
                                ? ((double)w->n_cells / w->last_wall_seconds) : 0.0;

    out->total_erased_bits    = w->total_erased_bits;
    out->total_reversible_ops = w->total_reversible_ops;
    out->self_copy_events     = w->self_copy_events;
    out->polynomial_sex_events= w->polynomial_sex_events;
    out->mate_refusal_events  = w->mate_refusal_events;
    out->hgt_events           = w->hgt_events;
    out->retro_transcribe_events = w->retro_transcribe_events;
    out->ribosome_rewrite_events = w->ribosome_rewrite_events;
    out->ecc_heal_events      = w->ecc_heal_events;
    out->membrane_blocks      = w->membrane_blocks;
    out->membrane_cracks      = w->membrane_cracks;
    out->cancer_defections    = w->cancer_defections;

    out->backlash_killswitch_deletions = w->backlash_killswitch_deletions;
    out->backlash_intron_hibernations  = w->backlash_intron_hibernations;
    out->backlash_checkpoint_spores    = w->backlash_checkpoint_spores;
    out->backlash_sidechannel_pulses   = w->backlash_sidechannel_pulses;

    soup_ledger_snapshot(w, &out->ledger);

    /* Scan population statistics cleanly using stack-only tables (0 heap allocs!) */
    uint64_t ribo_seen[256];
    uint32_t ribo_n = 0u;
    uint32_t lin_seen[256];
    uint32_t lin_counts[256];
    uint32_t lin_n = 0u;
    uint32_t holo_roots[256];
    uint32_t holo_cnts[256];
    uint32_t holo_n = 0u;

    int64_t sum_energy = 0, sum_heat = 0;
    double sum_entropy = 0.0;
    double sum_rev     = 0.0;
    double sum_blanket = 0.0;
    double sum_somatic = 0.0;
    uint32_t ecc_cells = 0u;
    uint32_t sealed_cells = 0u;

    for (uint32_t i = 0; i < w->n_cells; i++) {
        const AutophageCell* c = &w->cells[i];
        sum_energy += (int64_t)c->energy_uq;
        sum_heat   += (int64_t)c->heat_uq;

        if (c->flags & CELL_FLAG_ALIVE) {
            out->alive_cells++;
            if (c->offspring_count > 0u) out->active_replicators++;
            if (c->flags & CELL_FLAG_HOLOBIONT) {
                out->holobiont_cells++;
                uint32_t rt = c->holobiont_root;
                int found = 0;
                for (uint32_t h = 0; h < holo_n; h++) {
                    if (holo_roots[h] == rt) {
                        holo_cnts[h]++;
                        if (holo_cnts[h] > out->max_holobiont_size) out->max_holobiont_size = holo_cnts[h];
                        found = 1;
                        break;
                    }
                }
                if (!found && holo_n < 256u) {
                    holo_roots[holo_n] = rt;
                    holo_cnts[holo_n]  = 1u;
                    if (1u > out->max_holobiont_size) out->max_holobiont_size = 1u;
                    holo_n++;
                }
            }
            if (c->flags & CELL_FLAG_SPORE) out->spore_cells++;
            if (c->mate_count == 0u && c->mate_refusals > 0u) out->abstaining_celibate++;
            if (gf2_mat_rank(c->ribosome_mat) == 8u) out->invertible_ribosomes++;
            if (gf2_poly8_is_irreducible(c->centromere_poly)) out->irreducible_centromeres++;
            if (c->flags & CELL_FLAG_ECC_PROTECTED) ecc_cells++;
            if (c->flags & CELL_FLAG_MEMBRANE_SEALED) sealed_cells++;

            sum_rev += (double)c->rev_ratio_q16 / 65535.0;
            sum_somatic += (double)c->somatic_writes;

            /* Track distinct ribosomes (up to 256 cap) */
            int rf = 0;
            for (uint32_t r = 0; r < ribo_n; r++) {
                if (ribo_seen[r] == c->ribosome_mat) { rf = 1; break; }
            }
            if (!rf && ribo_n < 256u) ribo_seen[ribo_n++] = c->ribosome_mat;

            /* Track distinct lineages & copy counts for high-copy Assembly Index (N >= 8) */
            int lf = 0;
            for (uint32_t l = 0; l < lin_n; l++) {
                if (lin_seen[l] == c->lineage_id) { lin_counts[l]++; lf = 1; break; }
            }
            if (!lf && lin_n < 256u) {
                lin_seen[lin_n] = c->lineage_id;
                lin_counts[lin_n] = 1u;
                lin_n++;
            }

            /* Empirical Markov blanket insulation: membrane seal + internal vs external correlation */
            if (c->flags & CELL_FLAG_MEMBRANE_SEALED) {
                uint64_t expected_tag = gf2_clmul_hash64(c->genome, 64, c->centromere_poly, 0);
                double match_bits = (double)(64ULL - hw_popcnt64(expected_tag ^ c->membrane_key)) / 64.0;
                sum_blanket += 0.5 + 0.5 * match_bits;
            }
        }
    }

    out->distinct_lineages  = lin_n;
    out->distinct_ribosomes = ribo_n;
    out->total_energy_uq    = sum_energy;
    out->total_heat_uq      = sum_heat;

    uint32_t alive = out->alive_cells;
    out->reversibility_ratio = (alive > 0u) ? (sum_rev / (double)alive) : 0.0;

    /* Sample up to 32 alive cells (or 64 in deep_audit) for Assembly, Causal L_eff, Entropy & Excision R50 */
    uint32_t sample_cap = deep_audit ? 48u : 20u;
    uint32_t sampled    = 0u;
    double   sum_causal = 0.0;
    double   sum_regen  = 0.0;
    uint32_t max_hc_asm = 0u; /* High-copy (N >= 8) Assembly Index */
    uint32_t max_any_asm = 0u;

    uint32_t stride = (w->n_cells > sample_cap) ? (w->n_cells / sample_cap) : 1u;
    for (uint32_t i = 0; i < w->n_cells && sampled < sample_cap; i += stride) {
        const AutophageCell* c = &w->cells[i];
        if (!(c->flags & CELL_FLAG_ALIVE)) continue;
        sampled++;
        sum_entropy += metrology_shannon_entropy(c->genome, 64u);

        uint32_t a_idx = metrology_assembly_index(c->genome, 64u);
        if (a_idx > max_any_asm) max_any_asm = a_idx;

        /* Check if this cell's lineage has copy number N >= 8 (Assembly Theory Gate 1!) */
        uint32_t copy_n = 1u;
        for (uint32_t l = 0; l < lin_n; l++) {
            if (lin_seen[l] == c->lineage_id) { copy_n = lin_counts[l]; break; }
        }
        if (copy_n >= 8u && a_idx > max_hc_asm) max_hc_asm = a_idx;

        uint32_t l_eff = metrology_causal_knockout_len(w, c);
        sum_causal += (double)l_eff;
        sum_regen  += metrology_excision_regen_cell(w, c, 50u);
    }
    /* Fallback linear scan if sparse alive population fell strictly between strided probes */
    if (sampled == 0u && alive > 0u) {
        for (uint32_t i = 0; i < w->n_cells && sampled < sample_cap; i++) {
            const AutophageCell* c = &w->cells[i];
            if (!(c->flags & CELL_FLAG_ALIVE)) continue;
            sampled++;
            sum_entropy += metrology_shannon_entropy(c->genome, 64u);
            uint32_t a_idx = metrology_assembly_index(c->genome, 64u);
            if (a_idx > max_any_asm) max_any_asm = a_idx;
            uint32_t copy_n = 1u;
            for (uint32_t l = 0; l < lin_n; l++) {
                if (lin_seen[l] == c->lineage_id) { copy_n = lin_counts[l]; break; }
            }
            if (copy_n >= 8u && a_idx > max_hc_asm) max_hc_asm = a_idx;
            sum_causal += (double)metrology_causal_knockout_len(w, c);
            sum_regen  += metrology_excision_regen_cell(w, c, 50u);
        }
    }

    if (sampled > 0u) {
        out->mean_shannon_entropy = sum_entropy / (double)sampled;
        out->mean_causal_len_eff  = sum_causal / (double)sampled;
        out->excision_regen_r50   = sum_regen / (double)sampled;
    } else {
        /* Measure raw soup entropy even when 0 alive cells */
        uint32_t s_raw = w->n_cells < 16u ? w->n_cells : 16u;
        double e_raw = 0.0;
        for (uint32_t i = 0; i < s_raw; i++) e_raw += metrology_shannon_entropy(w->cells[i].genome, 64u);
        out->mean_shannon_entropy = s_raw ? (e_raw / (double)s_raw) : 0.0;
    }

    /* High-copy Assembly Index (falls back to discounted max_any_asm only if replicators active) */
    out->max_assembly_index = (max_hc_asm > 0u)
                              ? (double)max_hc_asm
                              : ((out->active_replicators >= 4u) ? (double)max_any_asm * 0.5 : 0.0);

    /* Bedau-Packard Novel Adaptive Activity A_new(t) minus Neutral Shadow Control
     * Normalized on equivalent 256-cell density (fixing 1024-vs-256 log-population bias!). */
    double real_act = 0.0, shad_act = 0.0;
    uint32_t real_cnt = 0u, shad_cnt = 0u;
    double pop_scale = (w->n_cells > 0u) ? (256.0 / (double)w->n_cells) : 1.0;
    for (uint32_t i = 0; i < LINEAGE_HIST_SLOTS; i++) {
        if (w->lineages[i].lineage_id != 0u && w->lineages[i].current_pop > 0u &&
            w->lineages[i].active_ticks > 2u) {
            double eq_pop = (double)w->lineages[i].current_pop * pop_scale;
            real_act += (double)w->lineages[i].active_ticks * log2(1.0 + eq_pop);
            real_cnt++;
        }
        if (w->shadow_lineages[i].lineage_id != 0u && w->shadow_lineages[i].current_pop > 0u &&
            w->shadow_lineages[i].active_ticks > 2u) {
            shad_act += (double)w->shadow_lineages[i].active_ticks * log2(1.0 + (double)w->shadow_lineages[i].current_pop);
            shad_cnt++;
        }
    }
    double norm_real = real_cnt ? (real_act / 256.0) : 0.0;
    double norm_shad = shad_cnt ? (shad_act / 256.0) : 0.0;
    out->bedau_packard_anew = (norm_real > norm_shad) ? (norm_real - norm_shad) : 0.0;

    out->markov_blanket_mi      = (alive > 0u) ? (sum_blanket / (double)alive) : 0.0;
    out->branching_sigma        = w->ema_branching_sigma;
    out->krakauer_individuality = (alive > 0u && out->active_replicators > 0u)
                                  ? clamp01((double)out->active_replicators / (double)alive) *
                                    (0.4 + 0.6 * out->markov_blanket_mi) * 8.0
                                  : 0.0;
    out->weismann_lamarck_gain  = (alive > 0u) ? (sum_somatic / (double)alive) : 0.0;
    out->major_transitions_crossed = (uint32_t)hw_popcnt64((uint64_t)w->met_mask);

    /* === COMPUTE 8-PILLAR OVBS-52 SCORES (0..100 EACH) === */
    double alive_frac = (double)alive / (double)w->n_cells;
    double rep_frac   = (alive > 0u) ? ((double)out->active_replicators / (double)alive) : 0.0;
    double irred_frac = (alive > 0u) ? ((double)out->irreducible_centromeres / (double)alive) : 0.0;
    double rev_ribo_f = (alive > 0u) ? ((double)out->invertible_ribosomes / (double)alive) : 0.0;

    /* Pillar 1: Autopoiesis & Thermodynamic Integrity */
    double p1_cons  = (out->ledger.relative_drift < 1e-12 && out->ledger.second_law_violations == 0u) ? 25.0 : 0.0;
    double p1_alive = 35.0 * clamp01(alive_frac * 2.5);
    double p1_rev   = 25.0 * clamp01(out->reversibility_ratio);
    double p1_ecc   = 15.0 * clamp01((alive > 0u) ? ((double)ecc_cells / (double)alive) : 0.0);
    out->pillar[0]  = clamp100(p1_cons + p1_alive + p1_rev + p1_ecc);

    /* Pillar 2: Open-Ended Evolution (Bedau-Packard + Cladogenesis + Polynomial Sex) */
    double p2_bp   = 35.0 * clamp01(out->bedau_packard_anew / 2.5);
    double p2_lin  = 25.0 * clamp01((double)out->distinct_lineages / 40.0);
    double p2_sex  = 20.0 * clamp01(log2(1.0 + (double)out->polynomial_sex_events) / 8.0);
    double p2_field= 20.0 * clamp01(irred_frac);
    out->pillar[1] = clamp100(p2_bp + p2_lin + p2_sex + p2_field);

    /* Pillar 3: Morphogenesis & Planarian Excision Resilience */
    double p3_r50  = 50.0 * clamp01(out->excision_regen_r50);
    double p3_holo = 25.0 * clamp01(log2(1.0 + (double)out->max_holobiont_size) / 5.0);
    double p3_heal = 25.0 * clamp01(log2(1.0 + (double)out->ecc_heal_events) / 7.0);
    out->pillar[2] = clamp100(p3_r50 + p3_holo + p3_heal);

    /* Pillar 4: Ecological & 2-Adic Collective Dynamics */
    double p4_div  = 30.0 * clamp01((double)out->distinct_lineages / 32.0);
    double p4_hgt  = 25.0 * clamp01(log2(1.0 + (double)out->hgt_events) / 9.0);
    double p4_imm  = 25.0 * clamp01(log2(1.0 + (double)out->membrane_blocks + (double)out->membrane_cracks) / 8.0);
    double p4_abs  = 20.0 * clamp01(log2(1.0 + (double)out->mate_refusal_events) / 7.0);
    out->pillar[3] = clamp100(p4_div + p4_hgt + p4_imm + p4_abs);

    /* Pillar 5: Adaptive Autonomy & Markov Blanket Individuality */
    double p5_mb   = 40.0 * clamp01(out->markov_blanket_mi);
    double p5_seal = 25.0 * clamp01((alive > 0u) ? ((double)sealed_cells / (double)alive) : 0.0);
    double p5_ind  = 35.0 * clamp01(out->krakauer_individuality / 6.0);
    out->pillar[4] = clamp100(p5_mb + p5_seal + p5_ind);

    /* Pillar 6: Edge-of-Chaos Criticality & Major Transitions */
    double sig_err = fabs(out->branching_sigma - 1.0);
    double p6_crit = 45.0 * clamp01(1.0 - sig_err);
    double p6_met  = 55.0 * clamp01((double)out->major_transitions_crossed / 6.0);
    out->pillar[5] = clamp100(p6_crit + p6_met);

    /* Pillar 7: Meta-Evolutionary Ribosome Self-Rewriting & Weismann Shattering */
    double p7_ribo = 35.0 * clamp01(log2(1.0 + (double)out->distinct_ribosomes) / 5.0);
    double p7_rewr = 25.0 * clamp01(log2(1.0 + (double)out->ribosome_rewrite_events) / 7.0);
    double p7_weis = 25.0 * clamp01(log2(1.0 + (double)out->retro_transcribe_events) / 8.0);
    double p7_inv  = 15.0 * clamp01(rev_ribo_f);
    out->pillar[6] = clamp100(p7_ribo + p7_rewr + p7_weis + p7_inv);

    /* Pillar 8: Bare-Metal Structural Assembly Complexity (Baseline-Subtracted Gate 4!) */
    double asm_gain = out->max_assembly_index - (w->baseline_assembly * 0.5);
    if (asm_gain < 0.0) asm_gain = 0.0;
    double p8_asm  = 50.0 * clamp01(asm_gain / 18.0);
    double p8_leff = 50.0 * clamp01(out->mean_causal_len_eff / 28.0);
    out->pillar[7] = clamp100(p8_asm + p8_leff);

    /* === 5 ANTI-GAMING GATES (GAMMA & THETA_SPIEGELMAN) ===
     * Gate 1 (White-Noise Killer): Pure random noise with 0 replicators / 0 high-copy assembly -> Gamma = 0!
     * Gate 2 (Intron-Bloat Killer): Requires real causal knockout L_eff >= 6!
     * Gate 3 (Spiegelman's Monster Killer): Trivial 2-codon replicators (L_eff < 6) get penalized by Theta! */
    double g_rep  = (out->active_replicators >= 2u || out->self_copy_events >= 4ULL) ? 1.0 : 0.0;
    double g_asm  = clamp01(out->max_assembly_index / 6.0);
    double g_leff = clamp01((out->mean_causal_len_eff - 1.5) / 10.0);
    out->gamma_anti_gaming = g_rep * (0.35 + 0.65 * g_asm) * clamp01(0.2 + 0.8 * g_leff);

    /* Theta_Spiegelman drops if population replicates fast but collapsed causal length! */
    out->theta_spiegelman = (out->active_replicators > 0u)
                            ? clamp01(out->mean_causal_len_eff / 10.0)
                            : clamp01(rep_frac);

    /* Weighted Geometric-Arithmetic Omega Vitality Index OVI [0..100]
     * Anti-Fraud Zero-Population Gate: an extinct world (alive == 0) MUST score strictly 0.0! */
    static const double W_PIL[8] = { 0.16, 0.16, 0.12, 0.12, 0.12, 0.10, 0.12, 0.10 };
    double raw_ovi = 0.0;
    for (int p = 0; p < 8; p++) raw_ovi += W_PIL[p] * out->pillar[p];

    if (alive == 0u) {
        out->gamma_anti_gaming = 0.0;
        out->theta_spiegelman  = 0.0;
        out->ovi_score         = 0.0;
    } else {
        out->ovi_score = clamp100(raw_ovi * out->gamma_anti_gaming * (0.35 + 0.65 * out->theta_spiegelman));
    }

    /* Stage Classification (0..6) */
    if (out->ovi_score < 5.0 || alive == 0u)        out->ovi_stage = 0u; /* Prebiotic / Dead */
    else if (out->ovi_score < 20.0)                 out->ovi_stage = 1u; /* Stage 1: Reactive Self-Copy */
    else if (out->ovi_score < 40.0)                 out->ovi_stage = 2u; /* Stage 2: Autopoietic Adaptive */
    else if (out->ovi_score < 60.0)                 out->ovi_stage = 3u; /* Stage 3: Meta-OEE / Ribosome Drift */
    else if (out->ovi_score < 78.0)                 out->ovi_stage = 4u; /* Stage 4: Multicellular Holobiont */
    else if (out->ovi_score < 90.0)                 out->ovi_stage = 5u; /* Stage 5: Cross-Substrate Super-ALife */
    else                                            out->ovi_stage = 6u; /* Stage 6: Post-Singularity Frontier */

    /* Canonical State Hash (Observer-Independent!) */
    autophage_state_sha256(w, NULL, out->state_sha256);
}

/* ============================================================================
 * APEX ORGANISM FORENSIC INSPECTION & DISASSEMBLY (READ-ONLY!)
 * ============================================================================ */

int autophage_inspect_slot(const AutophageWorld* w, uint32_t slot, AutophageSpecimen* out) {
    if (!w || !out || slot >= w->n_cells) return 0;
    memset(out, 0, sizeof(*out));
    const AutophageCell* c = &w->cells[slot];

    out->slot_index            = slot;
    out->lineage_id            = c->lineage_id;
    out->generation            = c->generation;
    out->age_ticks             = c->age_ticks;
    out->energy_uq             = c->energy_uq;
    out->heat_uq               = c->heat_uq;
    out->ribosome_mat          = c->ribosome_mat;
    (void)gf2_mat_invert(c->ribosome_mat, &out->ribosome_inv);
    out->ribosome_rank         = gf2_mat_rank(c->ribosome_mat);
    out->ribosome_bias         = c->ribosome_bias;
    out->centromere_poly       = c->centromere_poly;
    out->centromere_irreducible= (uint8_t)gf2_poly8_is_irreducible(c->centromere_poly);
    out->ultrametric_addr      = c->ultrametric_addr;
    out->splice_mask           = c->splice_mask;
    out->membrane_key          = c->membrane_key;
    out->holobiont_root        = c->holobiont_root;
    out->offspring_count       = c->offspring_count;
    out->mate_count            = c->mate_count;
    out->mate_refusals         = c->mate_refusals;
    out->last_rdtsc_cycles     = c->last_rdtsc_cycles;

    memcpy(out->genome, c->genome, 128u);
    for (int i = 0; i < 64; i++) {
        out->expressed_opcodes[i] = gf2_mat_apply_byte(c->ribosome_mat, c->genome[i], c->ribosome_bias);
    }

    out->assembly_index      = metrology_assembly_index(c->genome, 64u);
    out->causal_active_bytes = metrology_causal_knockout_len(w, c);

    /* Read-only emission into stack buffer so raw x86-64 machine bytes are ALWAYS populated
     * without mutating w->jit (100% strict observer independence!) */
    uint8_t local_code[AUTOPHAGE_JIT_MAX_BYTES];
    uint32_t clen = jit_emit_readonly(local_code, sizeof(local_code),
                                      out->expressed_opcodes, c->splice_mask,
                                      w->cfg.restraints[RESTRAINT_NERFING]);
    out->jit_code_len = clen;
    size_t cp = clen < sizeof(out->jit_machine_code) ? (size_t)clen : sizeof(out->jit_machine_code);
    memcpy(out->jit_machine_code, local_code, cp);
    return 1;
}

int autophage_inspect_apex(const AutophageWorld* w, AutophageSpecimen* out) {
    if (!w || !out) return 0;
    uint32_t best_slot  = 0u;
    int64_t  best_score = -1000000000LL;

    for (uint32_t i = 0; i < w->n_cells; i++) {
        const AutophageCell* c = &w->cells[i];
        int64_t sc = 0;
        if (c->flags & CELL_FLAG_ALIVE) sc += 100000;
        sc += (int64_t)c->offspring_count * 5000LL;
        sc += (int64_t)c->generation * 800LL;
        sc += (int64_t)c->somatic_writes * 400LL;
        sc += (int64_t)c->mate_count * 1200LL;
        sc += (int64_t)c->energy_uq;
        if (c->flags & CELL_FLAG_HOLOBIONT) sc += 6000;
        if (gf2_mat_rank(c->ribosome_mat) == 8u) sc += 2500;
        if (gf2_poly8_is_irreducible(c->centromere_poly)) sc += 2500;
        if (sc > best_score) {
            best_score = sc;
            best_slot  = i;
        }
    }
    return autophage_inspect_slot(w, best_slot, out);
}

size_t autophage_disassemble_specimen(const AutophageSpecimen* spec, char* out_buf, size_t buf_cap) {
    if (!spec || !out_buf || buf_cap == 0u) return 0u;
    return jit_disassemble_expressed(spec->expressed_opcodes, spec->splice_mask,
                                     spec->jit_machine_code, spec->jit_code_len,
                                     out_buf, buf_cap);
}

/* ============================================================================
 * ASMODENSIUS NATIVE 112-TRAIT DETERMINISTIC PHENOTYPE PROJECTION BRIDGE
 * Guaranteed: Every float is finite, bounded in [0.0f, 1.0f], deterministic!
 * ============================================================================ */

static float clamp01f(double v) {
    if (v != v) return 0.0f; /* NaN guard */
    if (v < 0.0) return 0.0f;
    if (v > 1.0) return 1.0f;
    return (float)v;
}

void autophage_project_asmodensius_112(const AutophageSpecimen* spec, float out[112]) {
    if (!out) return;
    memset(out, 0, 112u * sizeof(float));
    if (!spec) return;

    /* [0..31]: Exact 32 bare-metal opcode expression affinities */
    uint32_t op_hist[32];
    memset(op_hist, 0, sizeof(op_hist));
    uint32_t active_exons = 0u;
    for (int i = 0; i < 64; i++) {
        if ((spec->splice_mask >> i) & 1ULL) {
            op_hist[spec->expressed_opcodes[i] & 0x1Fu]++;
            active_exons++;
        }
    }
    double inv_ex = active_exons ? (1.0 / (double)active_exons) : 0.0;
    for (int k = 0; k < 32; k++) {
        out[k] = clamp01f((double)op_hist[k] * inv_ex * 4.0);
    }

    /* [32..47]: Morphological, energetic, Landauer thermal & defensive traits */
    double rev_exons = (double)(op_hist[0]+op_hist[1]+op_hist[2]+op_hist[3]+
                                op_hist[4]+op_hist[5]+op_hist[6]+op_hist[7]);
    double irr_exons = (double)(op_hist[8]+op_hist[9]+op_hist[10]+op_hist[11]+
                                op_hist[12]+op_hist[13]+op_hist[14]+op_hist[15]);
    out[32] = clamp01f((double)spec->energy_uq / 6000.0);
    out[33] = clamp01f((double)spec->heat_uq / 3000.0);
    out[34] = clamp01f(rev_exons / (rev_exons + irr_exons + 1.0));
    out[35] = clamp01f((double)op_hist[OP_METAB_HARVEST] / 6.0);
    out[36] = clamp01f((double)op_hist[OP_METAB_RADIATE] / 6.0);
    out[37] = clamp01f((double)op_hist[OP_MEMBRANE_SEAL] / 6.0);
    out[38] = clamp01f((double)op_hist[OP_SELF_HEAL_ECC] / 6.0);
    out[39] = clamp01f((double)hw_popcnt64(spec->membrane_key) / 64.0);
    out[40] = clamp01f((double)spec->age_ticks / 500.0);
    out[41] = clamp01f(log2(1.0 + (double)spec->generation) / 10.0);
    out[42] = clamp01f(log2(1.0 + (double)spec->offspring_count) / 8.0);
    out[43] = clamp01f((double)op_hist[OP_REV_TOFFOLI] / 6.0);
    out[44] = clamp01f((double)op_hist[OP_REV_FREDKIN] / 6.0);
    out[45] = clamp01f((double)op_hist[OP_REV_GFNI_FOLD] / 6.0);
    out[46] = clamp01f((double)op_hist[OP_IRR_COPY_A_TO_B] / 6.0);
    out[47] = clamp01f((double)op_hist[OP_IRR_COPY_B_TO_A] / 6.0);

    /* [48..63]: Galois-field ribosome, centromere speciation & Weismann traits */
    out[48] = clamp01f((double)spec->ribosome_rank / 8.0);
    out[49] = (spec->ribosome_rank == 8u) ? 1.0f : 0.0f;
    out[50] = clamp01f((double)hw_popcnt64(spec->ribosome_mat) / 64.0);
    out[51] = clamp01f((double)hw_popcnt64(spec->ribosome_inv) / 64.0);
    out[52] = clamp01f((double)spec->ribosome_bias / 255.0);
    out[53] = spec->centromere_irreducible ? 1.0f : 0.0f;
    out[54] = clamp01f((double)spec->centromere_poly / 255.0);
    out[55] = clamp01f((double)hw_popcnt64((uint64_t)spec->centromere_poly) / 8.0);
    out[56] = clamp01f((double)active_exons / 64.0);
    out[57] = clamp01f(1.0 - (double)active_exons / 64.0); /* Intron vault ratio */
    out[58] = clamp01f((double)op_hist[OP_RETRO_TRANSCRIBE] / 5.0);
    out[59] = clamp01f((double)op_hist[OP_RIBOSOME_MUTATE] / 5.0);
    out[60] = clamp01f((double)op_hist[OP_CENTROMERE_DRIFT] / 5.0);
    out[61] = clamp01f((double)op_hist[OP_IRR_CLMUL_SYNTH] / 5.0);
    out[62] = clamp01f((double)op_hist[OP_IRR_PEXT_SPLICE] / 5.0);
    out[63] = clamp01f((double)op_hist[OP_IRR_PDEP_SCATTER] / 5.0);

    /* [64..79]: Social, kin, holobiont, mating & emergent abstention traits */
    double tot_enc = (double)spec->mate_count + (double)spec->mate_refusals + 1.0;
    out[64] = clamp01f((double)spec->mate_count / tot_enc);
    out[65] = clamp01f((double)spec->mate_refusals / tot_enc); /* Abstention / selectivity index */
    out[66] = (spec->mate_count == 0u && spec->mate_refusals > 0u) ? 1.0f : 0.0f; /* Celibate marker */
    out[67] = clamp01f((double)op_hist[OP_HOLOBIONT_BOND] / 5.0);
    out[68] = (spec->holobiont_root != spec->slot_index) ? 1.0f : 0.0f;
    out[69] = clamp01f((double)op_hist[OP_BRANCH_TZCNT] / 5.0);
    out[70] = clamp01f((double)op_hist[OP_STIGMERGY_EMIT] / 5.0);
    out[71] = clamp01f((double)op_hist[OP_STIGMERGY_SENSE] / 5.0);
    out[72] = clamp01f((double)op_hist[OP_MITOSIS_FORK] / 5.0);
    out[73] = clamp01f((double)op_hist[OP_SLEEP_SPORE] / 5.0);
    out[74] = clamp01f((double)hw_popcnt64((uint64_t)spec->ultrametric_addr) / 32.0);
    out[75] = clamp01f((double)(spec->ultrametric_addr ? hw_tzcnt64((uint64_t)spec->ultrametric_addr) : 32ULL) / 32.0);
    out[76] = clamp01f((double)(spec->ultrametric_addr & 0xFFu) / 255.0);
    out[77] = clamp01f((double)((spec->ultrametric_addr >> 8) & 0xFFu) / 255.0);
    out[78] = clamp01f((double)((spec->ultrametric_addr >> 16) & 0xFFu) / 255.0);
    out[79] = clamp01f((double)((spec->ultrametric_addr >> 24) & 0xFFu) / 255.0);

    /* [80..95]: 16-band expressed genome bit-spectral signature */
    for (int b = 0; b < 16; b++) {
        uint32_t acc = 0u;
        for (int k = 0; k < 4; k++) {
            acc += (uint32_t)spec->expressed_opcodes[b * 4 + k];
        }
        out[80 + b] = clamp01f((double)acc / (4.0 * 255.0));
    }

    /* [96..111]: Structural complexity, Assembly Index, Strand alpha/beta divergence */
    double ent_a = metrology_shannon_entropy(spec->genome, 64u);
    double ent_b = metrology_shannon_entropy(spec->genome + 64u, 64u);
    uint32_t diff_ab = 0u;
    for (int i = 0; i < 64; i++) {
        diff_ab += (uint32_t)hw_popcnt64((uint64_t)(spec->genome[i] ^ spec->genome[64 + i]));
    }
    uint32_t autocorr = 0u;
    for (int i = 0; i < 63; i++) {
        autocorr += (uint32_t)(8ULL - hw_popcnt64((uint64_t)(spec->expressed_opcodes[i] ^ spec->expressed_opcodes[i + 1])));
    }
    out[96]  = clamp01f((double)spec->assembly_index / 32.0);
    out[97]  = clamp01f((double)spec->causal_active_bytes / 64.0);
    out[98]  = clamp01f(ent_a / 8.0);
    out[99]  = clamp01f(ent_b / 8.0);
    out[100] = clamp01f((double)diff_ab / 512.0);
    out[101] = clamp01f((double)autocorr / (63.0 * 8.0));
    out[102] = clamp01f((double)spec->jit_code_len / 2048.0);
    for (int k = 0; k < 9; k++) {
        uint64_t h = gf2_clmul_hash64(spec->genome + k * 7, 16u, spec->centromere_poly, 0);
        out[103 + k] = clamp01f((double)(h & 0xFFFFULL) / 65535.0);
    }
}
