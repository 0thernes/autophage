/* ============================================================================
 * AUTOPHAGE v1.4.0 — libFuzzer Differential JIT vs Scalar Oracle Harness
 *
 * Build & run under Linux/Clang with AddressSanitizer + UndefinedBehaviorSanitizer:
 *   clang -O1 -g -fsanitize=fuzzer,address,undefined -DAUTOPHAGE_CI_PORTABLE \
 *     -Iinclude src/hw_isa.c src/galois_math.c src/jit_x86_64.c \
 *     src/soup_engine.c src/metrology_ovbs.c src/abi_exports.c \
 *     test/fuzz_jit_libfuzzer.c -o bin/fuzz_jit_libfuzzer -lm
 *   ./bin/fuzz_jit_libfuzzer -max_total_time=30
 * ============================================================================ */

#include "autophage_internal.h"
#include <stdlib.h>

static AutophageHwCaps  g_fuzz_caps;
static AutophageJitCache g_fuzz_cache;
static int              g_fuzz_ready = 0;

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 80u) return 0;

    if (!g_fuzz_ready) {
        autophage_probe_hardware(&g_fuzz_caps);
        if (!jit_cache_init(&g_fuzz_cache, &g_fuzz_caps)) {
            abort();
        }
        g_fuzz_ready = 1;
    }

    uint8_t expr[64];
    memcpy(expr, data, 64u);

    uint64_t smask = 0;
    memcpy(&smask, data + 64u, 8u);
    uint64_t ribo_mat = 0;
    memcpy(&ribo_mat, data + 72u, 8u);
    uint8_t nerf = (size > 80u) ? (uint8_t)((data[80] % 5u) * 25u) : 0u;

    /* 64-byte guard canaries before and after both 128-byte organism tapes */
    uint8_t buf_a[256], buf_b[256], ref_a[128], ref_b[128];
    memset(buf_a, 0xA5, sizeof(buf_a));
    memset(buf_b, 0x5A, sizeof(buf_b));
    uint8_t* ta = buf_a + 64;
    uint8_t* tb = buf_b + 64;
    for (int i = 0; i < 128; i++) {
        ta[i] = (uint8_t)(data[(size_t)i % size] ^ (uint8_t)i);
        tb[i] = (uint8_t)(data[((size_t)i * 7u + 3u) % size] ^ 0x55u);
    }
    memcpy(ref_a, ta, 128u);
    memcpy(ref_b, tb, 128u);

    AutophageExecCtx ctx_jit, ctx_ref;
    memset(&ctx_jit, 0, sizeof(ctx_jit));
    ctx_jit.regs[0]       = smask ^ 0x1122334455667788ULL;
    ctx_jit.regs[1]       = ribo_mat ^ 0xF0F0F0F00F0F0F0FULL;
    ctx_jit.regs[2]       = 0xCAFEBABEDEADBEEFULL;
    ctx_jit.regs[3]       = 0x0123456789ABCDEFULL;
    ctx_jit.splice_mask   = smask;
    ctx_jit.ribosome_mat  = ribo_mat;
    ctx_jit.ribosome_rank = (uint32_t)gf2_mat_rank(ribo_mat);
    ctx_jit.env_wave      = 0x13579BDF2468ACE0ULL;
    ctx_jit.clade_xor     = (uint64_t)(smask & 0xFFFFFFFFULL);
    ctx_jit.stigmergy_in  = 0x0F1E2D3C4B5A6978ULL;
    ctx_jit.gas           = 32u;
    ctx_jit.head0         = (uint32_t)(data[0] & 0x7Fu);
    ctx_jit.head1         = (uint32_t)(data[1] & 0x7Fu);
    ctx_ref = ctx_jit;

    autophage_jit_fn fn = jit_compile_or_fetch(&g_fuzz_cache, expr, smask, nerf, NULL, NULL, 0);
    uint64_t r_jit = fn(ta, tb, &ctx_jit);
    uint64_t r_ref = jit_reference_oracle(expr, smask, nerf, ref_a, ref_b, &ctx_ref);

    if (r_jit != r_ref ||
        memcmp(&ctx_jit, &ctx_ref, sizeof(ctx_jit)) != 0 ||
        memcmp(ta, ref_a, 128u) != 0 ||
        memcmp(tb, ref_b, 128u) != 0) {
        abort();
    }
    for (int i = 0; i < 64; i++) {
        if (buf_a[i] != 0xA5 || buf_a[192 + i] != 0xA5 ||
            buf_b[i] != 0x5A || buf_b[192 + i] != 0x5A) {
            abort();
        }
    }
    return 0;
}
