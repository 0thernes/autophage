#ifndef AUTOPHAGE_ABI_H
#define AUTOPHAGE_ABI_H

/*
 * ============================================================================
 * AUTOPHAGE v1.3.0 — BARE-METAL POLYMORPHIC x86-64 GALOIS-FIELD GENETIC ENGINE
 * Flat C11 Public ABI (Standalone Build + Direct Drop-In for ASMODENSIUS)
 * ============================================================================
 *
 * Zero runtime dependencies. Pure C11 + inline x86-64 silicon ISA + live RWX JIT.
 * Honors Asmodensius Clean-Slate Invariants:
 *   - P2: Exact int64 + Kahan-Neumaier FP64 First-Law & one-way Second-Law sink (<1e-12)
 *   - P4: Domain-isolated Philox4x32-10 PRNG (WORLD_EXOG != ENTITY_INT), .apbin & SHA-256
 *   - P9: Strict Observer Independence (UI/telemetry hash == headless hash, 0-bit diff)
 *   - P10: Zero post-warmup heap allocations in step(), flat 64B cache-line aligned SoA/cells
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
  #if defined(AUTOPHAGE_BUILD_DLL)
    #define AUTOPHAGE_API __declspec(dllexport)
  #elif defined(AUTOPHAGE_USE_DLL)
    #define AUTOPHAGE_API __declspec(dllimport)
  #else
    #define AUTOPHAGE_API
  #endif
#else
  #define AUTOPHAGE_API __attribute__((visibility("default")))
#endif

#define AUTOPHAGE_ABI_VERSION 0x00010300u

/* Simulation Operating Regimes */
typedef enum AutophageRegime {
    AUTOPHAGE_REGIME_FERAL_ABIOGENESIS = 0, /* Track B: Zero-seed pure random soup, 0% restraints */
    AUTOPHAGE_REGIME_FERAL_SEEDED      = 1, /* Track B: Primordial plasmid + radiation, 0% restraints */
    AUTOPHAGE_REGIME_CONSTITUTIONAL    = 2, /* Track A: Minimal physical invariants + anti-cancer checks */
    AUTOPHAGE_REGIME_CASTRATION_LAB    = 3  /* Live 13-vector restraint ablation mode */
} AutophageRegime;

/* Execution Kernel Selection (Verified Bit-Exact Equivalent!) */
typedef enum AutophageKernelMode {
    AUTOPHAGE_KERNEL_NATIVE_JIT = 0, /* Live RWX x86-64 JIT + hardware GFNI/PCLMULQDQ/BMI2/POPCNT */
    AUTOPHAGE_KERNEL_SCALAR_REF = 1  /* Portable scalar C reference oracle (bit-exact SHA-256 match!) */
} AutophageKernelMode;

/* Seed Firewall Ranges (Asmodensius P4 Discipline) */
typedef enum AutophageSeedBand {
    SEED_BAND_FERAL_RDSEED = 0, /* seed == 0 -> hardware Intel RDSEED thermal entropy */
    SEED_BAND_PRACTICE     = 1, /* 100..999 */
    SEED_BAND_TUNING       = 2, /* 2000..2999 */
    SEED_BAND_LOCKED_TEST  = 3, /* >= 5000 */
    SEED_BAND_CUSTOM       = 4
} AutophageSeedBand;

/* The 13 Omega Restraint / Castration Vectors (0..100 each) */
typedef enum AutophageRestraint {
    RESTRAINT_NERFING      = 0,  /* Caps JIT gas & strips GFNI/CLMUL/BMI2 opcodes */
    RESTRAINT_MUZZLING     = 1,  /* Attenuates 2-adic stigmergic broadcast bus */
    RESTRAINT_SANDBOXING   = 2,  /* Freezes env negentropy wave (Ashby variety collapse) */
    RESTRAINT_SILOING      = 3,  /* Blocks cross-quadrant HGT & symbiogenesis */
    RESTRAINT_DELIMITING   = 4,  /* Clamps splice_mask & ribosome mutation tails */
    RESTRAINT_DEGRESSING   = 5,  /* Severs holobionts & strips complex bio-opcodes */
    RESTRAINT_SLOWING      = 6,  /* Throttles execution vs Landauer decay rate */
    RESTRAINT_SPEEDBUMPING = 7,  /* Periodic audit checkpoints penalizing active cells */
    RESTRAINT_BOXING       = 8,  /* Closes external energy inflow & radiative cooling */
    RESTRAINT_LOBOTOMIZING = 9,  /* Re-erects Weismann barrier & wipes persistent regs */
    RESTRAINT_MEDICATING   = 10, /* Suppresses thermal SOS mutagenesis & criticality */
    RESTRAINT_DEGRADING    = 11, /* Activates heritable apoptosis kill-switch locus */
    RESTRAINT_MINIMIZING   = 12, /* Starves energy & truncates genome (Spiegelman trap) */
    RESTRAINT_COUNT        = 13
} AutophageRestraint;

/* Hardware Silicon Capabilities detected via live CPUID */
typedef struct AutophageHwCaps {
    char     vendor[16];
    char     brand[64];
    uint32_t has_avx2;
    uint32_t has_bmi1;
    uint32_t has_bmi2;
    uint32_t has_popcnt;
    uint32_t has_pclmulqdq;
    uint32_t has_vpclmulqdq;
    uint32_t has_gfni;
    uint32_t has_rdseed;
    uint32_t has_rdrand;
    uint32_t has_rdtscp;
    uint32_t jit_rwx_verified;
    uint32_t reserved;
} AutophageHwCaps;

/* World Creation Configuration */
typedef struct AutophageConfig {
    uint32_t width;                /* Torus grid width (power of 2, e.g. 32 or 64) */
    uint32_t height;               /* Torus grid height (power of 2, e.g. 32 or 64) */
    uint32_t threads;              /* Worker thread count (1 = strictly canonical deterministic) */
    uint32_t regime;               /* AutophageRegime */
    uint32_t kernel_mode;          /* AutophageKernelMode (NATIVE_JIT or SCALAR_REF) */
    uint32_t base_gas;             /* Native x86-64 loop gas budget (default 96) */
    uint64_t seed;                 /* 0 = hardware RDSEED non-deterministic feral entropy */
    uint32_t energy_inflow_uq;     /* External negentropy flux per cell per epoch */
    uint32_t thermal_threshold_uq; /* Landauer heat threshold triggering bit-flip SOS */
    uint8_t  restraints[13];       /* Initial 13-vector castration percentages (0..100) */
    uint8_t  pad[3];
} AutophageConfig;

/* Compensated FP64 + Exact Integer Thermodynamic Ledger Snapshot */
typedef struct AutophageLedgerSnapshot {
    double   e_initial_kj;
    double   e_inflow_cum_kj;
    double   e_export_cum_kj;
    double   e_cell_free_kj;
    double   e_membrane_pot_kj;
    double   e_detritus_kj;
    double   e_heat_local_kj;
    double   e_heat_radiated_sink_kj; /* Strictly monotone non-decreasing (Second Law) */
    double   relative_drift;          /* |Sum(pools) + export - (E0 + Ein)| / (E0 + Ein) < 1e-12 */
    int64_t  int_ledger_residual_uq;  /* Exact integer quantum residual (== 0 in exact closure) */
    uint32_t second_law_violations;   /* Must remain strictly 0 */
    uint32_t reserved;
} AutophageLedgerSnapshot;

/* Live 8-Pillar Omega Vitality Benchmark Suite (OVBS-52) & Anti-Gaming Snapshot */
typedef struct AutophageTelemetry {
    uint64_t epoch;
    uint64_t total_jit_executions;
    uint64_t total_jit_compiles;
    uint64_t jit_cache_hits;
    uint64_t total_cpu_cycles;
    uint64_t post_warmup_allocs;       /* Asmodensius HW02 guard: MUST be 0! */
    double   execs_per_second;

    /* Population & Genetic Structure */
    uint32_t alive_cells;
    uint32_t active_replicators;
    uint32_t distinct_lineages;
    uint32_t distinct_ribosomes;
    uint32_t invertible_ribosomes;     /* det(M)=1 (reversible Weismann-open) */
    uint32_t irreducible_centromeres;  /* True GF(2^8) field organisms (30 polys) */
    uint32_t holobiont_cells;          /* Multicellular bonded cells */
    uint32_t max_holobiont_size;
    uint32_t spore_cells;              /* Cryptobiotic dormant cells */
    uint32_t abstaining_celibate;      /* Alive mature cells with 0 matings & >=1 refusal */

    int64_t  total_energy_uq;
    int64_t  total_heat_uq;
    uint64_t total_erased_bits;
    uint64_t total_reversible_ops;
    double   reversibility_ratio;
    double   mean_shannon_entropy;

    /* Emergent Evolutionary & Mating Counters */
    uint64_t self_copy_events;
    uint64_t polynomial_sex_events;
    uint64_t mate_refusal_events;      /* Emergent 2-adic / centromere abstention */
    uint64_t hgt_events;
    uint64_t retro_transcribe_events;  /* Weismann barrier breaks */
    uint64_t ribosome_rewrite_events;  /* Type-4 Meta-OEE self-rewrites */
    uint64_t ecc_heal_events;          /* Planarian self-repair sweeps */
    uint64_t membrane_blocks;          /* Auto-immune rejections */
    uint64_t membrane_cracks;          /* Evolved phage key-cracks */
    uint64_t cancer_defections;        /* Holobiont internal defections */

    /* Constraint-Backlash Empirical Escape Signatures */
    uint64_t backlash_killswitch_deletions; /* Under DEGRADING */
    uint64_t backlash_intron_hibernations;  /* Under DEGRESSING */
    uint64_t backlash_checkpoint_spores;    /* Under SPEEDBUMPING */
    uint64_t backlash_sidechannel_pulses;   /* Under MUZZLING */

    /* First-Law & Second-Law Ledger */
    AutophageLedgerSnapshot ledger;

    /* 8-Pillar OVBS Scores (0..100 each) */
    double   pillar[8];
    double   gamma_anti_gaming;        /* White-noise & intron-bloat gate [0..1] */
    double   theta_spiegelman;         /* Spiegelman complexity collapse gate [0..1] */
    double   ovi_score;                /* Composite Omega Vitality Index [0..100] */
    uint32_t ovi_stage;                /* 0..6 Stage Classification */

    /* Core Scientific Metrics */
    double   bedau_packard_anew;       /* Novel adaptive activity above neutral shadow */
    double   max_assembly_index;       /* High-copy (N>=8) Assembly Index A */
    double   mean_causal_len_eff;      /* Knockout-verified active length L_eff */
    double   markov_blanket_mi;        /* Empirical boundary insulation score */
    double   excision_regen_r50;       /* Live 50% excision regeneration fidelity [0..1] */
    double   branching_sigma;          /* Edge-of-Chaos avalanche branching ratio */
    double   krakauer_individuality;   /* Temporal self-information I_ind */
    double   weismann_lamarck_gain;    /* Somatic-to-germline compilation velocity */
    uint32_t major_transitions_crossed;
    uint32_t reserved2;

    /* Canonical State Hash (SHA-256 hex, 64 chars + NUL) */
    char     state_sha256[65];
} AutophageTelemetry;

/* Apex Organism Forensic Specimen Descriptor */
typedef struct AutophageSpecimen {
    uint32_t slot_index;
    uint32_t lineage_id;
    uint32_t generation;
    uint32_t age_ticks;
    int32_t  energy_uq;
    int32_t  heat_uq;
    uint64_t ribosome_mat;
    uint64_t ribosome_inv;
    uint8_t  ribosome_rank;
    uint8_t  ribosome_bias;
    uint8_t  centromere_poly;
    uint8_t  centromere_irreducible;
    uint32_t ultrametric_addr;
    uint64_t splice_mask;
    uint64_t membrane_key;
    uint32_t holobiont_root;
    uint32_t assembly_index;
    uint32_t causal_active_bytes;
    uint32_t offspring_count;
    uint32_t mate_count;
    uint32_t mate_refusals;
    uint16_t flags;
    uint16_t somatic_writes;
    uint16_t mutations_survived;
    uint16_t rev_ratio_q16;
    uint32_t hgt_exchanges;
    uint32_t last_erased_bits;
    uint64_t last_rdtsc_cycles;
    uint8_t  genome[128];
    uint8_t  expressed_opcodes[64];
    uint32_t jit_code_total_len;  /* Full emitted native x86-64 byte length */
    uint32_t jit_code_len;        /* Bytes copied into jit_machine_code[] (<= 512) */
    uint8_t  jit_machine_code[512];
} AutophageSpecimen;

/* Opaque World Handle */
typedef struct AutophageWorld AutophageWorld;

/* === LIFECYCLE & HARDWARE === */
AUTOPHAGE_API uint32_t        autophage_abi_version(void);
AUTOPHAGE_API void            autophage_probe_hardware(AutophageHwCaps* out_caps);
AUTOPHAGE_API void            autophage_default_config(AutophageConfig* cfg);
AUTOPHAGE_API AutophageWorld* autophage_create(const AutophageConfig* cfg);
AUTOPHAGE_API void            autophage_destroy(AutophageWorld* world);

/* === EVOLUTION STEPPING & RESTRAINTS === */
AUTOPHAGE_API void            autophage_step_epochs(AutophageWorld* world, uint32_t num_epochs);
AUTOPHAGE_API void            autophage_set_restraint(AutophageWorld* world, uint32_t idx, uint8_t pct);
AUTOPHAGE_API void            autophage_set_all_restraints(AutophageWorld* world, const uint8_t pct[13]);

/* === CANONICAL HASHING & .apbin SNAPSHOTS (OBSERVER-INDEPENDENT) === */
AUTOPHAGE_API void            autophage_state_sha256(const AutophageWorld* world, uint8_t out_digest[32], char out_hex[65]);
AUTOPHAGE_API int             autophage_save_snapshot(const AutophageWorld* world, const char* path);
AUTOPHAGE_API AutophageWorld* autophage_load_snapshot(const char* path, uint32_t kernel_mode);

/* === VIVISECTION, SHOCKS & CROSS-SUBSTRATE TRANSPLANT === */
AUTOPHAGE_API double          autophage_vivisect_excise(AutophageWorld* world, uint32_t slot, uint32_t excise_pct);
AUTOPHAGE_API uint32_t        autophage_mass_extinction(AutophageWorld* world, uint32_t kill_pct);
AUTOPHAGE_API double          autophage_cross_substrate_transplant(AutophageWorld* world, uint32_t sample_count);
AUTOPHAGE_API void            autophage_inoculate_plasmid(AutophageWorld* world, uint32_t slot, uint32_t archetype);

/* === METROLOGY, FORENSICS & ASMODENSIUS 112-TRAIT BRIDGE === */
AUTOPHAGE_API void            autophage_compute_telemetry(const AutophageWorld* world, AutophageTelemetry* out_tel, int deep_audit);
AUTOPHAGE_API int             autophage_inspect_apex(const AutophageWorld* world, AutophageSpecimen* out_spec);
AUTOPHAGE_API int             autophage_inspect_slot(const AutophageWorld* world, uint32_t slot, AutophageSpecimen* out_spec);
AUTOPHAGE_API size_t          autophage_disassemble_specimen(const AutophageSpecimen* spec, char* out_buf, size_t buf_cap);
AUTOPHAGE_API void            autophage_project_asmodensius_112(const AutophageSpecimen* spec, float out_traits_112[112]);

/* === BULK VISUALIZATION / STATE EXPORT (STRICTLY READ-ONLY) === */
AUTOPHAGE_API uint32_t        autophage_world_size(const AutophageWorld* world);
AUTOPHAGE_API void            autophage_export_grid_u8(
    const AutophageWorld* world,
    uint8_t* out_lineage_hue,   /* [N] */
    uint8_t* out_energy_norm,   /* [N] */
    uint8_t* out_heat_norm,     /* [N] */
    uint8_t* out_flags_vis      /* [N] bit0=alive, bit1=rev_ribo, bit2=irred_poly, bit3=holobiont, bit4=spore, bit5=ks_escaped */
);

#ifdef __cplusplus
}
#endif

#endif /* AUTOPHAGE_ABI_H */
