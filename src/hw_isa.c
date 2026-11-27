#include "autophage_internal.h"

/* ============================================================================
 * CPUID SILICON DISCOVERY, HARDWARE ENTROPY, RWX ALLOCATOR & FIPS 180-4 SHA256
 * ============================================================================ */

static int      g_hw_caps_cached  = 0;
static uint32_t g_hw_has_rdtscp   = 0;
static uint32_t g_hw_has_rdseed   = 0;
static uint32_t g_hw_has_rdrand   = 0;
static uint32_t g_hw_has_pclmul   = 0;
static uint32_t g_hw_has_gfni     = 0;
static uint32_t g_hw_has_avx2     = 0;

static void cpuid_ex(uint32_t leaf, uint32_t subleaf,
                     uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    uint32_t a, b, c, d;
    __asm__ volatile (
        "cpuid"
        : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
        : "a"(leaf), "c"(subleaf)
    );
    *eax = a; *ebx = b; *ecx = c; *edx = d;
}

static void hw_ensure_runtime_caps(void) {
    if (g_hw_caps_cached) return;
    uint32_t a = 0, b = 0, c = 0, d = 0;
    cpuid_ex(0, 0, &a, &b, &c, &d);
    uint32_t max_leaf = a;
    if (max_leaf >= 1u) {
        cpuid_ex(1, 0, &a, &b, &c, &d);
        g_hw_has_pclmul = (c >> 1) & 1u;
        g_hw_has_rdrand = (c >> 30) & 1u;
    }
    if (max_leaf >= 7u) {
        cpuid_ex(7, 0, &a, &b, &c, &d);
        g_hw_has_avx2   = (b >> 5) & 1u;
        g_hw_has_rdseed = (b >> 18) & 1u;
        g_hw_has_gfni   = (c >> 8) & 1u;
    }
    cpuid_ex(0x80000000u, 0, &a, &b, &c, &d);
    if (a >= 0x80000001u) {
        cpuid_ex(0x80000001u, 0, &a, &b, &c, &d);
        g_hw_has_rdtscp = (d >> 27) & 1u;
    }
    g_hw_caps_cached = 1;
}

uint64_t hw_rdtscp(void) {
    hw_ensure_runtime_caps();
    uint32_t lo = 0, hi = 0;
    if (g_hw_has_rdtscp) {
        uint32_t aux = 0;
        __asm__ volatile ("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));
    } else {
        __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    }
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

void hw_detect_caps(AutophageHwCaps* caps) {
    memset(caps, 0, sizeof(*caps));
    uint32_t a = 0, b = 0, c = 0, d = 0;

    /* Leaf 0: Vendor string + max leaf */
    cpuid_ex(0, 0, &a, &b, &c, &d);
    uint32_t max_leaf = a;
    memcpy(caps->vendor + 0, &b, 4);
    memcpy(caps->vendor + 4, &d, 4);
    memcpy(caps->vendor + 8, &c, 4);
    caps->vendor[12] = '\0';

    /* Leaf 1: PCLMULQDQ, POPCNT, RDRAND */
    if (max_leaf >= 1) {
        cpuid_ex(1, 0, &a, &b, &c, &d);
        caps->has_pclmulqdq = (c >> 1) & 1u;
        caps->has_popcnt    = (c >> 23) & 1u;
        caps->has_rdrand    = (c >> 30) & 1u;
    }

    /* Leaf 7 subleaf 0: BMI1, AVX2, BMI2, RDSEED, GFNI, VPCLMULQDQ */
    if (max_leaf >= 7) {
        cpuid_ex(7, 0, &a, &b, &c, &d);
        caps->has_bmi1      = (b >> 3) & 1u;
        caps->has_avx2      = (b >> 5) & 1u;
        caps->has_bmi2      = (b >> 8) & 1u;
        caps->has_rdseed    = (b >> 18) & 1u;
        caps->has_gfni      = (c >> 8) & 1u;
        caps->has_vpclmulqdq= (c >> 10) & 1u;
    }

    /* Extended leaves: RDTSCP + Processor Brand String (strict-aliasing clean!) */
    cpuid_ex(0x80000000u, 0, &a, &b, &c, &d);
    uint32_t max_ext = a;
    if (max_ext >= 0x80000001u) {
        cpuid_ex(0x80000001u, 0, &a, &b, &c, &d);
        caps->has_rdtscp = (d >> 27) & 1u;
    }
    if (max_ext >= 0x80000004u) {
        for (uint32_t i = 0; i < 3; i++) {
            uint32_t regs[4];
            cpuid_ex(0x80000002u + i, 0, &regs[0], &regs[1], &regs[2], &regs[3]);
            memcpy(caps->brand + i * 16u, regs, sizeof(regs));
        }
        caps->brand[48] = '\0';
        /* Trim leading spaces */
        char* p = caps->brand;
        while (*p == ' ') p++;
        if (p != caps->brand) {
            size_t len = strlen(p);
            memmove(caps->brand, p, len + 1);
        }
    }

    /* Sync runtime dispatch flags */
    g_hw_has_rdtscp  = caps->has_rdtscp;
    g_hw_has_rdseed  = caps->has_rdseed;
    g_hw_has_rdrand  = caps->has_rdrand;
    g_hw_has_pclmul  = caps->has_pclmulqdq;
    g_hw_has_gfni    = caps->has_gfni;
    g_hw_has_avx2    = caps->has_avx2;
    g_hw_caps_cached = 1;

    /* Verify live RWX JIT code page execution on silicon */
    uint8_t* stub = (uint8_t*)hw_alloc_rwx(4096);
    if (stub) {
        /* mov eax, 0x1337C0DE ; ret */
        static const uint8_t code[6] = { 0xB8, 0xDE, 0xC0, 0x37, 0x13, 0xC3 };
        memcpy(stub, code, sizeof(code));
        hw_flush_icache(stub, sizeof(code));
        typedef uint32_t (*probe_fn)(void);
        probe_fn fn = (probe_fn)(uintptr_t)stub;
        if (fn() == 0x1337C0DEu) {
            caps->jit_rwx_verified = 1u;
        }
        hw_free_rwx(stub, 4096);
    }
}

uint64_t hw_entropy_seed64(void) {
    hw_ensure_runtime_caps();
    uint64_t val = 0;
    unsigned char ok = 0;
#if defined(__RDSEED__)
    if (g_hw_has_rdseed) {
        for (int i = 0; i < 32; i++) {
            __asm__ volatile ("rdseed %0; setc %1" : "=r"(val), "=qm"(ok));
            if (ok && val != 0) return val;
        }
    }
#endif
#if defined(__RDRND__)
    if (g_hw_has_rdrand) {
        for (int i = 0; i < 32; i++) {
            __asm__ volatile ("rdrand %0; setc %1" : "=r"(val), "=qm"(ok));
            if (ok && val != 0) return val;
        }
    }
#endif
    (void)ok;
    /* Fallback: RDTSCP/RDTSC mixed with stack address via SplitMix64 */
    uint64_t t = hw_rdtscp() ^ (uint64_t)(uintptr_t)&val;
    t += 0x9E3779B97F4A7C15ULL;
    t = (t ^ (t >> 30)) * 0xBF58476D1CE4E5B9ULL;
    t = (t ^ (t >> 27)) * 0x94D049BB133111EBULL;
    t ^= (t >> 31);
    return t ? t : 0xCAFEBABE1337F00DULL;
}

void* hw_alloc_rwx(size_t bytes) {
#if defined(_WIN32)
    return VirtualAlloc(NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    void* p = mmap(NULL, bytes, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? NULL : p;
#endif
}

void hw_free_rwx(void* ptr, size_t bytes) {
    if (!ptr) return;
#if defined(_WIN32)
    (void)bytes;
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, bytes);
#endif
}

void hw_flush_icache(void* ptr, size_t bytes) {
#if defined(_WIN32)
    FlushInstructionCache(GetCurrentProcess(), ptr, bytes);
#else
    __builtin___clear_cache((char*)ptr, (char*)ptr + bytes);
#endif
}

double hw_wall_seconds(void) {
#if defined(_WIN32)
    static int64_t freq = 0;
    if (freq == 0) {
        QueryPerformanceFrequency(&freq);
        if (freq == 0) freq = 1;
    }
    int64_t now = 0;
    QueryPerformanceCounter(&now);
    return (double)now / (double)freq;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

/* ============================================================================
 * HARDWARE SILICON PCLMULQDQ & VGF2P8AFFINEQB KERNELS
 * ============================================================================ */

uint64_t hw_pclmul64_lo(uint64_t a, uint64_t b) {
    hw_ensure_runtime_caps();
#if defined(__PCLMUL__)
    if (g_hw_has_pclmul) {
        uint64_t lo = 0;
        __asm__ (
            "movq %1, %%xmm0\n\t"
            "movq %2, %%xmm1\n\t"
            "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
            "movq %%xmm0, %0\n\t"
            : "=r"(lo)
            : "r"(a), "r"(b)
            : "xmm0", "xmm1"
        );
        return lo;
    }
#endif
    uint64_t lo = 0;
    for (int i = 0; i < 64; i++) {
        if ((b >> i) & 1ULL) lo ^= (a << i);
    }
    return lo;
}

void hw_pclmul64_full(uint64_t a, uint64_t b, uint64_t* out_lo, uint64_t* out_hi) {
    hw_ensure_runtime_caps();
#if defined(__PCLMUL__)
    if (g_hw_has_pclmul) {
        uint64_t lo = 0, hi = 0;
        __asm__ (
            "movq %2, %%xmm0\n\t"
            "movq %3, %%xmm1\n\t"
            "pclmulqdq $0x00, %%xmm1, %%xmm0\n\t"
            "movq %%xmm0, %0\n\t"
            "psrldq $8, %%xmm0\n\t"
            "movq %%xmm0, %1\n\t"
            : "=r"(lo), "=r"(hi)
            : "r"(a), "r"(b)
            : "xmm0", "xmm1"
        );
        *out_lo = lo;
        *out_hi = hi;
        return;
    }
#endif
    uint64_t lo = 0, hi = 0;
    for (int i = 0; i < 64; i++) {
        if ((b >> i) & 1ULL) {
            lo ^= (a << i);
            if (i > 0) hi ^= (a >> (64 - i));
        }
    }
    *out_lo = lo;
    *out_hi = hi;
}

void hw_gfni_affine_64bytes(const uint8_t in[64], uint8_t out[64],
                            uint64_t mat, uint8_t bias, int use_hw_gfni) {
    hw_ensure_runtime_caps();
#if defined(__GFNI__) && defined(__AVX2__)
    if (use_hw_gfni && g_hw_has_gfni && g_hw_has_avx2) {
        uint64_t bias_q = (uint64_t)bias * 0x0101010101010101ULL;
        __asm__ volatile (
            "vpbroadcastq %2, %%ymm2\n\t"
            "vpbroadcastq %3, %%ymm3\n\t"
            "vmovdqu (%0), %%ymm0\n\t"
            "vmovdqu 32(%0), %%ymm1\n\t"
            "vgf2p8affineqb $0x00, %%ymm2, %%ymm0, %%ymm0\n\t"
            "vgf2p8affineqb $0x00, %%ymm2, %%ymm1, %%ymm1\n\t"
            "vpxor %%ymm3, %%ymm0, %%ymm0\n\t"
            "vpxor %%ymm3, %%ymm1, %%ymm1\n\t"
            "vmovdqu %%ymm0, (%1)\n\t"
            "vmovdqu %%ymm1, 32(%1)\n\t"
            "vzeroupper\n\t"
            :
            : "r"(in), "r"(out), "m"(mat), "m"(bias_q)
            : "ymm0", "ymm1", "ymm2", "ymm3", "memory"
        );
        return;
    }
#else
    (void)use_hw_gfni;
#endif
    for (int i = 0; i < 64; i++) {
        out[i] = gf2_mat_apply_byte(mat, in[i], bias);
    }
}

/* ============================================================================
 * HARDWARE FAULT SHIELD (Diagnostics Only — JIT is Provably Memory-Safe)
 * ============================================================================ */

#if defined(_WIN32)
static void* g_veh_handle = NULL;
static volatile uint64_t g_fault_count = 0;

static long __stdcall autophage_veh_handler(EXCEPTION_POINTERS* info) {
    if (info && info->ExceptionRecord) {
        DWORD code = info->ExceptionRecord->ExceptionCode;
        if (code == 0xC0000005u || code == 0xC000001Du || code == 0xC0000094u) {
            g_fault_count++;
            fprintf(stderr, "[AUTOPHAGE FATAL FAULT] Exception 0x%08X at %p (count=%llu)\n",
                    (unsigned)code, info->ExceptionRecord->ExceptionAddress,
                    (unsigned long long)g_fault_count);
            fflush(stderr);
        }
    }
    return 0; /* EXCEPTION_CONTINUE_SEARCH */
}

void hw_install_fault_shield(void) {
    if (!g_veh_handle) {
        g_veh_handle = AddVectoredExceptionHandler(1, autophage_veh_handler);
    }
}

void hw_remove_fault_shield(void) {
    if (g_veh_handle) {
        RemoveVectoredExceptionHandler(g_veh_handle);
        g_veh_handle = NULL;
    }
}
#else
void hw_install_fault_shield(void) {}
void hw_remove_fault_shield(void) {}
#endif

/* ============================================================================
 * SELF-CONTAINED FIPS 180-4 SHA-256 (Canonical State & Parity Verification)
 * ============================================================================ */

static const uint32_t SHA256_K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static inline uint32_t sha_rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32u - n));
}

static void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4 + 0] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8)  |
               ((uint32_t)block[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = sha_rotr(w[i - 15], 7) ^ sha_rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = sha_rotr(w[i - 2], 17) ^ sha_rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t S1 = sha_rotr(e, 6) ^ sha_rotr(e, 11) ^ sha_rotr(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + SHA256_K[i] + w[i];
        uint32_t S0 = sha_rotr(a, 2) ^ sha_rotr(a, 13) ^ sha_rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256_init(Sha256Ctx* ctx) {
    ctx->state[0] = 0x6a09e667u; ctx->state[1] = 0xbb67ae85u;
    ctx->state[2] = 0x3c6ef372u; ctx->state[3] = 0xa54ff53au;
    ctx->state[4] = 0x510e527fu; ctx->state[5] = 0x9b05688cu;
    ctx->state[6] = 0x1f83d9abu; ctx->state[7] = 0x5be0cd19u;
    ctx->total_bytes = 0;
    ctx->buf_len = 0;
}

void sha256_update(Sha256Ctx* ctx, const void* data, size_t len) {
    const uint8_t* p = (const uint8_t*)data;
    ctx->total_bytes += (uint64_t)len;
    if (ctx->buf_len > 0) {
        size_t need = 64u - ctx->buf_len;
        size_t take = len < need ? len : need;
        memcpy(ctx->buf + ctx->buf_len, p, take);
        ctx->buf_len += take;
        p += take;
        len -= take;
        if (ctx->buf_len == 64u) {
            sha256_transform(ctx->state, ctx->buf);
            ctx->buf_len = 0;
        }
    }
    while (len >= 64u) {
        sha256_transform(ctx->state, p);
        p += 64u;
        len -= 64u;
    }
    if (len > 0) {
        memcpy(ctx->buf, p, len);
        ctx->buf_len = len;
    }
}

void sha256_final(Sha256Ctx* ctx, uint8_t out[32]) {
    AP_ALIGN64 uint8_t tail[128];
    memset(tail, 0, sizeof(tail));
    if (ctx->buf_len > 0) memcpy(tail, ctx->buf, ctx->buf_len);
    tail[ctx->buf_len] = 0x80u;

    size_t pad_blocks = (ctx->buf_len < 56u) ? 1u : 2u;
    uint64_t bit_len = ctx->total_bytes * 8ULL;
    size_t len_pos = pad_blocks * 64u - 8u;
    for (int i = 0; i < 8; i++) {
        tail[len_pos + i] = (uint8_t)(bit_len >> (56 - i * 8));
    }
    sha256_transform(ctx->state, tail);
    if (pad_blocks == 2u) {
        sha256_transform(ctx->state, tail + 64u);
    }
    for (int i = 0; i < 8; i++) {
        out[i * 4 + 0] = (uint8_t)(ctx->state[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        out[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

void sha256_compute(const uint8_t* data, size_t len, uint8_t out[32]) {
    Sha256Ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}

void sha256_to_hex(const uint8_t digest[32], char hex[65]) {
    static const char HEX[17] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        hex[i * 2 + 0] = HEX[(digest[i] >> 4) & 0xFu];
        hex[i * 2 + 1] = HEX[digest[i] & 0xFu];
    }
    hex[64] = '\0';
}
