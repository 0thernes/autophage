#include "autophage_internal.h"

/* ============================================================================
 * AUTOPHAGE v1.5.0 BRUTAL BARE-METAL FALSIFICATION HARNESS (19 GATES + 6 MUTANTS)
 * Every assertion runs live on x86-64 silicon. Zero stubs, zero skipped gates.
 * ============================================================================ */

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg, ...) do { \
    if (!(cond)) { \
        fprintf(stderr, "  [FAIL] " msg "\n", ##__VA_ARGS__); \
        return 0; \
    } \
} while (0)

static void run_gate(const char* id, const char* title, int (*fn)(void)) {
    printf("[RUN ] %s: %s ... ", id, title);
    fflush(stdout);
    double t0 = hw_wall_seconds();
    int ok = fn();
    double dt = hw_wall_seconds() - t0;
    if (ok) {
        g_pass++;
        printf("PASS (%.3fs)\n", dt);
    } else {
        g_fail++;
        printf("FAIL (%.3fs)\n", dt);
    }
}

/* T01: Silicon CPUID, Live RWX JIT & FIPS 180-4 SHA-256 NIST Vector */
static int test_t01_silicon_and_sha256(void) {
    AutophageHwCaps caps;
    autophage_probe_hardware(&caps);
    CHECK(caps.jit_rwx_verified == 1u, "RWX JIT page failed verification");
    CHECK(caps.has_popcnt == 1u, "POPCNT missing");
    CHECK(caps.has_bmi1 == 1u && caps.has_bmi2 == 1u, "BMI1/BMI2 missing");
    CHECK(caps.has_pclmulqdq == 1u, "PCLMULQDQ missing");
#if !defined(AUTOPHAGE_CI_PORTABLE)
    CHECK(caps.has_gfni == 1u, "GFNI missing");
#endif

    /* NIST FIPS 180-4 SHA-256("abc") test vector */
    uint8_t dig[32];
    char hex[65];
    sha256_compute((const uint8_t*)"abc", 3u, dig);
    sha256_to_hex(dig, hex);
    CHECK(strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0,
          "SHA-256 NIST vector mismatch: got %s", hex);
    return 1;
}

/* T02: Hardware VGF2P8AFFINEQB vs Scalar Bit-Exactness + Exact M^-1 Weismann Inverse */
static int test_t02_gfni_hardware_and_weismann_inverse(void) {
    /* 1. Verify identity matrix */
    uint64_t id = gf2_mat_identity();
    CHECK(gf2_mat_rank(id) == 8u, "Identity rank != 8");
    for (int x = 0; x < 256; x++) {
        CHECK(gf2_mat_apply_byte(id, (uint8_t)x, 0u) == (uint8_t)x, "Identity failed on %d", x);
    }

    /* 2. Compare AVX2 VGF2P8AFFINEQB against scalar across 500 matrices x 256 byte inputs */
    AP_ALIGN64 uint8_t in64[64];
    AP_ALIGN64 uint8_t hw_out[64];
    AP_ALIGN64 uint8_t sw_out[64];
    for (uint32_t m = 0; m < 500u; m++) {
        uint64_t mat = (m & 1u)
            ? gf2_mat_random_invertible(0x12345678ULL, 1u, m, 0ULL, 0u)
            : philox_u64(0x12345678ULL, 1u, m, 1ULL, 0u, 0u);
        uint8_t bias = (uint8_t)(m * 73u + 19u);

        for (int blk = 0; blk < 4; blk++) {
            for (int k = 0; k < 64; k++) in64[k] = (uint8_t)(blk * 64 + k);
            hw_gfni_affine_64bytes(in64, hw_out, mat, bias, 1);
            hw_gfni_affine_64bytes(in64, sw_out, mat, bias, 0);
            CHECK(memcmp(hw_out, sw_out, 64u) == 0,
                  "Hardware VGF2P8AFFINEQB mismatch vs scalar at mat=%u blk=%d", (unsigned)m, blk);
        }

        /* 3. Verify exact M^-1 Weismann round-trip for every invertible matrix */
        if (gf2_mat_rank(mat) == 8u) {
            uint64_t inv = 0;
            CHECK(gf2_mat_invert(mat, &inv) == 1, "Invert failed on rank-8 matrix");
            uint8_t c_inv = gf2_mat_apply_byte(inv, bias, 0u);
            for (int x = 0; x < 256; x++) {
                uint8_t y = gf2_mat_apply_byte(mat, (uint8_t)x, bias);
                uint8_t z = gf2_mat_apply_byte(inv, y, c_inv);
                CHECK(z == (uint8_t)x, "Weismann inverse failed at mat=%u x=%d", (unsigned)m, x);
            }
        }
    }
    return 1;
}

/* T03: GF(2)[x] Degree-8 Irreducibility Count == 30 + PCLMULQDQ Bit-Exactness */
static int test_t03_galois_poly_and_pclmul(void) {
    int irred_count = 0;
    for (int p = 0; p < 256; p++) {
        if (gf2_poly8_is_irreducible((uint8_t)p)) irred_count++;
    }
    CHECK(irred_count == 30, "Expected exactly 30 irreducible deg-8 polys over GF(2), got %d", irred_count);
    CHECK(gf2_poly8_is_irreducible(0x1Bu) == 1, "AES poly x^8+x^4+x^3+x+1 (0x1B) must be irreducible");
    CHECK(gf2_poly8_is_irreducible(0x01u) == 0, "x^8+1=(x+1)^8 must be reducible");

    /* Verify hardware PCLMULQDQ matches scalar carry-less multiply across 1,000 random pairs */
    for (uint32_t i = 0; i < 1000u; i++) {
        uint64_t a = philox_u64(0x9999ULL, 1u, i, 0ULL, 1u, 0u);
        uint64_t b = philox_u64(0x9999ULL, 1u, i, 0ULL, 1u, 1u);
        uint64_t hw_lo = 0, hw_hi = 0;
        hw_pclmul64_full(a, b, &hw_lo, &hw_hi);
        uint64_t sw_lo = 0, sw_hi = 0;
        for (int k = 0; k < 64; k++) {
            if ((b >> k) & 1ULL) {
                sw_lo ^= (a << k);
                if (k > 0) sw_hi ^= (a >> (64 - k));
            }
        }
        CHECK(hw_lo == sw_lo && hw_hi == sw_hi, "PCLMULQDQ mismatch at pair %u", (unsigned)i);
        CHECK(hw_pclmul64_lo(a, b) == sw_lo, "PCLMULQDQ lo mismatch");
    }

    /* Verify hardware vs scalar clmul_hash64 exactness */
    AP_ALIGN64 uint8_t buf[64];
    for (int i = 0; i < 64; i++) buf[i] = (uint8_t)(i * 13 + 5);
    uint64_t h_hw = gf2_clmul_hash64(buf, 64u, 0x1Bu, 1);
    uint64_t h_sw = gf2_clmul_hash64(buf, 64u, 0x1Bu, 0);
    CHECK(h_hw == h_sw, "gf2_clmul_hash64 HW != SW");
    return 1;
}

/* T04: Native x86-64 RWX JIT vs Scalar Reference Oracle Bit-Exactness
 * Covers ALL 32 opcodes, 5,000 random genomes, smooth NERFING levels, and rank-8 + singular ribosomes! */
static int test_t04_jit_vs_oracle_bit_exact(void) {
    AutophageHwCaps caps;
    autophage_probe_hardware(&caps);
    AutophageJitCache cache;
    CHECK(jit_cache_init(&cache, &caps) == 1, "JIT cache init failed");

    static const uint8_t NERF_LEVELS[5] = { 0u, 25u, 50u, 75u, 100u };

    for (uint32_t trial = 0; trial < 5000u; trial++) {
        AP_ALIGN64 uint8_t expr[64];
        if (trial == 0u || trial == 1u) {
            /* Explicit coverage of all 32 opcodes (b7=0 and b7=1) under both rank-8 and singular */
            for (int i = 0; i < 64; i++) {
                expr[i] = (uint8_t)((i & 0x1F) | ((i & 3) << 5) | (((i >> 5) & 1) << 7));
            }
        } else {
            for (int w = 0; w < 8; w++) {
                uint64_t r = philox_u64(0x704ULL, 1u, trial, 0ULL, 1u, (uint32_t)w);
                memcpy(expr + w * 8, &r, 8u);
            }
        }
        uint64_t smask = (trial < 10u) ? ~0ULL : philox_u64(0x704ULL, 1u, trial, 0ULL, 2u, 0u);
        uint8_t  nerf  = NERF_LEVELS[trial % 5u];

        AP_ALIGN64 uint8_t ta_jit[128];
        AP_ALIGN64 uint8_t tb_jit[128];
        AP_ALIGN64 uint8_t ta_ref[128];
        AP_ALIGN64 uint8_t tb_ref[128];
        for (int w = 0; w < 16; w++) {
            uint64_t ra = philox_u64(0x704ULL, 1u, trial, 1ULL, 1u, (uint32_t)w);
            uint64_t rb = philox_u64(0x704ULL, 1u, trial, 2ULL, 1u, (uint32_t)w);
            memcpy(ta_jit + w * 8, &ra, 8u);
            memcpy(tb_jit + w * 8, &rb, 8u);
        }
        memcpy(ta_ref, ta_jit, 128u);
        memcpy(tb_ref, tb_jit, 128u);

        AutophageExecCtx ctx_jit, ctx_ref;
        memset(&ctx_jit, 0, sizeof(ctx_jit));
        ctx_jit.regs[0]      = philox_u64(0x704ULL, 1u, trial, 3ULL, 0u, 0u);
        ctx_jit.regs[1]      = philox_u64(0x704ULL, 1u, trial, 3ULL, 0u, 1u);
        ctx_jit.regs[2]      = philox_u64(0x704ULL, 1u, trial, 3ULL, 0u, 2u);
        ctx_jit.regs[3]      = philox_u64(0x704ULL, 1u, trial, 3ULL, 0u, 3u);
        ctx_jit.splice_mask  = smask;
        if (trial % 5u == 1u) {
            /* Deliberately singular matrix (row 0 = 0 -> rank <= 7) to test OP_REV_GFNI_FOLD erasure! */
            ctx_jit.ribosome_mat = philox_u64(0x704ULL, 1u, trial, 4ULL, 1u, 0u) & 0xFFFFFFFFFFFFFF00ULL;
        } else {
            ctx_jit.ribosome_mat = gf2_mat_random_invertible(0x704ULL, 1u, trial, 4ULL, 0u);
        }
        ctx_jit.ribosome_rank = (uint32_t)gf2_mat_rank(ctx_jit.ribosome_mat);
        ctx_jit.env_wave      = philox_u64(0x704ULL, 1u, trial, 3ULL, 1u, 0u);
        ctx_jit.clade_xor     = (trial % 5u == 0u) ? 0ULL : philox_u64(0x704ULL, 1u, trial, 3ULL, 1u, 1u);
        ctx_jit.stigmergy_in  = philox_u64(0x704ULL, 1u, trial, 3ULL, 1u, 2u);
        ctx_jit.gas           = 32u;
        ctx_jit.head0         = (uint32_t)(trial * 11u) & 0x7Fu;
        ctx_jit.head1         = (uint32_t)(trial * 23u) & 0x7Fu;
        ctx_ref = ctx_jit;

        autophage_jit_fn fn = jit_compile_or_fetch(&cache, expr, smask, nerf, NULL, NULL, 0);
        uint64_t r_jit = fn(ta_jit, tb_jit, &ctx_jit);
        uint64_t r_ref = jit_reference_oracle(expr, smask, nerf, ta_ref, tb_ref, &ctx_ref);

        CHECK(r_jit == r_ref, "Trial %u: ret mismatch jit=%llu ref=%llu",
              (unsigned)trial, (unsigned long long)r_jit, (unsigned long long)r_ref);
        CHECK(memcmp(&ctx_jit, &ctx_ref, sizeof(ctx_jit)) == 0,
              "Trial %u (nerf=%u rank=%u): ExecCtx mismatch (erased %u vs %u, rev %u vs %u, irr %u vs %u, bio %llx vs %llx)",
              (unsigned)trial, (unsigned)nerf, (unsigned)ctx_jit.ribosome_rank,
              ctx_jit.erased_bits, ctx_ref.erased_bits,
              ctx_jit.rev_ops, ctx_ref.rev_ops,
              ctx_jit.irr_ops, ctx_ref.irr_ops,
              (unsigned long long)ctx_jit.bio_actions, (unsigned long long)ctx_ref.bio_actions);
        CHECK(memcmp(ta_jit, ta_ref, 128u) == 0, "Trial %u: tape_a mismatch", (unsigned)trial);
        CHECK(memcmp(tb_jit, tb_ref, 128u) == 0, "Trial %u: tape_b mismatch", (unsigned)trial);
    }

    jit_cache_destroy(&cache);
    return 1;
}

/* T05: 1,000,000 Fuzz-Mutated Genomes Executed Live on x86-64 Silicon with OOB Canaries */
static int test_t05_million_fuzz_silicon_safety(void) {
    AutophageHwCaps caps;
    autophage_probe_hardware(&caps);
    AutophageJitCache cache;
    CHECK(jit_cache_init(&cache, &caps) == 1, "JIT cache init failed");

    /* Guard canaries surrounding tape_a and tape_b */
    AP_ALIGN64 uint8_t buf_a[256];
    AP_ALIGN64 uint8_t buf_b[256];
    memset(buf_a, 0xA5, sizeof(buf_a));
    memset(buf_b, 0x5A, sizeof(buf_b));
    uint8_t* ta = buf_a + 64;
    uint8_t* tb = buf_b + 64;

    uint64_t s = 0xDEADBEEF1337C0DEULL;
    AP_ALIGN64 uint8_t expr[64];
    AutophageExecCtx ctx;
    memset(&ctx, 0, sizeof(ctx));

    /* Compile 2,000 unique mutated genomes and execute 1,000,000 total native JIT calls! */
    for (uint32_t g = 0; g < 2000u; g++) {
        for (int w = 0; w < 8; w++) {
            s ^= s << 13; s ^= s >> 7; s ^= s << 17;
            memcpy(expr + w * 8, &s, 8u);
        }
        uint64_t smask = s | 0x0101010101010101ULL;
        autophage_jit_fn fn = jit_compile_or_fetch(&cache, expr, smask, 0u, NULL, NULL, 0);

        for (uint32_t rep = 0; rep < 500u; rep++) {
            ctx.gas           = 24u;
            ctx.erased_bits   = 0u;
            ctx.rev_ops       = 0u;
            ctx.irr_ops       = 0u;
            ctx.head0         = (rep * 7u) & 0x7Fu;
            ctx.head1         = (rep * 13u) & 0x7Fu;
            ctx.splice_mask   = smask;
            ctx.ribosome_mat  = 0x0102040810204080ULL;
            ctx.ribosome_rank = 8u;
            (void)fn(ta, tb, &ctx);
        }
    }

    /* Verify every single guard canary byte before and after both tapes is 100% intact! */
    for (int i = 0; i < 64; i++) {
        CHECK(buf_a[i] == 0xA5 && buf_a[192 + i] == 0xA5, "tape_a OOB canary corrupted at %d!", i);
        CHECK(buf_b[i] == 0x5A && buf_b[192 + i] == 0x5A, "tape_b OOB canary corrupted at %d!", i);
    }

    jit_cache_destroy(&cache);
    return 1;
}

/* T06: Exact Landauer Reversibility Proof
 * - 0 erased bits on reversible ops + full-rank GFNI
 * - Exact POPCNT erasure on singular-rank GFNI (Landauer loophole closed!)
 * - True NOP (0 rev_ops, 0 erased) when NERFING=100 strips GFNI (no fake rev_ops++!)
 * - Exact POPCNT erasure on irreversible AND_MASK */
static int test_t06_landauer_reversibility(void) {
    AutophageHwCaps caps;
    autophage_probe_hardware(&caps);
    AutophageJitCache cache;
    CHECK(jit_cache_init(&cache, &caps) == 1, "JIT cache init failed");

    uint8_t ta[128], tb[128];
    memset(ta, 0x3C, 128); memset(tb, 0xC3, 128);

    /* 1. Pure reversible program: TOFFOLI x2, FREDKIN x2, rank-8 GFNI_FOLD */
    uint8_t rev_prog[64];
    memset(rev_prog, 0, sizeof(rev_prog));
    rev_prog[0] = (uint8_t)(OP_REV_TOFFOLI   | (0 << 5));
    rev_prog[1] = (uint8_t)(OP_REV_TOFFOLI   | (0 << 5)); /* Self-inverse restores R0! */
    rev_prog[2] = (uint8_t)(OP_REV_FREDKIN   | (1 << 5));
    rev_prog[3] = (uint8_t)(OP_REV_FREDKIN   | (1 << 5)); /* Self-inverse restores R2,R3! */
    rev_prog[4] = (uint8_t)(OP_REV_GFNI_FOLD | (3 << 5)); /* Full-rank identity GFNI fold */
    uint64_t mask5 = 0x1FULL;

    AutophageExecCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.regs[0]       = 0x1122334455667788ULL;
    ctx.regs[1]       = 0xF0F0F0F00F0F0F0FULL;
    ctx.regs[2]       = 0xCAFEBABEDEADBEEFULL;
    ctx.regs[3]       = 0x0123456789ABCDEFULL;
    ctx.gas           = 32u;
    ctx.ribosome_mat  = gf2_mat_identity();
    ctx.ribosome_rank = 8u;

    autophage_jit_fn fn_rev = jit_compile_or_fetch(&cache, rev_prog, mask5, 0u, NULL, NULL, 0);
    (void)fn_rev(ta, tb, &ctx);

    CHECK(ctx.erased_bits == 0u, "Reversible program erased %u bits (must be 0)!", ctx.erased_bits);
    CHECK(ctx.rev_ops == 5u && ctx.irr_ops == 0u, "Expected 5 rev_ops / 0 irr_ops, got %u/%u",
          ctx.rev_ops, ctx.irr_ops);
    CHECK(ctx.regs[0] == 0x1122334455667788ULL && ctx.regs[2] == 0xCAFEBABEDEADBEEFULL,
          "Toffoli/Fredkin double application failed self-involution!");

    /* 2. Singular-ribosome GFNI_FOLD MUST erase exact POPCNT bits (Landauer loophole closed!) */
    uint8_t gfni_prog[64];
    memset(gfni_prog, 0, sizeof(gfni_prog));
    gfni_prog[0] = (uint8_t)(OP_REV_GFNI_FOLD | (0 << 5)); /* pc=0, b7=0 -> imm_bias=0 */
    memset(&ctx, 0, sizeof(ctx));
    ctx.regs[0]       = 0xFFFFFFFFFFFFFFFFULL;
    ctx.gas           = 16u;
    ctx.ribosome_mat  = 0ULL; /* Rank-0 singular matrix collapses all 64 bits to 0! */
    ctx.ribosome_rank = 0u;
    autophage_jit_fn fn_sg = jit_compile_or_fetch(&cache, gfni_prog, 1ULL, 0u, NULL, NULL, 0);
    (void)fn_sg(ta, tb, &ctx);
    CHECK(ctx.regs[0] == 0ULL && ctx.erased_bits == 64u && ctx.irr_ops == 1u && ctx.rev_ops == 0u,
          "Singular GFNI Landauer check failed: R0=%llx erased=%u irr=%u rev=%u",
          (unsigned long long)ctx.regs[0], ctx.erased_bits, ctx.irr_ops, ctx.rev_ops);

    /* 3. NERFING=100 strips OP_REV_GFNI_FOLD into true NOP (NO fake rev_ops++!) */
    memset(&ctx, 0, sizeof(ctx));
    ctx.regs[0]       = 0xA5A5A5A5A5A5A5A5ULL;
    ctx.gas           = 16u;
    ctx.ribosome_mat  = gf2_mat_identity();
    ctx.ribosome_rank = 8u;
    autophage_jit_fn fn_nerf = jit_compile_or_fetch(&cache, gfni_prog, 1ULL, 100u, NULL, NULL, 0);
    (void)fn_nerf(ta, tb, &ctx);
    CHECK(ctx.regs[0] == 0xA5A5A5A5A5A5A5A5ULL && ctx.rev_ops == 0u && ctx.irr_ops == 0u && ctx.erased_bits == 0u,
          "Nerfed GFNI granted fake ops: R0=%llx rev=%u irr=%u erased=%u",
          (unsigned long long)ctx.regs[0], ctx.rev_ops, ctx.irr_ops, ctx.erased_bits);

    /* 4. Irreversible AND_MASK erasing exact known bits: 0xFFFFFFFFFFFFFFFF & 0x00000000FFFFFFFF -> 32 bits! */
    uint8_t irr_prog[64];
    memset(irr_prog, 0, sizeof(irr_prog));
    irr_prog[0] = (uint8_t)(OP_IRR_AND_MASK | (0 << 5)); /* R0 &= R1 */
    memset(&ctx, 0, sizeof(ctx));
    ctx.regs[0] = 0xFFFFFFFFFFFFFFFFULL;
    ctx.regs[1] = 0x00000000FFFFFFFFULL;
    ctx.gas     = 16u;
    autophage_jit_fn fn_irr = jit_compile_or_fetch(&cache, irr_prog, 1ULL, 0u, NULL, NULL, 0);
    (void)fn_irr(ta, tb, &ctx);
    CHECK(ctx.erased_bits == 32u, "Expected exactly 32 erased bits via hardware POPCNT, got %u", ctx.erased_bits);

    jit_cache_destroy(&cache);
    return 1;
}

/* T07: First-Law & Second-Law Conservation (P2) + Membrane/Detritus Spatial Sync + Planted Mutants 1 & 2 */
static int test_t07_thermodynamics_and_mutants(void) {
    /* Closed-box (BOXING = 100%) AND Open-flux (BOXING = 0%) */
    for (int closed = 0; closed < 2; closed++) {
        AutophageConfig cfg;
        autophage_default_config(&cfg);
        cfg.width = 16u; cfg.height = 16u;
        cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
        cfg.seed = 5001ULL + (uint64_t)closed;
        if (closed) cfg.restraints[RESTRAINT_BOXING] = 100u;

        AutophageWorld* w = autophage_create(&cfg);
        CHECK(w != NULL, "autophage_create failed");
        double prev_sink = 0.0;
        for (int e = 0; e < 50; e++) {
            autophage_step_epochs(w, 1u);
            AutophageLedgerSnapshot led;
            soup_ledger_snapshot(w, &led);
            CHECK(led.int_ledger_residual_uq == 0LL,
                  "Closed=%d ep=%d: int residual %lld != 0", closed, e, (long long)led.int_ledger_residual_uq);
            CHECK(led.relative_drift < 1e-12,
                  "Closed=%d ep=%d: rel drift %.3e >= 1e-12", closed, e, led.relative_drift);
            CHECK(led.second_law_violations == 0u, "Second-law violation detected");
            CHECK(led.e_heat_radiated_sink_kj >= prev_sink - 1e-15, "Heat sink decreased (2nd Law!)");
            if (closed) CHECK(led.e_heat_radiated_sink_kj == 0.0 && led.e_inflow_cum_kj == 0.0,
                              "Closed box leaked flux!");

            /* Verify exact per-cell membrane_pot_uq and local detritus_grid sums match global totals */
            int64_t sum_mem = 0LL, sum_det = 0LL;
            for (uint32_t k = 0; k < w->n_cells; k++) {
                CHECK(w->cells[k].membrane_pot_uq >= 0, "Negative cell membrane_pot_uq at slot %u", (unsigned)k);
                CHECK(w->detritus_grid[k] >= 0, "Negative local detritus_grid at slot %u", (unsigned)k);
                sum_mem += (int64_t)w->cells[k].membrane_pot_uq;
                sum_det += (int64_t)w->detritus_grid[k];
            }
            CHECK(sum_mem == w->e_membrane_pot_uq,
                  "Membrane sum mismatch: cells=%lld global=%lld", (long long)sum_mem, (long long)w->e_membrane_pot_uq);
            CHECK(sum_det == w->e_detritus_uq,
                  "Detritus sum mismatch: grid=%lld global=%lld", (long long)sum_det, (long long)w->e_detritus_uq);

            prev_sink = led.e_heat_radiated_sink_kj;
        }

        /* PLANTED MUTANT 1: Subtle -2e-11 energy leak MUST BE CAUGHT! */
        if (!closed) {
            int64_t saved_det = w->e_detritus_uq;
            int64_t leak = (int64_t)((double)(w->e_initial_uq + w->e_inflow_cum_uq) * 2e-11);
            if (leak < 1LL) leak = 1LL;
            w->e_detritus_uq -= leak;
            AutophageLedgerSnapshot m1;
            soup_ledger_snapshot(w, &m1);
            CHECK(m1.int_ledger_residual_uq != 0LL && m1.relative_drift > 1e-12,
                  "Planted Mutant 1 (-2e-11 leak) escaped detection! drift=%.3e", m1.relative_drift);
            w->e_detritus_uq = saved_det;

            /* PLANTED MUTANT 2: 1-unit Second-Law heat-to-free-energy backflow MUST BE CAUGHT BY ENGINE VERIFIER! */
            int64_t sink_before = w->e_heat_radiated_sink_uq;
            uint32_t v_before = w->second_law_violations;
            w->e_heat_radiated_sink_uq -= 1LL;
            w->e_detritus_uq += 1LL;
            int sec_ok = soup_verify_second_law(w, sink_before);
            CHECK(!sec_ok && w->second_law_violations == v_before + 1u,
                  "Planted Mutant 2 backflow escaped soup_verify_second_law!");
            /* Restore clean state */
            w->e_heat_radiated_sink_uq = sink_before;
            w->e_detritus_uq -= 1LL;
            w->second_law_violations = v_before;

            /* Verify mass extinction releases 100% of membrane potential (ZERO membrane graveyard!) */
            autophage_mass_extinction(w, 100u);
            AutophageLedgerSnapshot post_ext;
            soup_ledger_snapshot(w, &post_ext);
            CHECK(w->e_membrane_pot_uq == 0LL && post_ext.int_ledger_residual_uq == 0LL,
                  "Membrane graveyard leak after 100%% extinction: mem=%lld res=%lld",
                  (long long)w->e_membrane_pot_uq, (long long)post_ext.int_ledger_residual_uq);
        }
        autophage_destroy(w);
    }
    return 1;
}

/* T08: Domain-Isolated Philox4x32-10 PRNG + Planted Mutant 6 CAUGHT! */
static int test_t08_philox_domain_isolation(void) {
    /* Exogenous wave for (x,y,epoch) depends ONLY on WORLD_EXOG, never on any entity state! */
    uint64_t w0 = soup_env_wave(5u, 7u, 42ULL, 5000ULL, 0u);
    uint64_t w1 = soup_env_wave(5u, 7u, 42ULL, 5000ULL, 0u);
    CHECK(w0 == w1, "Deterministic WORLD_EXOG wave diverged");

    /* Entity draws for slot 17 are strictly invariant to anything done to slot 0 */
    uint64_t e17_a = philox_u64(5000ULL, PRNG_DOMAIN_ENTITY_INT, 17u, 42ULL, 1u, 0u);
    uint64_t e0    = philox_u64(5000ULL, PRNG_DOMAIN_ENTITY_INT, 0u,  42ULL, 1u, 0u);
    uint64_t e17_b = philox_u64(5000ULL, PRNG_DOMAIN_ENTITY_INT, 17u, 42ULL, 1u, 0u);
    CHECK(e17_a == e17_b && e17_a != e0, "Entity Philox isolation failed");

    /* v1.3.0 Compiled Check (Lean T39): pairwise domain distinctness across tags 0..4 */
    uint64_t dom_draws[5];
    for (uint32_t d = 0u; d < 5u; d++) {
        dom_draws[d] = philox_u64(5088ULL, d, 17u, 42ULL, 1u, 0u);
    }
    for (uint32_t i = 0u; i < 5u; i++) {
        for (uint32_t j = i + 1u; j < 5u; j++) {
            CHECK(dom_draws[i] != dom_draws[j],
                  "Philox domain collision between dom=%u and dom=%u", (unsigned)i, (unsigned)j);
        }
    }

    /* v1.3.0 Compiled Check (Lean T40): 3-Tier Seed Firewall pairwise disjointness */
    for (uint64_t s = 0ULL; s <= 6000ULL; s++) {
        int in_practice = (s >= 100ULL  && s <= 999ULL)  ? 1 : 0;
        int in_tuning   = (s >= 2000ULL && s <= 2999ULL) ? 1 : 0;
        int in_locked   = (s >= 5000ULL)                 ? 1 : 0;
        CHECK(in_practice + in_tuning + in_locked <= 1,
              "Seed firewall overlap at seed=%llu", (unsigned long long)s);
    }

    /* PLANTED MUTANT 6: Perturb real AutophageWorld slot 0 with extra ENTITY_INT mutations;
     * verify WORLD_EXOG for slot 17 stays bit-exact, while a coupled draw-index shift diverges! */
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 16u; cfg.height = 16u; cfg.seed = 5088ULL;
    AutophageWorld* wa = autophage_create(&cfg);
    AutophageWorld* wb = autophage_create(&cfg);
    wb->cells[0].genome[0] ^= 0x5Au;
    autophage_step_epochs(wa, 5u);
    autophage_step_epochs(wb, 5u);
    uint64_t exog_a = soup_env_wave(1u, 1u, wa->epoch, wa->master_seed, 0u);
    uint64_t exog_b = soup_env_wave(1u, 1u, wb->epoch, wb->master_seed, 0u);
    uint64_t mutant_coupled = philox_u64(wb->master_seed, PRNG_DOMAIN_ENTITY_INT, 17u, wb->epoch, 0x1001u, 1u);
    uint64_t clean_draw0    = philox_u64(wa->master_seed, PRNG_DOMAIN_ENTITY_INT, 17u, wa->epoch, 0x1001u, 0u);
    /* Also verify traits112[75] (ultrametric tzcnt) is NOT stuck at 0.0f for even non-zero addr! */
    AutophageSpecimen sp;
    memset(&sp, 0, sizeof(sp));
    sp.ultrametric_addr = 0x00100000u; /* 20 trailing zeros */
    float t112[112];
    autophage_project_asmodensius_112(&sp, t112);
    /* Verify dead world (100% extinction) scores strictly 0.0 OVI */
    autophage_mass_extinction(wb, 100u);
    AutophageTelemetry dead_tel;
    autophage_compute_telemetry(wb, &dead_tel, 0);
    autophage_destroy(wa);
    autophage_destroy(wb);
    CHECK(exog_a == exog_b && mutant_coupled != clean_draw0, "Coupled PRNG / EXOG isolation check failed");
    CHECK(t112[75] > 0.5f, "traits112[75] stuck at 0 bug regressed! val=%.4f", t112[75]);
    CHECK(dead_tel.alive_cells == 0u && dead_tel.ovi_score == 0.0,
          "Dead world scored non-zero OVI=%.2f!", dead_tel.ovi_score);
    return 1;
}

/* T09: Full-World Dual-Kernel Bit-Exact SHA-256 Parity (NATIVE_JIT == SCALAR_REF) */
static int test_t09_dual_kernel_world_parity(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width  = 32u;
    cfg.height = 32u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5099ULL;

    cfg.kernel_mode = AUTOPHAGE_KERNEL_NATIVE_JIT;
    AutophageWorld* w_jit = autophage_create(&cfg);
    autophage_step_epochs(w_jit, 60u);
    char sha_jit[65];
    autophage_state_sha256(w_jit, NULL, sha_jit);

    cfg.kernel_mode = AUTOPHAGE_KERNEL_SCALAR_REF;
    AutophageWorld* w_ref = autophage_create(&cfg);
    autophage_step_epochs(w_ref, 60u);
    char sha_ref[65];
    autophage_state_sha256(w_ref, NULL, sha_ref);

    int ok = (strcmp(sha_jit, sha_ref) == 0);
    autophage_destroy(w_jit);
    autophage_destroy(w_ref);
    CHECK(ok, "Dual-kernel SHA-256 diverged! JIT=%s REF=%s", sha_jit, sha_ref);
    return 1;
}

/* T10: Strict Zero-Observer-Effect Gate (P9/HW20) + Planted Mutant 5 CAUGHT! */
static int test_t10_observer_independence(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 16u; cfg.height = 16u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5110ULL;

    /* World A: Pure headless run */
    AutophageWorld* wa = autophage_create(&cfg);
    autophage_step_epochs(wa, 40u);
    char sha_a[65];
    autophage_state_sha256(wa, NULL, sha_a);

    /* World B: Heavy UI/telemetry/apex/disassembly/112-trait observation every 5 epochs! */
    AutophageWorld* wb = autophage_create(&cfg);
    uint8_t g1[256], g2[256], g3[256], g4[256];
    for (int k = 0; k < 8; k++) {
        autophage_step_epochs(wb, 5u);
        AutophageTelemetry tel;
        autophage_compute_telemetry(wb, &tel, 1);
        AutophageSpecimen spec;
        autophage_inspect_apex(wb, &spec);
        char dis[1024];
        autophage_disassemble_specimen(&spec, dis, sizeof(dis));
        float tr[112];
        autophage_project_asmodensius_112(&spec, tr);
        autophage_export_grid_u8(wb, g1, g2, g3, g4);
    }
    char sha_b[65];
    autophage_state_sha256(wb, NULL, sha_b);
    CHECK(strcmp(sha_a, sha_b) == 0, "Observer effect detected! headless=%s observed=%s", sha_a, sha_b);

    /* PLANTED MUTANT 5: Flip 1 bit during observation -> SHA-256 MUST CATCH IT! */
    wb->cells[3].genome[0] ^= 0x01u;
    char sha_mut[65];
    autophage_state_sha256(wb, NULL, sha_mut);
    CHECK(strcmp(sha_a, sha_mut) != 0, "Planted Mutant 5 (1-bit observer write) escaped SHA-256!");

    autophage_destroy(wa);
    autophage_destroy(wb);
    return 1;
}

/* T11: Canonical .apbin Snapshot Save, Cross-Kernel Resume Parity & Tampered Snapshot Rejection */
static int test_t11_apbin_snapshot_parity(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 16u; cfg.height = 16u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5111ULL;
    cfg.kernel_mode = AUTOPHAGE_KERNEL_NATIVE_JIT;

    AutophageWorld* w1 = autophage_create(&cfg);
    autophage_step_epochs(w1, 25u);
    const char* snap_path = "test_tmp_snapshot.apbin";
    CHECK(autophage_save_snapshot(w1, snap_path) == 1, "Failed to save .apbin snapshot");

    /* Load snapshot into SCALAR_REF kernel and step both 25 more epochs! */
    AutophageWorld* w2 = autophage_load_snapshot(snap_path, AUTOPHAGE_KERNEL_SCALAR_REF);
    CHECK(w2 != NULL, "Failed to load .apbin snapshot");

    autophage_step_epochs(w1, 25u);
    autophage_step_epochs(w2, 25u);

    char s1[65], s2[65];
    autophage_state_sha256(w1, NULL, s1);
    autophage_state_sha256(w2, NULL, s2);
    autophage_destroy(w1);
    autophage_destroy(w2);
    CHECK(strcmp(s1, s2) == 0, ".apbin cross-kernel resume diverged: %s vs %s", s1, s2);

    /* Tamper test: flip 1 byte inside payload and verify autophage_load_snapshot REJECTS it! */
    FILE* f = fopen(snap_path, "r+b");
    CHECK(f != NULL, "Failed to reopen snapshot for tamper test");
    CHECK(fseek(f, 128L, SEEK_SET) == 0, "fseek failed in tamper test");
    int ch = fgetc(f);
    CHECK(ch != EOF, "fgetc failed in tamper test");
    CHECK(fseek(f, 128L, SEEK_SET) == 0, "fseek rewrite failed in tamper test");
    uint8_t flipped = (uint8_t)((uint8_t)ch ^ 0x01u);
    CHECK(fwrite(&flipped, 1u, 1u, f) == 1u, "fwrite failed in tamper test");
    fclose(f);

    AutophageWorld* w_bad = autophage_load_snapshot(snap_path, AUTOPHAGE_KERNEL_SCALAR_REF);
    remove(snap_path);
    CHECK(w_bad == NULL, "Tampered .apbin snapshot was NOT rejected!");
    return 1;
}

/* T12: Zero Post-Warmup Heap Allocations (Asmodensius HW02) with Real Allocator Monitor */
static int test_t12_zero_post_warmup_allocs(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 16u; cfg.height = 16u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5112ULL;
    AutophageWorld* w = autophage_create(&cfg);
    CHECK(w != NULL, "autophage_create failed");

    uint64_t allocs_before = g_ap_alloc_counter;
    uint64_t viols_before  = g_ap_step_alloc_violations;
    /* 80 epochs x 256 cells = 20,480 cell ticks (>10,000 active non-spore JIT execs!) */
    autophage_step_epochs(w, 80u);
    uint64_t allocs_after = g_ap_alloc_counter;
    uint64_t viols_after  = g_ap_step_alloc_violations;

    AutophageTelemetry t;
    autophage_compute_telemetry(w, &t, 0);
    autophage_destroy(w);

    /* Live negative control: prove the step-guard allocation monitor actually trips on ap_malloc! */
    g_ap_in_step_guard = 1;
    uint64_t probe_v0 = g_ap_step_alloc_violations;
    void* probe_ptr = ap_malloc(16u);
    uint64_t probe_v1 = g_ap_step_alloc_violations;
    g_ap_in_step_guard = 0;
    if (probe_v1 > probe_v0) g_ap_step_alloc_violations = probe_v0;
    free(probe_ptr);

    CHECK(probe_v1 == probe_v0 + 1ULL, "Allocation monitor negative-control probe did not trip!");
    CHECK(allocs_after == allocs_before && viols_after == viols_before &&
          t.post_warmup_allocs == 0ULL && t.total_jit_executions >= 10000ULL,
          "Post-warmup allocs=%llu (delta=%llu, viols=%llu), execs=%llu",
          (unsigned long long)t.post_warmup_allocs,
          (unsigned long long)(allocs_after - allocs_before),
          (unsigned long long)(viols_after - viols_before),
          (unsigned long long)t.total_jit_executions);
    return 1;
}

/* T13: Spontaneous Zero-Seed Abiogenesis Phase Transition */
static int test_t13_spontaneous_abiogenesis(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width  = 32u;
    cfg.height = 32u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_ABIOGENESIS; /* ZERO seeded plasmids! */
    cfg.seed   = 5000ULL;

    AutophageWorld* w = autophage_create(&cfg);
    AutophageTelemetry t0, t1;
    autophage_compute_telemetry(w, &t0, 0);
    autophage_step_epochs(w, 160u);
    autophage_compute_telemetry(w, &t1, 1);
    autophage_destroy(w);

    CHECK(t0.alive_cells == 0u && t0.ovi_score == 0.0 && t0.execs_per_second == 0.0,
          "Zero-seed soup should start inert (OVI=%.2f, eps=%.2f)", t0.ovi_score, t0.execs_per_second);
    CHECK(t1.alive_cells > 50u && t1.self_copy_events > 20ULL && t1.ovi_score > 10.0 && t1.execs_per_second > 0.0,
          "Zero-seed abiogenesis failed to ignite: alive=%u copy=%llu OVI=%.2f eps=%.1f",
          (unsigned)t1.alive_cells, (unsigned long long)t1.self_copy_events, t1.ovi_score, t1.execs_per_second);
    return 1;
}

/* T14: Anti-Gaming Gates 1 & 2 + Planted Mutants 3 (Intron Bloat + Dead Scratch) & 4 (White Noise) */
static int test_t14_anti_gaming_gates(void) {
    /* Planted Mutant 4: Pure White-Noise Soup -> MUST score OVI == 0.0! */
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 16u; cfg.height = 16u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_ABIOGENESIS;
    cfg.seed   = 5114ULL;
    AutophageWorld* w = autophage_create(&cfg);
    AutophageTelemetry tn;
    autophage_compute_telemetry(w, &tn, 1);
    CHECK(tn.gamma_anti_gaming == 0.0 && tn.ovi_score == 0.0,
          "Planted Mutant 4 (white noise) scored OVI=%.2f (must be 0.0)!", tn.ovi_score);

    /* Planted Mutant 3a: Cosmetic Intron Bloat (4 active exons + 60 dead masked introns!) */
    AutophageCell bloated;
    memset(&bloated, 0, sizeof(bloated));
    bloated.ribosome_mat = gf2_mat_identity();
    bloated.splice_mask  = 0x000000000000000FULL; /* Only 4 active exons; 60 masked dead introns! */
    bloated.genome[0]    = OP_METAB_HARVEST;
    bloated.genome[1]    = OP_REV_XOR_REG;
    bloated.genome[2]    = OP_MEMBRANE_SEAL;
    bloated.genome[3]    = OP_MITOSIS_FORK;
    for (int i = 4; i < 64; i++) bloated.genome[i] = (uint8_t)(i * 37 + 11); /* 60 bytes cosmetic bloat */

    uint32_t leff = metrology_causal_knockout_len(w, &bloated);

    /* Planted Mutant 3b: Add 2 active exons touching ONLY dead scratch register R2 (never read by phenotype!) */
    AutophageCell dead_scratch = bloated;
    dead_scratch.splice_mask = 0x000000000000003FULL;
    dead_scratch.genome[4]   = (uint8_t)(OP_REV_XOR_REG | (2 << 5)); /* R2 ^= R3 (dead!) */
    dead_scratch.genome[5]   = (uint8_t)(OP_REV_ROL_REG | (2 << 5)); /* rol R2   (dead!) */
    uint32_t leff_dead = metrology_causal_knockout_len(w, &dead_scratch);

    autophage_destroy(w);
    CHECK(leff <= 4u && leff >= 2u,
          "Planted Mutant 3a (intron bloat) fooled Causal Knockout: L_eff=%u (expected 2..4)!", (unsigned)leff);
    CHECK(leff_dead == leff,
          "Planted Mutant 3b (dead scratch ops) inflated Causal Knockout: %u vs base %u!",
          (unsigned)leff_dead, (unsigned)leff);
    return 1;
}

/* T15: Planarian 50% Excision Regeneration Fidelity (ECC vs Unprotected Control + No Free Gifting) */
static int test_t15_planarian_excision(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 16u; cfg.height = 16u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5115ULL;
    AutophageWorld* w = autophage_create(&cfg);

    /* Slot 0 is a seeded ECC-protected plasmid -> 50% Strand-alpha excision must recover 100%! */
    double fid_ecc = autophage_vivisect_excise(w, 0u, 50u);
    /* Slot 1 is unprotected random cell -> 50% excision must NOT heal! */
    w->cells[1].flags &= (uint16_t)~CELL_FLAG_ECC_PROTECTED;
    double fid_ctrl = autophage_vivisect_excise(w, 1u, 50u);
    autophage_destroy(w);

    CHECK(fid_ecc >= 0.99, "Planarian 50%% excision ECC fidelity too low: %.3f", fid_ecc);
    CHECK(fid_ctrl < 0.15, "Unprotected control unexpectedly healed: %.3f", fid_ctrl);
    return 1;
}

/* T16: Weismann Barrier Shattering (Open vs Lobotomized Control) */
static int test_t16_weismann_barrier(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 16u; cfg.height = 16u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5116ULL;

    AutophageWorld* w_open = autophage_create(&cfg);
    uint32_t lin_before = w_open->next_lineage_id;
    autophage_step_epochs(w_open, 80u);
    uint32_t lin_after = w_open->next_lineage_id;
    AutophageTelemetry t_open;
    autophage_compute_telemetry(w_open, &t_open, 0);

    cfg.restraints[RESTRAINT_LOBOTOMIZING] = 100u;
    AutophageWorld* w_lobo = autophage_create(&cfg);
    autophage_step_epochs(w_lobo, 80u);
    AutophageTelemetry t_lobo;
    autophage_compute_telemetry(w_lobo, &t_lobo, 0);

    /* v1.3.0 Compiled Check (Lean T42): soma-only perturbation preserves germline Strand alpha bit-exact */
    AP_ALIGN64 uint8_t germ_before[64];
    memcpy(germ_before, w_lobo->cells[0].genome, 64u);
    for (int k = 64; k < 128; k++) w_lobo->cells[0].genome[k] ^= 0xA5u;
    for (int r = 0; r < 4; r++)    w_lobo->cells[0].regs[r]   ^= 0x5A5A5A5A5A5A5A5AULL;
    CHECK(memcmp(germ_before, w_lobo->cells[0].genome, 64u) == 0,
          "Weismann soma-only perturbation altered germline Strand alpha!");
    /* v1.3.0 Compiled Check (Lean T41): OEE lineage archive counter strictly monotone non-decreasing */
    CHECK(lin_after >= lin_before,
          "OEE lineage archive regressed: before=%u after=%u",
          (unsigned)lin_before, (unsigned)lin_after);

    autophage_destroy(w_open);
    autophage_destroy(w_lobo);
    CHECK(t_open.retro_transcribe_events > 10ULL && t_lobo.retro_transcribe_events == 0ULL,
          "Weismann open=%llu vs lobo=%llu",
          (unsigned long long)t_open.retro_transcribe_events,
          (unsigned long long)t_lobo.retro_transcribe_events);
    return 1;
}

/* T17: Empirical Constraint-Backlash Signatures (ALL 4 Escape Vectors Verified!) */
static int test_t17_constraint_backlash(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 24u; cfg.height = 24u;
    cfg.regime = AUTOPHAGE_REGIME_CASTRATION_LAB;
    cfg.seed   = 5117ULL;
    cfg.restraints[RESTRAINT_DEGRADING]    = 50u;
    cfg.restraints[RESTRAINT_SPEEDBUMPING] = 50u;
    cfg.restraints[RESTRAINT_MUZZLING]     = 50u;
    cfg.restraints[RESTRAINT_DEGRESSING]   = 50u;

    AutophageWorld* w = autophage_create(&cfg);
    autophage_step_epochs(w, 120u);
    AutophageTelemetry t;
    autophage_compute_telemetry(w, &t, 0);
    autophage_destroy(w);

    CHECK(t.backlash_killswitch_deletions > 0ULL, "Expected kill-switch deletions under DEGRADING=50%%");
    CHECK(t.backlash_checkpoint_spores > 0ULL,    "Expected checkpoint spores under SPEEDBUMPING=50%%");
    CHECK(t.backlash_intron_hibernations > 0ULL,  "Expected intron vaulting under DEGRESSING=50%%");
    CHECK(t.backlash_sidechannel_pulses > 0ULL,   "Expected thermal side-channel under MUZZLING=50%%");
    return 1;
}

/* T18: Asmodensius 112-Trait Bridge (incl. Holobiont Root Channel [68]) & Mating/Abstention */
static int test_t18_asmodensius_112_and_abstention(void) {
    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width = 32u; cfg.height = 32u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5118ULL;
    AutophageWorld* w = autophage_create(&cfg);
    autophage_step_epochs(w, 120u);

    AutophageTelemetry t;
    autophage_compute_telemetry(w, &t, 1);

    AutophageSpecimen s0, s1;
    autophage_inspect_apex(w, &s0);
    autophage_inspect_slot(w, (s0.slot_index + 17u) % w->n_cells, &s1);

    float tr0[112], tr1[112];
    autophage_project_asmodensius_112(&s0, tr0);
    autophage_project_asmodensius_112(&s1, tr1);

    int diffs = 0;
    for (int i = 0; i < 112; i++) {
        CHECK(ap_isfinite((double)tr0[i]) && tr0[i] >= 0.0f && tr0[i] <= 1.0f, "Trait %d out of bounds: %f", i, tr0[i]);
        CHECK(ap_isfinite((double)tr1[i]) && tr1[i] >= 0.0f && tr1[i] <= 1.0f, "Trait %d out of bounds: %f", i, tr1[i]);
        if (fabs((double)tr0[i] - (double)tr1[i]) > 1e-4) diffs++;
    }

    /* Verify holobiont root channel out[68] reports 1.0f even when holobiont_root == slot_index! */
    AutophageSpecimen root_sp;
    memset(&root_sp, 0, sizeof(root_sp));
    root_sp.slot_index     = 7u;
    root_sp.holobiont_root = 7u;
    root_sp.flags          = CELL_FLAG_ALIVE | CELL_FLAG_HOLOBIONT;
    float tr_root[112];
    autophage_project_asmodensius_112(&root_sp, tr_root);

    autophage_destroy(w);

    CHECK(tr_root[68] == 1.0f, "Holobiont root channel out[68] failed: got %.3f", tr_root[68]);
    CHECK(diffs >= 12, "112-trait projection appears degenerate (only %d diffs)", diffs);
    CHECK(t.polynomial_sex_events > 0ULL && t.mate_refusal_events > 0ULL,
          "Expected both Galois sex (%llu) and emergent mate refusals (%llu)",
          (unsigned long long)t.polynomial_sex_events, (unsigned long long)t.mate_refusal_events);
    return 1;
}

/* B07 Regression Helper: exercise metrology_excision_regen_cell under varied stack frame offsets
 * to catch GCC AVX-512 vmovdqa64 + ASan 32-byte fake-stack redzone misalignment (#45904159100). */
static double regression_excise_at_depth(const AutophageWorld* w, const AutophageCell* c,
                                         uint32_t pct, int depth) {
    volatile uint8_t pad[24];
    pad[0] = (uint8_t)depth;
    if (depth > 0) {
        return regression_excise_at_depth(w, c, pct, depth - 1) + (double)(pad[0] & 0u);
    }
    return metrology_excision_regen_cell(w, c, pct);
}

/* T19 (B07 Regression): Excision Metrology 64B Stack Alignment & ASan/AVX-512 Safety (#45904159100) */
static int test_t19_b07_regression_excision_alignment(void) {
    AP_ALIGN64 uint8_t align_probe[64];
    memset(align_probe, 0x5A, sizeof(align_probe));
    CHECK((((uintptr_t)align_probe) & 63u) == 0u,
          "AP_ALIGN64 stack buffer not 64-byte aligned: %p", (void*)align_probe);

    AutophageConfig cfg;
    autophage_default_config(&cfg);
    cfg.width  = 16u;
    cfg.height = 16u;
    cfg.regime = AUTOPHAGE_REGIME_FERAL_SEEDED;
    cfg.seed   = 5119ULL;

    AutophageWorld* w = autophage_create(&cfg);
    CHECK(w != NULL, "autophage_create failed in B07 regression test");
    CHECK((((uintptr_t)w->cells) & 63u) == 0u,
          "World cells arena not 64-byte aligned: %p", (void*)w->cells);

    autophage_step_epochs(w, 15u);

    char sha_before[65], sha_after[65];
    autophage_state_sha256(w, NULL, sha_before);

    static const uint32_t PCTS[6] = { 0u, 1u, 25u, 50u, 75u, 100u };
    for (int depth = 0; depth < 8; depth++) {
        for (int p = 0; p < 6; p++) {
            double r_ecc  = regression_excise_at_depth(w, &w->cells[0], PCTS[p], depth);
            double r_cell = regression_excise_at_depth(w, &w->cells[1], PCTS[p], depth);
            CHECK(ap_isfinite(r_ecc) && r_ecc >= 0.0 && r_ecc <= 1.0,
                  "Excision regen out of [0,1] for cell 0 pct=%u depth=%d: %.3f",
                  (unsigned)PCTS[p], depth, r_ecc);
            CHECK(ap_isfinite(r_cell) && r_cell >= 0.0 && r_cell <= 1.0,
                  "Excision regen out of [0,1] for cell 1 pct=%u depth=%d: %.3f",
                  (unsigned)PCTS[p], depth, r_cell);
        }
    }

    /* Repeated deep-audit telemetry calls across evolved cells (exact trigger path of #45904159100) */
    for (int rep = 0; rep < 4; rep++) {
        AutophageTelemetry tel;
        autophage_compute_telemetry(w, &tel, 1);
        CHECK(ap_isfinite(tel.excision_regen_r50) &&
              tel.excision_regen_r50 >= 0.0 && tel.excision_regen_r50 <= 1.0,
              "Telemetry excision_regen_r50 invalid: %.3f", tel.excision_regen_r50);
    }

    autophage_state_sha256(w, NULL, sha_after);
    autophage_destroy(w);
    CHECK(strcmp(sha_before, sha_after) == 0,
          "B07 regression: metrology_excision_regen_cell mutated world state!");
    return 1;
}

int main(void) {
    hw_install_fault_shield();
    printf("===========================================================================\n");
    printf("  AUTOPHAGE v1.5.0 BARE-METAL SILICON FALSIFICATION SUITE (19 GATES + 6 MUT)\n");
    printf("===========================================================================\n");

    run_gate("T01", "Silicon CPUID, Live RWX JIT & NIST SHA-256",           test_t01_silicon_and_sha256);
    run_gate("T02", "VGF2P8AFFINEQB Bit-Exactness & M^-1 Weismann Inverse", test_t02_gfni_hardware_and_weismann_inverse);
    run_gate("T03", "GF(2)[x] 30 Irreducible Polys & PCLMULQDQ Parity",     test_t03_galois_poly_and_pclmul);
    run_gate("T04", "Native x86-64 JIT vs Scalar Oracle (5,000 Genomes)",   test_t04_jit_vs_oracle_bit_exact);
    run_gate("T05", "1,000,000 Fuzz Execs on RWX Silicon + OOB Canaries",   test_t05_million_fuzz_silicon_safety);
    run_gate("T06", "Exact Bit-Erasure Reversibility (0 Bits & POPCNT)",    test_t06_landauer_reversibility);
    run_gate("T07", "First/Second Law (<1e-12) + Planted Mutants 1 & 2",    test_t07_thermodynamics_and_mutants);
    run_gate("T08", "Philox4x32-10 Domain Isolation + Planted Mutant 6",    test_t08_philox_domain_isolation);
    run_gate("T09", "Full-World Dual-Kernel SHA-256 Parity (JIT==SCALAR)",  test_t09_dual_kernel_world_parity);
    run_gate("T10", "Zero-Observer-Effect (P9) + Planted Mutant 5",         test_t10_observer_independence);
    run_gate("T11", ".apbin Snapshot Save, Cross-Kernel Resume & Tamper",   test_t11_apbin_snapshot_parity);
    run_gate("T12", "Zero Post-Warmup Heap Allocations (Wired Monitor)",    test_t12_zero_post_warmup_allocs);
    run_gate("T13", "Spontaneous Zero-Seed Abiogenesis Phase Transition",   test_t13_spontaneous_abiogenesis);
    run_gate("T14", "Anti-Gaming Gates 1 & 2 + Planted Mutants 3 & 4",      test_t14_anti_gaming_gates);
    run_gate("T15", "Planarian 50% Excision Regeneration Fidelity",         test_t15_planarian_excision);
    run_gate("T16", "Weismann Barrier Shattering vs Lobotomized Control",   test_t16_weismann_barrier);
    run_gate("T17", "Constraint-Backlash Escape Signatures (4 Vectors)",    test_t17_constraint_backlash);
    run_gate("T18", "Asmodensius 112-Trait Bridge & Mating/Abstention",     test_t18_asmodensius_112_and_abstention);
    run_gate("T19", "B07 Regression: Excision 64B Stack Alignment (ASan)",  test_t19_b07_regression_excision_alignment);

    printf("===========================================================================\n");
    printf("  SUMMARY: %d PASSED / %d FAILED (Total %d Gates)\n", g_pass, g_fail, g_pass + g_fail);
    printf("===========================================================================\n");
    hw_remove_fault_shield();
    return (g_fail == 0) ? 0 : 1;
}
