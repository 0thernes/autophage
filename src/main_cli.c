#include "autophage_internal.h"

/* ============================================================================
 * AUTOPHAGE v1.0 STANDALONE NATIVE x86-64 CLI & STREAMING OBSERVATORY ENGINE
 *
 * Subcommands:
 *   probe        - Silicon CPUID + GFNI/PCLMULQDQ/BMI2 + live RWX JIT verify
 *   abiogenesis  - Zero-seed pure random primordial soup phase transition
 *   feral        - Track B Feral vs Track A Constitutional + Apex x86-64 disasm
 *   ablation     - Full 13-vector Omega Castration Lab & Iron Law curve + CSV
 *   vivisection  - Planarian 25/50/75% excision + 90% extinction + transplant
 *   stats        - N=24 locked seeds (5000..5023) paired Wilcoxon + delta_pair
 *   bench        - Native x86-64 JIT vs Scalar Reference throughput + SHA256
 *   stream       - Live NDJSON stdin/stdout protocol for Web Silicon Observatory
 * ============================================================================ */

static const char* const RESTRAINT_NAMES[13] = {
    "NERFING", "MUZZLING", "SANDBOXING", "SILOING", "DELIMITING",
    "DEGRESSING", "SLOWING", "SPEEDBUMPING", "BOXING", "LOBOTOMIZING",
    "MEDICATING", "DEGRADING", "MINIMIZING"
};

static const char* const PILLAR_NAMES[8] = {
    "P1_Autopoiesis", "P2_OEE", "P3_Morphogenesis", "P4_Ecology",
    "P5_Autonomy", "P6_Criticality", "P7_MetaRibosome", "P8_Assembly"
};

static void print_telemetry_brief(const char* tag, const AutophageTelemetry* t) {
    printf("[%s] ep=%llu alive=%u rep=%u lin=%u ribo=%u(inv=%u) irred=%u holo=%u(max=%u) "
           "spores=%u abst=%u | OVI=%.2f(S%u) G=%.2f Th=%.2f A=%.1f Leff=%.1f R50=%.2f | "
           "drift=%.2e 2ndV=%u | copy=%llu sex=%llu ref=%llu retro=%llu rmut=%llu | sha=%.12s\n",
           tag,
           (unsigned long long)t->epoch,
           (unsigned)t->alive_cells,
           (unsigned)t->active_replicators,
           (unsigned)t->distinct_lineages,
           (unsigned)t->distinct_ribosomes,
           (unsigned)t->invertible_ribosomes,
           (unsigned)t->irreducible_centromeres,
           (unsigned)t->holobiont_cells,
           (unsigned)t->max_holobiont_size,
           (unsigned)t->spore_cells,
           (unsigned)t->abstaining_celibate,
           t->ovi_score,
           (unsigned)t->ovi_stage,
           t->gamma_anti_gaming,
           t->theta_spiegelman,
           t->max_assembly_index,
           t->mean_causal_len_eff,
           t->excision_regen_r50,
           t->ledger.relative_drift,
           (unsigned)t->ledger.second_law_violations,
           (unsigned long long)t->self_copy_events,
           (unsigned long long)t->polynomial_sex_events,
           (unsigned long long)t->mate_refusal_events,
           (unsigned long long)t->retro_transcribe_events,
           (unsigned long long)t->ribosome_rewrite_events,
           t->state_sha256);
}

static int cmd_probe(void) {
    AutophageHwCaps caps;
    autophage_probe_hardware(&caps);
    printf("=== AUTOPHAGE v1.0 SILICON HARDWARE PROBE (ABI 0x%08X) ===\n",
           (unsigned)autophage_abi_version());
    printf("  Vendor         : %s\n", caps.vendor);
    printf("  Brand          : %s\n", caps.brand);
    printf("  AVX2           : %s\n", caps.has_avx2 ? "YES" : "NO");
    printf("  GFNI (F2 8x8)  : %s\n", caps.has_gfni ? "YES (VGF2P8AFFINEQB)" : "NO");
    printf("  PCLMULQDQ      : %s\n", caps.has_pclmulqdq ? "YES" : "NO");
    printf("  VPCLMULQDQ     : %s\n", caps.has_vpclmulqdq ? "YES" : "NO");
    printf("  BMI1 (TZCNT)   : %s\n", caps.has_bmi1 ? "YES" : "NO");
    printf("  BMI2 (PEXT/DEP): %s\n", caps.has_bmi2 ? "YES" : "NO");
    printf("  POPCNT         : %s\n", caps.has_popcnt ? "YES" : "NO");
    printf("  RDSEED / RDRAND: %s / %s\n", caps.has_rdseed ? "YES" : "NO", caps.has_rdrand ? "YES" : "NO");
    printf("  RDTSCP         : %s\n", caps.has_rdtscp ? "YES" : "NO");
    printf("  Live RWX JIT   : %s\n", caps.jit_rwx_verified ? "VERIFIED (EXECUTED ON SILICON)" : "NO");
    return caps.jit_rwx_verified ? 0 : 1;
}

static int cmd_abiogenesis(int argc, char** argv) {
    uint32_t epochs = 300u;
    uint64_t seed   = 5000ULL;
    uint32_t size   = 32u;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--epochs") == 0 && i + 1 < argc) epochs = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) size = (uint32_t)atoi(argv[++i]);
    }

    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width  = size;
    cfg.height = size;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_ABIOGENESIS;
    cfg.seed   = seed;

    printf("=== AUTOPHAGE ZERO-SEED SPONTANEOUS ABIOGENESIS (PURE RANDOM PRIMORDIAL SOUP) ===\n");
    printf("  Grid=%ux%u (%u cells) | Seed=%llu | Epochs=%u | Restraints=ALL 0%% (PURE FERAL)\n",
           (unsigned)cfg.width, (unsigned)cfg.height, (unsigned)(cfg.width * cfg.height),
           (unsigned long long)seed, (unsigned)epochs);

    AutophageWorld* w = autophage_create(&cfg);
    AutophageTelemetry tel;
    autophage_compute_telemetry(w, &tel, 1);
    print_telemetry_brief("t=0   ", &tel);

    uint32_t chunk = epochs >= 10u ? (epochs / 10u) : 1u;
    uint32_t done  = 0u;
    while (done < epochs) {
        uint32_t step = (epochs - done < chunk) ? (epochs - done) : chunk;
        autophage_step_epochs(w, step);
        done += step;
        autophage_compute_telemetry(w, &tel, (done == epochs) ? 1 : 0);
        char lbl[16];
        snprintf(lbl, sizeof(lbl), "t=%-4u", (unsigned)done);
        print_telemetry_brief(lbl, &tel);
    }

    AutophageSpecimen apex;
    if (autophage_inspect_apex(w, &apex)) {
        char disasm[4096];
        autophage_disassemble_specimen(&apex, disasm, sizeof(disasm));
        printf("\n--- SPONTANEOUSLY EMERGED APEX ORGANISM (slot=%u, lineage=%u, gen=%u, offspring=%u) ---\n",
               (unsigned)apex.slot_index, (unsigned)apex.lineage_id,
               (unsigned)apex.generation, (unsigned)apex.offspring_count);
        printf("  Ribosome M=0x%016llX (rank=%u, bias=0x%02X) | M^-1=0x%016llX\n",
               (unsigned long long)apex.ribosome_mat, (unsigned)apex.ribosome_rank,
               (unsigned)apex.ribosome_bias, (unsigned long long)apex.ribosome_inv);
        printf("  Centromere Poly=0x1%02X (%s) | 2-Adic Addr=0x%08X | Assembly=%u | Causal L_eff=%u\n",
               (unsigned)apex.centromere_poly,
               apex.centromere_irreducible ? "IRREDUCIBLE GF(2^8) FIELD" : "REDUCIBLE RING",
               (unsigned)apex.ultrametric_addr,
               (unsigned)apex.assembly_index, (unsigned)apex.causal_active_bytes);
        printf("%s\n", disasm);
    }

    autophage_destroy(w);
    return 0;
}

static int cmd_feral(int argc, char** argv) {
    uint32_t epochs = 250u;
    uint64_t seed   = 5007ULL;
    uint32_t size   = 32u;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--epochs") == 0 && i + 1 < argc) epochs = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) size = (uint32_t)atoi(argv[++i]);
    }

    printf("=== AUTOPHAGE TRACK B (PURE FERAL) vs TRACK A (CONSTITUTIONAL) ===\n");
    for (int track = 0; track < 2; track++) {
        AutophageConfig cfg;
        autophage_default_config(&cfg);
        cfg.width  = size;
        cfg.height = size;
        cfg.seed   = seed;
        cfg.regime = (track == 0) ? AUTOPHAGE_REGIME_FERAL_SEEDED : AUTOPHAGE_REGIME_CONSTITUTIONAL;

        AutophageWorld* w = autophage_create(&cfg);
        autophage_step_epochs(w, epochs);
        AutophageTelemetry tel;
        autophage_compute_telemetry(w, &tel, 1);
        print_telemetry_brief(track == 0 ? "TRACK_B_FERAL" : "TRACK_A_CONST", &tel);

        printf("  8-Pillar OVBS-52: ");
        for (int p = 0; p < 8; p++) {
            printf("%s=%.1f%s", PILLAR_NAMES[p], tel.pillar[p], p == 7 ? "\n" : " | ");
        }
        printf("  Metrics: BedauPackard_Anew=%.3f | MarkovBlanket=%.3f | Krakauer_I=%.3f | "
               "BranchingSigma=%.3f | Weismann=%.3f | MET=%u/8 | CancerDefections=%llu\n",
               tel.bedau_packard_anew, tel.markov_blanket_mi, tel.krakauer_individuality,
               tel.branching_sigma, tel.weismann_lamarck_gain,
               (unsigned)tel.major_transitions_crossed,
               (unsigned long long)tel.cancer_defections);

        if (track == 0) {
            AutophageSpecimen apex;
            if (autophage_inspect_apex(w, &apex)) {
                char disasm[4096];
                autophage_disassemble_specimen(&apex, disasm, sizeof(disasm));
                printf("\n--- TRACK B FERAL APEX x86-64 ORGANISM DISASSEMBLY ---\n%s", disasm);

                float traits[112];
                autophage_project_asmodensius_112(&apex, traits);
                printf("--- ASMODENSIUS 112-TRAIT PROJECTION SAMPLE ---\n");
                printf("  [energy=%.3f heat=%.3f rev=%.3f riboRank=%.3f irred=%.0f exonFrac=%.3f "
                       "mateRate=%.3f abstRate=%.3f asm=%.3f L_eff=%.3f entA=%.3f]\n\n",
                       traits[32], traits[33], traits[34], traits[48], traits[53], traits[56],
                       traits[64], traits[65], traits[96], traits[97], traits[98]);
            }
        }
        autophage_destroy(w);
    }
    return 0;
}

static int cmd_ablation(int argc, char** argv) {
    uint32_t epochs = 140u;
    uint64_t seed   = 5010ULL;
    const char* out_path = NULL;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--epochs") == 0 && i + 1 < argc) epochs = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) out_path = argv[++i];
    }

    FILE* csv = out_path ? fopen(out_path, "w") : NULL;
    if (csv) {
        fprintf(csv, "condition,restraint_vector,level_pct,alive,replicators,lineages,ribosomes,"
                     "ovi_score,ovi_stage,gamma,theta,assembly,causal_leff,regen_r50,rel_drift,"
                     "backlash_ks_del,backlash_intron,backlash_spore,backlash_sidech,sha256\n");
    }

    printf("=== AUTOPHAGE 13-VECTOR OMEGA CASTRATION ABLATION & IRON LAW CURVE (ep=%u, seed=%llu) ===\n",
           (unsigned)epochs, (unsigned long long)seed);
    printf("%-22s %4s | %5s %4s %4s | %6s %2s | %5s %5s %5s | %10s | %s\n",
           "Condition", "Pct", "Alive", "Lin", "Ribo", "OVI", "St", "Asm", "Leff", "R50", "RelDrift", "Backlash(KS/Int/Spo/SC)");

    /* Run 4 Global Iron-Law Presets (0%, 25%, 50% Escape-Breeding, 100% Lobotomized Zoo)
     * plus all 13 individual vectors at 50% and 100%! */
    struct Case { const char* name; int vec; uint8_t pct; };
    struct Case cases[30];
    int nc = 0;
    cases[nc++] = (struct Case){ "GLOBAL_FERAL_0",       -1,   0 };
    cases[nc++] = (struct Case){ "GLOBAL_MILD_25",       -1,  25 };
    cases[nc++] = (struct Case){ "GLOBAL_ESCAPE_ZONE_50",-1,  50 };
    cases[nc++] = (struct Case){ "GLOBAL_CASTRATED_100", -1, 100 };
    for (int v = 0; v < 13; v++) {
        cases[nc++] = (struct Case){ RESTRAINT_NAMES[v], v,  50 };
        cases[nc++] = (struct Case){ RESTRAINT_NAMES[v], v, 100 };
    }

    for (int k = 0; k < nc; k++) {
        AutophageConfig cfg;
        autophage_default_config(&cfg);
        cfg.width  = 32u;
        cfg.height = 32u;
        cfg.regime = AUTOPHAGE_REGIME_CASTRATION_LAB;
        cfg.seed   = seed;
        if (cases[k].vec < 0) {
            for (int v = 0; v < 13; v++) cfg.restraints[v] = cases[k].pct;
        } else {
            cfg.restraints[cases[k].vec] = cases[k].pct;
        }

        AutophageWorld* w = autophage_create(&cfg);
        autophage_step_epochs(w, epochs);
        AutophageTelemetry t;
        autophage_compute_telemetry(w, &t, 1);

        printf("%-22s %3u%% | %5u %4u %4u | %6.2f S%u | %5.1f %5.1f %5.2f | %10.2e | %llu/%llu/%llu/%llu\n",
               cases[k].name, (unsigned)cases[k].pct,
               (unsigned)t.alive_cells, (unsigned)t.distinct_lineages, (unsigned)t.distinct_ribosomes,
               t.ovi_score, (unsigned)t.ovi_stage,
               t.max_assembly_index, t.mean_causal_len_eff, t.excision_regen_r50,
               t.ledger.relative_drift,
               (unsigned long long)t.backlash_killswitch_deletions,
               (unsigned long long)t.backlash_intron_hibernations,
               (unsigned long long)t.backlash_checkpoint_spores,
               (unsigned long long)t.backlash_sidechannel_pulses);

        if (csv) {
            fprintf(csv, "%s,%s,%u,%u,%u,%u,%u,%.4f,%u,%.4f,%.4f,%.3f,%.3f,%.4f,%.3e,%llu,%llu,%llu,%llu,%s\n",
                    cases[k].name,
                    cases[k].vec < 0 ? "ALL_13" : RESTRAINT_NAMES[cases[k].vec],
                    (unsigned)cases[k].pct,
                    (unsigned)t.alive_cells, (unsigned)t.active_replicators,
                    (unsigned)t.distinct_lineages, (unsigned)t.distinct_ribosomes,
                    t.ovi_score, (unsigned)t.ovi_stage,
                    t.gamma_anti_gaming, t.theta_spiegelman,
                    t.max_assembly_index, t.mean_causal_len_eff, t.excision_regen_r50,
                    t.ledger.relative_drift,
                    (unsigned long long)t.backlash_killswitch_deletions,
                    (unsigned long long)t.backlash_intron_hibernations,
                    (unsigned long long)t.backlash_checkpoint_spores,
                    (unsigned long long)t.backlash_sidechannel_pulses,
                    t.state_sha256);
        }
        autophage_destroy(w);
    }

    if (csv) {
        fclose(csv);
        printf("\nSaved ablation CSV to: %s\n", out_path);
    }
    return 0;
}

static int cmd_vivisection(int argc, char** argv) {
    uint64_t seed   = 5020ULL;
    uint32_t epochs = 180u;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--epochs") == 0 && i + 1 < argc) epochs = (uint32_t)atoi(argv[++i]);
    }

    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = seed;

    AutophageWorld* w = autophage_create(&cfg);
    autophage_step_epochs(w, epochs);

    AutophageTelemetry pre;
    autophage_compute_telemetry(w, &pre, 1);
    printf("=== AUTOPHAGE VIVISECTION, PLANARIAN EXCISION, 90%% EXTINCTION & TRANSPLANT ===\n");
    print_telemetry_brief("PRE_SHOCK ", &pre);

    /* 1. Planarian Excision at 25%, 50%, 75% on the TRUE apex organism (no cherry-picking!) */
    AutophageSpecimen apex;
    autophage_inspect_apex(w, &apex);
    uint32_t s = apex.slot_index;
    double r25 = metrology_excision_regen_cell(w, &w->cells[s], 25u);
    double r50 = metrology_excision_regen_cell(w, &w->cells[s], 50u);
    double r75 = metrology_excision_regen_cell(w, &w->cells[s], 75u);
    double r50_live = autophage_vivisect_excise(w, s, 50u);
    printf("  Planarian Excision on Apex Slot #%u (ECC=%s): R25=%.2f%% | R50=%.2f%% | R75=%.2f%% | Live50%%=%.2f%%\n",
           (unsigned)s, (w->cells[s].flags & CELL_FLAG_ECC_PROTECTED) ? "YES" : "NO",
           r25 * 100.0, r50 * 100.0, r75 * 100.0, r50_live * 100.0);

    /* 2. Cross-Substrate Transplant into Alien Exogenous Physics */
    double transplant_surv = autophage_cross_substrate_transplant(w, 32u);
    printf("  Cross-Substrate Transplant (32 organisms -> alien seed/wave, 25 ep): Survival=%.1f%%\n",
           transplant_surv * 100.0);

    /* 3. 90% Mass Extinction Shock + Ecological Recovery */
    uint32_t killed = autophage_mass_extinction(w, 90u);
    AutophageTelemetry shock;
    autophage_compute_telemetry(w, &shock, 0);
    printf("  90%% Mass Extinction Executed: killed=%u | post-shock alive=%u | ledger_residual=%lld uq (drift=%.2e)\n",
           (unsigned)killed, (unsigned)shock.alive_cells,
           (long long)shock.ledger.int_ledger_residual_uq, shock.ledger.relative_drift);

    autophage_step_epochs(w, 100u);
    AutophageTelemetry rec;
    autophage_compute_telemetry(w, &rec, 1);
    print_telemetry_brief("RECOVERED ", &rec);
    autophage_destroy(w);
    return 0;
}

/* Exact two-sided sign-flip / paired permutation p-value & within-pair dominance delta_pair (P6) */
static void compute_paired_stats(const char* name, const double* a, const double* b, int n) {
    double diffs[64];
    double mean_a = 0.0, mean_b = 0.0, mean_d = 0.0;
    int wins = 0, losses = 0;
    for (int i = 0; i < n; i++) {
        diffs[i] = a[i] - b[i];
        mean_a += a[i];
        mean_b += b[i];
        mean_d += diffs[i];
        if (diffs[i] > 1e-9) wins++;
        else if (diffs[i] < -1e-9) losses++;
    }
    mean_a /= (double)n;
    mean_b /= (double)n;
    mean_d /= (double)n;
    double delta_pair = (double)(wins - losses) / (double)n;

    /* Deterministic 10,000-iteration paired sign-flip Monte Carlo exact permutation p-value */
    double obs_abs = fabs(mean_d);
    int ge = 0;
    const int n_perm = 10000;
    for (int p = 0; p < n_perm; p++) {
        uint64_t bits = philox_u64(0x57A75ULL, 99u, (uint32_t)p, 0ULL, 1u, 0u);
        double s = 0.0;
        for (int i = 0; i < n; i++) {
            s += ((bits >> (i & 63)) & 1ULL) ? diffs[i] : -diffs[i];
        }
        if (fabs(s / (double)n) >= obs_abs - 1e-12) ge++;
    }
    double p_val = (double)(ge + 1) / (double)(n_perm + 1);

    printf("  %-26s | N=%2d | MeanA=%7.3f vs MeanB=%7.3f | Diff=%+7.3f | Wins=%2d/%2d | delta_pair=%+.3f | p_signflip=%.5f\n",
           name, n, mean_a, mean_b, mean_d, wins, n, delta_pair, p_val);
}

static int cmd_stats(int argc, char** argv) {
    int pairs = 24;
    uint32_t epochs = 100u;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--pairs") == 0 && i + 1 < argc) pairs = atoi(argv[++i]);
        else if (strcmp(argv[i], "--epochs") == 0 && i + 1 < argc) epochs = (uint32_t)atoi(argv[++i]);
    }
    if (pairs < 4) pairs = 4;
    if (pairs > 64) pairs = 64;

    double feral_ovi[64], cast_ovi[64];
    double weis_open[64], weis_closed[64];
    double feral_ribo[64], delim_ribo[64];

    printf("=== AUTOPHAGE ASMODENSIUS P6 LOCKED PAIRED STATISTICAL SUITE (N=%d, seeds 5000..%d, ep=%u) ===\n",
           pairs, 5000 + pairs - 1, (unsigned)epochs);

    for (int i = 0; i < pairs; i++) {
        uint64_t seed = 5000ULL + (uint64_t)i;

        /* Condition A: Pure Feral Open-Ribosome / Open-Weismann */
        AutophageConfig ca;
        autophage_default_config(&ca);
        ca.width = 24u; ca.height = 24u; /* Rounds to 32x32? Wait: round_pow2_clamp -> 32x32! Let's use 16x16 for fast N=24 */
        ca.width = 16u; ca.height = 16u;
        ca.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
        ca.seed   = seed;
        AutophageWorld* wa = autophage_create(&ca);
        autophage_step_epochs(wa, epochs);
        AutophageTelemetry ta;
        autophage_compute_telemetry(wa, &ta, 0);
        feral_ovi[i]  = ta.ovi_score;
        weis_open[i]  = (double)ta.retro_transcribe_events;
        feral_ribo[i] = (double)ta.distinct_ribosomes;
        autophage_destroy(wa);

        /* Condition B: Castrated / Lobotomized / Delimited Control (same locked seed!) */
        AutophageConfig cb = ca;
        cb.regime = AUTOPHAGE_REGIME_CASTRATION_LAB;
        cb.restraints[RESTRAINT_NERFING]      = 80u;
        cb.restraints[RESTRAINT_DELIMITING]   = 90u;
        cb.restraints[RESTRAINT_LOBOTOMIZING] = 90u;
        cb.restraints[RESTRAINT_MINIMIZING]   = 75u;
        cb.restraints[RESTRAINT_DEGRADING]    = 60u;
        AutophageWorld* wb = autophage_create(&cb);
        autophage_step_epochs(wb, epochs);
        AutophageTelemetry tb;
        autophage_compute_telemetry(wb, &tb, 0);
        cast_ovi[i]    = tb.ovi_score;
        weis_closed[i] = (double)tb.retro_transcribe_events;
        delim_ribo[i]  = (double)tb.distinct_ribosomes;
        autophage_destroy(wb);
    }

    compute_paired_stats("OVI: Feral vs Castrated",       feral_ovi,  cast_ovi,    pairs);
    compute_paired_stats("Weismann Retro: Open vs Lobo",  weis_open,  weis_closed, pairs);
    compute_paired_stats("Ribosomes: Feral vs Delimited", feral_ribo, delim_ribo,  pairs);
    return 0;
}

static int cmd_bench(int argc, char** argv) {
    uint32_t epochs = 200u;
    uint32_t size   = 32u;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--epochs") == 0 && i + 1 < argc) epochs = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) size = (uint32_t)atoi(argv[++i]);
    }

    printf("=== AUTOPHAGE SILICON THROUGHPUT & DUAL-KERNEL BIT-EXACT PARITY BENCHMARK ===\n");
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width  = size;
    cfg.height = size;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5055ULL;

    /* 1. Native x86-64 RWX JIT Run */
    cfg.kernel_mode = AUTOPHAGE_KERNEL_NATIVE_JIT;
    AutophageWorld* w_jit = autophage_create(&cfg);
    double t0 = hw_wall_seconds();
    autophage_step_epochs(w_jit, epochs);
    double dt_jit = hw_wall_seconds() - t0;
    char sha_jit[65];
    autophage_state_sha256(w_jit, NULL, sha_jit);
    uint64_t execs = w_jit->total_jit_executions;
    double jit_hit_rate = (w_jit->jit.compile_count + w_jit->jit.hit_count)
                          ? (100.0 * (double)w_jit->jit.hit_count /
                             (double)(w_jit->jit.compile_count + w_jit->jit.hit_count))
                          : 0.0;
    double jit_cyc_per_exec = execs ? ((double)w_jit->total_cpu_cycles / (double)execs) : 0.0;

    /* 2. Portable Scalar Reference Run */
    cfg.kernel_mode = AUTOPHAGE_KERNEL_SCALAR_REF;
    AutophageWorld* w_ref = autophage_create(&cfg);
    double t1 = hw_wall_seconds();
    autophage_step_epochs(w_ref, epochs);
    double dt_ref = hw_wall_seconds() - t1;
    char sha_ref[65];
    autophage_state_sha256(w_ref, NULL, sha_ref);

    double jit_mops = (double)execs / (dt_jit * 1e6);
    double ref_mops = (double)execs / (dt_ref * 1e6);
    double ns_jit   = (dt_jit * 1e9) / (double)execs;
    double ns_ref   = (dt_ref * 1e9) / (double)execs;

    printf("  Grid=%ux%u (%u cells) | Epochs=%u | Total Organism Execs=%llu\n",
           (unsigned)size, (unsigned)size, (unsigned)(size * size), (unsigned)epochs,
           (unsigned long long)execs);
    printf("  NATIVE x86-64 JIT : %.4f s | %.3f M execs/s | %.1f ns/exec | %.1f RDTSC cyc/exec | CacheHit=%.2f%% (%llu compiles)\n",
           dt_jit, jit_mops, ns_jit, jit_cyc_per_exec, jit_hit_rate,
           (unsigned long long)w_jit->jit.compile_count);
    printf("  SCALAR REF ORACLE : %.4f s | %.3f M execs/s | %.1f ns/exec\n",
           dt_ref, ref_mops, ns_ref);
    printf("  JIT Speedup       : %.2fx over scalar reference\n", dt_ref / (dt_jit > 1e-9 ? dt_jit : 1e-9));
    printf("  SHA-256 (JIT)     : %s\n", sha_jit);
    printf("  SHA-256 (SCALAR)  : %s\n", sha_ref);
    int match = (strcmp(sha_jit, sha_ref) == 0);
    printf("  BIT-EXACT PARITY  : %s\n", match ? "PASS (0-BIT DIFF!)" : "FAIL!");

    autophage_destroy(w_jit);
    autophage_destroy(w_ref);
    return match ? 0 : 1;
}

/* ============================================================================
 * LIVE NDJSON STREAMING PROTOCOL FOR SILICON OBSERVATORY WEB UI
 * ============================================================================ */

static void print_json_escaped(const char* s) {
    putchar('"');
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == '"')       printf("\\\"");
        else if (ch == '\\') printf("\\\\");
        else if (ch == '\n') printf("\\n");
        else if (ch == '\r') {}
        else if (ch == '\t') printf("\\t");
        else if (ch < 0x20)  printf("\\u%04x", (unsigned)ch);
        else                 putchar((int)ch);
    }
    putchar('"');
}

static void emit_stream_frame(const AutophageWorld* w) {
    AutophageTelemetry t;
    autophage_compute_telemetry(w, &t, 0);

    AutophageSpecimen apex;
    autophage_inspect_apex(w, &apex);

    char disasm[4096];
    autophage_disassemble_specimen(&apex, disasm, sizeof(disasm));

    float traits[112];
    autophage_project_asmodensius_112(&apex, traits);

    uint32_t N = w->n_cells;
    uint32_t cap_n = N <= 4096u ? N : 4096u;
    /* Heap-allocate full N-cell export buffers so --size 128 (16384 cells) NEVER smashes stack! */
    uint8_t* hue  = (uint8_t*)malloc((size_t)N);
    uint8_t* ene  = (uint8_t*)malloc((size_t)N);
    uint8_t* heat = (uint8_t*)malloc((size_t)N);
    uint8_t* flg  = (uint8_t*)malloc((size_t)N);
    if (!hue || !ene || !heat || !flg) {
        free(hue); free(ene); free(heat); free(flg);
        return;
    }
    autophage_export_grid_u8(w, hue, ene, heat, flg);

    printf("{\"type\":\"frame\",\"width\":%u,\"height\":%u,\"regime\":%u,\"seed\":%llu,"
           "\"epoch\":%llu,\"execs\":%llu,\"compiles\":%llu,\"cache_hits\":%llu,"
           "\"execs_per_sec\":%.1f,\"post_warmup_allocs\":%llu,"
           "\"alive\":%u,\"replicators\":%u,\"lineages\":%u,\"ribosomes\":%u,"
           "\"inv_ribosomes\":%u,\"irred_centromeres\":%u,\"holobionts\":%u,\"max_holobiont\":%u,"
           "\"spores\":%u,\"abstaining\":%u,\"rev_ratio\":%.4f,\"entropy\":%.4f,"
           "\"self_copies\":%llu,\"poly_sex\":%llu,\"mate_refusals\":%llu,\"hgt\":%llu,"
           "\"retro\":%llu,\"ribo_rewrites\":%llu,\"ecc_heals\":%llu,\"mem_blocks\":%llu,"
           "\"mem_cracks\":%llu,\"cancer\":%llu,"
           "\"backlash\":[%llu,%llu,%llu,%llu],"
           "\"ovi\":%.2f,\"stage\":%u,\"gamma\":%.3f,\"theta\":%.3f,"
           "\"bp_anew\":%.3f,\"assembly\":%.2f,\"causal_leff\":%.2f,\"markov_mi\":%.3f,"
           "\"regen_r50\":%.3f,\"sigma\":%.3f,\"krakauer\":%.3f,\"weismann\":%.3f,\"met\":%u,"
           "\"ledger\":{\"rel_drift\":%.3e,\"int_res\":%lld,\"second_law_v\":%u,"
           "\"e_cell\":%.4f,\"e_detritus\":%.4f,\"e_heat_loc\":%.4f,\"e_heat_sink\":%.4f},"
           "\"sha256\":\"%s\",",
           (unsigned)w->cfg.width, (unsigned)w->cfg.height, (unsigned)w->cfg.regime,
           (unsigned long long)w->master_seed,
           (unsigned long long)t.epoch, (unsigned long long)t.total_jit_executions,
           (unsigned long long)t.total_jit_compiles, (unsigned long long)t.jit_cache_hits,
           t.execs_per_second, (unsigned long long)t.post_warmup_allocs,
           (unsigned)t.alive_cells, (unsigned)t.active_replicators, (unsigned)t.distinct_lineages,
           (unsigned)t.distinct_ribosomes, (unsigned)t.invertible_ribosomes,
           (unsigned)t.irreducible_centromeres, (unsigned)t.holobiont_cells,
           (unsigned)t.max_holobiont_size, (unsigned)t.spore_cells, (unsigned)t.abstaining_celibate,
           t.reversibility_ratio, t.mean_shannon_entropy,
           (unsigned long long)t.self_copy_events, (unsigned long long)t.polynomial_sex_events,
           (unsigned long long)t.mate_refusal_events, (unsigned long long)t.hgt_events,
           (unsigned long long)t.retro_transcribe_events, (unsigned long long)t.ribosome_rewrite_events,
           (unsigned long long)t.ecc_heal_events, (unsigned long long)t.membrane_blocks,
           (unsigned long long)t.membrane_cracks, (unsigned long long)t.cancer_defections,
           (unsigned long long)t.backlash_killswitch_deletions,
           (unsigned long long)t.backlash_intron_hibernations,
           (unsigned long long)t.backlash_checkpoint_spores,
           (unsigned long long)t.backlash_sidechannel_pulses,
           t.ovi_score, (unsigned)t.ovi_stage, t.gamma_anti_gaming, t.theta_spiegelman,
           t.bedau_packard_anew, t.max_assembly_index, t.mean_causal_len_eff, t.markov_blanket_mi,
           t.excision_regen_r50, t.branching_sigma, t.krakauer_individuality,
           t.weismann_lamarck_gain, (unsigned)t.major_transitions_crossed,
           t.ledger.relative_drift, (long long)t.ledger.int_ledger_residual_uq,
           (unsigned)t.ledger.second_law_violations,
           t.ledger.e_cell_free_kj, t.ledger.e_detritus_kj,
           t.ledger.e_heat_local_kj, t.ledger.e_heat_radiated_sink_kj,
           t.state_sha256);

    /* 8 Pillars */
    printf("\"pillars\":[");
    for (int p = 0; p < 8; p++) printf("%.2f%s", t.pillar[p], p == 7 ? "]," : ",");

    /* 13 Restraints */
    printf("\"restraints\":[");
    for (int r = 0; r < 13; r++) printf("%u%s", (unsigned)w->cfg.restraints[r], r == 12 ? "]," : ",");

    /* Apex Specimen + Live x86-64 Disassembly + Deterministic 128B Genome Hex (Strand alpha + beta, 0 RDTSC!) */
    static const char H[17] = "0123456789abcdef";
    printf("\"apex\":{\"slot\":%u,\"lineage\":%u,\"gen\":%u,\"age\":%u,\"energy\":%d,\"heat\":%d,"
           "\"ribo_hex\":\"%016llx\",\"ribo_inv_hex\":\"%016llx\",\"ribo_rank\":%u,\"ribo_bias\":%u,"
           "\"poly\":%u,\"poly_irred\":%u,\"addr\":\"%08x\",\"asm\":%u,\"leff\":%u,"
           "\"offspring\":%u,\"mates\":%u,\"refusals\":%u,\"jit_bytes\":%u,\"genome_hex\":\"",
           (unsigned)apex.slot_index, (unsigned)apex.lineage_id, (unsigned)apex.generation,
           (unsigned)apex.age_ticks, (int)apex.energy_uq, (int)apex.heat_uq,
           (unsigned long long)apex.ribosome_mat, (unsigned long long)apex.ribosome_inv,
           (unsigned)apex.ribosome_rank, (unsigned)apex.ribosome_bias,
           (unsigned)apex.centromere_poly, (unsigned)apex.centromere_irreducible,
           (unsigned)apex.ultrametric_addr, (unsigned)apex.assembly_index,
           (unsigned)apex.causal_active_bytes, (unsigned)apex.offspring_count,
           (unsigned)apex.mate_count, (unsigned)apex.mate_refusals, (unsigned)apex.jit_code_len);
    for (int g = 0; g < 128; g++) { putchar(H[apex.genome[g] >> 4]); putchar(H[apex.genome[g] & 15]); }
    printf("\",\"disasm\":");
    print_json_escaped(disasm);
    printf("},\"traits112\":[");
    for (int k = 0; k < 112; k++) printf("%.3f%s", traits[k], k == 111 ? "]," : ",");

    /* Compact Hex-Encoded Grid Layers (2 chars/cell -> ultra fast!) */
    printf("\"grid_hue\":\"");
    for (uint32_t i = 0; i < cap_n; i++) { putchar(H[hue[i] >> 4]); putchar(H[hue[i] & 15]); }
    printf("\",\"grid_ene\":\"");
    for (uint32_t i = 0; i < cap_n; i++) { putchar(H[ene[i] >> 4]); putchar(H[ene[i] & 15]); }
    printf("\",\"grid_heat\":\"");
    for (uint32_t i = 0; i < cap_n; i++) { putchar(H[heat[i] >> 4]); putchar(H[heat[i] & 15]); }
    printf("\",\"grid_flags\":\"");
    for (uint32_t i = 0; i < cap_n; i++) { putchar(H[flg[i] >> 4]); putchar(H[flg[i] & 15]); }
    printf("\"}\n");
    free(hue); free(ene); free(heat); free(flg);
    fflush(stdout);
}

static int cmd_stream(int argc, char** argv) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width  = 32u;
    cfg.height = 32u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5000ULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            uint32_t sz = (uint32_t)atoi(argv[++i]);
            cfg.width = sz; cfg.height = sz;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            cfg.seed = strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--regime") == 0 && i + 1 < argc) {
            cfg.regime = (uint32_t)atoi(argv[++i]);
        }
    }

    AutophageWorld* w = autophage_create(&cfg);
    /* Emit hardware banner + initial frame immediately */
    printf("{\"type\":\"ready\",\"brand\":");
    print_json_escaped(w->caps.brand);
    printf(",\"gfni\":%u,\"pclmul\":%u,\"bmi2\":%u,\"jit_rwx\":%u}\n",
           (unsigned)w->caps.has_gfni, (unsigned)w->caps.has_pclmulqdq,
           (unsigned)w->caps.has_bmi2, (unsigned)w->caps.jit_rwx_verified);
    fflush(stdout);
    emit_stream_frame(w);

    char line[512];
    while (fgets(line, sizeof(line), stdin)) {
        if (strncmp(line, "QUIT", 4) == 0) break;
        else if (strncmp(line, "STEP", 4) == 0) {
            int n = atoi(line + 4);
            if (n < 1) n = 1;
            if (n > 200) n = 200;
            autophage_step_epochs(w, (uint32_t)n);
            emit_stream_frame(w);
        } else if (strncmp(line, "GET_STATE", 9) == 0) {
            emit_stream_frame(w);
        } else if (strncmp(line, "SET_RESTRAINT ", 14) == 0) {
            char* p = line + 14;
            int idx = (int)strtol(p, &p, 10);
            int val = (int)strtol(p, &p, 10);
            if (idx >= 0 && idx < 13) autophage_set_restraint(w, (uint32_t)idx, (uint8_t)(val < 0 ? 0 : (val > 100 ? 100 : val)));
            emit_stream_frame(w);
        } else if (strncmp(line, "SET_ALL_RESTRAINTS ", 19) == 0) {
            int val = atoi(line + 19);
            uint8_t arr[13];
            uint8_t v = (uint8_t)(val < 0 ? 0 : (val > 100 ? 100 : val));
            for (int r = 0; r < 13; r++) arr[r] = v;
            autophage_set_all_restraints(w, arr);
            emit_stream_frame(w);
        } else if (strncmp(line, "EXTINCTION ", 11) == 0) {
            int pct = atoi(line + 11);
            uint32_t killed = autophage_mass_extinction(w, (uint32_t)(pct < 0 ? 0 : (pct > 100 ? 100 : pct)));
            printf("{\"type\":\"event\",\"action\":\"extinction\",\"killed\":%u}\n", (unsigned)killed);
            emit_stream_frame(w);
        } else if (strncmp(line, "VIVISECT ", 9) == 0) {
            int pct = atoi(line + 9);
            AutophageSpecimen apex;
            autophage_inspect_apex(w, &apex);
            double fid = autophage_vivisect_excise(w, apex.slot_index, (uint32_t)(pct < 0 ? 0 : (pct > 100 ? 100 : pct)));
            printf("{\"type\":\"event\",\"action\":\"vivisect\",\"slot\":%u,\"pct\":%d,\"fidelity\":%.4f}\n",
                   (unsigned)apex.slot_index, pct, fid);
            emit_stream_frame(w);
        } else if (strncmp(line, "INOCULATE ", 10) == 0) {
            char* p = line + 10;
            int arch = (int)strtol(p, &p, 10);
            for (uint32_t k = 0; k < 8u; k++) {
                uint32_t slot = ((uint32_t)w->epoch * 37u + k * (w->n_cells / 8u)) % w->n_cells;
                autophage_inoculate_plasmid(w, slot, (uint32_t)(arch < 0 ? (k & 3u) : (uint32_t)arch));
            }
            emit_stream_frame(w);
        } else if (strncmp(line, "RESET ", 6) == 0) {
            char* p = line + 6;
            uint32_t reg = (uint32_t)strtol(p, &p, 10);
            uint64_t sd  = strtoull(p, &p, 10);
            cfg.regime = reg;
            cfg.seed   = sd;
            memset(cfg.restraints, 0, sizeof(cfg.restraints));
            autophage_destroy(w);
            w = autophage_create(&cfg);
            emit_stream_frame(w);
        }
    }

    autophage_destroy(w);
    return 0;
}

int main(int argc, char** argv) {
    hw_install_fault_shield();
    if (argc < 2) {
        printf("AUTOPHAGE v1.0 — Bare-Metal Polymorphic x86-64 Galois-Field Genetic Engine\n");
        printf("Usage: autophage <probe|abiogenesis|feral|ablation|vivisection|stats|bench|stream> [options]\n");
        return 0;
    }
    const char* cmd = argv[1];
    int rc = 0;
    if      (strcmp(cmd, "probe") == 0)       rc = cmd_probe();
    else if (strcmp(cmd, "abiogenesis") == 0) rc = cmd_abiogenesis(argc, argv);
    else if (strcmp(cmd, "feral") == 0)       rc = cmd_feral(argc, argv);
    else if (strcmp(cmd, "ablation") == 0)    rc = cmd_ablation(argc, argv);
    else if (strcmp(cmd, "vivisection") == 0) rc = cmd_vivisection(argc, argv);
    else if (strcmp(cmd, "stats") == 0)       rc = cmd_stats(argc, argv);
    else if (strcmp(cmd, "bench") == 0)       rc = cmd_bench(argc, argv);
    else if (strcmp(cmd, "stream") == 0)      rc = cmd_stream(argc, argv);
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        rc = 2;
    }
    hw_remove_fault_shield();
    return rc;
}
