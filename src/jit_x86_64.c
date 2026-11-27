#include "autophage_internal.h"

/* ============================================================================
 * AUTOPHAGE BARE-METAL POLYMORPHIC x86-64 MACHINE-CODE JIT COMPILER
 * + BIT-EXACT SCALAR REFERENCE ORACLE + REAL x86-64/CODON DISASSEMBLER
 *
 * Register Map (Identical inside body on both Win64 & SysV AMD64!):
 *   R12 = tape_a (uint8_t[128])
 *   R13 = tape_b (uint8_t[128])
 *   R14 = ctx    (AutophageExecCtx*)
 *   R15 = gas    (signed countdown; strictly decreases on every backward jump)
 *   RBP = head0  (always masked & 0x7F -> provably in [0..127])
 *   R9  = head1  (always masked & 0x7F -> provably in [0..127])
 *   R10 = rev_ops counter
 *   R11 = erased_bits counter (accumulated via hardware POPCNT)
 *   RBX, RSI, RDI, R8 = organism somatic registers R[0..3]
 *   RAX, RCX, RDX = scratch GPRs
 *   XMM0, XMM1 = scratch 128-bit registers (caller-saved on BOTH Win64 & SysV!)
 * ============================================================================ */

enum {
    X_RAX = 0, X_RCX = 1, X_RDX = 2, X_RBX = 3,
    X_RSP = 4, X_RBP = 5, X_RSI = 6, X_RDI = 7,
    X_R8  = 8, X_R9  = 9, X_R10 = 10, X_R11 = 11,
    X_R12 = 12, X_R13 = 13, X_R14 = 14, X_R15 = 15
};

static const uint8_t ORG_REG[4] = { X_RBX, X_RSI, X_RDI, X_R8 };

/* AutophageExecCtx field byte offsets (verified identically by _Static_assert!) */
#define CTX_OFF_REG0           0x00u
#define CTX_OFF_REG1           0x08u
#define CTX_OFF_REG2           0x10u
#define CTX_OFF_REG3           0x18u
#define CTX_OFF_SPLICE         0x20u
#define CTX_OFF_RIBOMAT        0x28u
#define CTX_OFF_ENVWAVE        0x30u
#define CTX_OFF_CLADEXOR       0x38u
#define CTX_OFF_STIG_IN        0x40u
#define CTX_OFF_STIG_OUT       0x48u
#define CTX_OFF_BIO_ACT        0x50u
#define CTX_OFF_HARVEST_PARAM  0x58u
#define CTX_OFF_GAS            0x60u
#define CTX_OFF_ERASED         0x64u
#define CTX_OFF_REVOPS         0x68u
#define CTX_OFF_IRROPS         0x6Cu
#define CTX_OFF_COPY_A2B       0x70u
#define CTX_OFF_COPY_B2A       0x74u
#define CTX_OFF_HEAD0          0x78u
#define CTX_OFF_HEAD1          0x7Cu
#define CTX_OFF_MEMBRANE_PARAM 0x80u
#define CTX_OFF_RETRO_PARAM    0x88u
#define CTX_OFF_RIBO_PARAM     0x90u
#define CTX_OFF_CENTRO_PARAM   0x98u
#define CTX_OFF_HOLO_PARAM     0xA0u
#define CTX_OFF_MITOSIS_PARAM  0xA8u
#define CTX_OFF_RIBO_RANK      0xB0u

_Static_assert(offsetof(AutophageExecCtx, harvest_param)  == CTX_OFF_HARVEST_PARAM,  "harvest_param offset mismatch");
_Static_assert(offsetof(AutophageExecCtx, membrane_param) == CTX_OFF_MEMBRANE_PARAM, "membrane_param offset mismatch");
_Static_assert(offsetof(AutophageExecCtx, retro_param)    == CTX_OFF_RETRO_PARAM,    "retro_param offset mismatch");
_Static_assert(offsetof(AutophageExecCtx, ribo_param)     == CTX_OFF_RIBO_PARAM,     "ribo_param offset mismatch");
_Static_assert(offsetof(AutophageExecCtx, centro_param)   == CTX_OFF_CENTRO_PARAM,   "centro_param offset mismatch");
_Static_assert(offsetof(AutophageExecCtx, holo_param)     == CTX_OFF_HOLO_PARAM,     "holo_param offset mismatch");
_Static_assert(offsetof(AutophageExecCtx, mitosis_param)  == CTX_OFF_MITOSIS_PARAM,  "mitosis_param offset mismatch");
_Static_assert(offsetof(AutophageExecCtx, ribosome_rank)  == CTX_OFF_RIBO_RANK,      "ribosome_rank offset mismatch");

/* Smooth deterministic per-locus NERFING gate (0% = all active, 100% = all stripped, no binary cliff!) */
static inline int hw_ext_allowed(uint8_t orig_pc, uint8_t op, uint8_t nerf_pct) {
    if (nerf_pct == 0u) return 1;
    if (nerf_pct >= 100u) return 0;
    return (((uint32_t)orig_pc * 37u + (uint32_t)op * 13u) % 100u) >= (uint32_t)nerf_pct;
}

typedef struct JitBuf {
    uint8_t* ptr;
    uint32_t len;
    uint32_t cap;
} JitBuf;

static inline void eb(JitBuf* b, uint8_t v) {
    if (b->len < b->cap) b->ptr[b->len++] = v;
}

static inline void e32(JitBuf* b, uint32_t v) {
    eb(b, (uint8_t)(v & 0xFFu));
    eb(b, (uint8_t)((v >> 8) & 0xFFu));
    eb(b, (uint8_t)((v >> 16) & 0xFFu));
    eb(b, (uint8_t)((v >> 24) & 0xFFu));
}

static inline void e64(JitBuf* b, uint64_t v) {
    e32(b, (uint32_t)(v & 0xFFFFFFFFULL));
    e32(b, (uint32_t)(v >> 32));
}

static uint64_t scalar_pext64(uint64_t src, uint64_t mask);
static uint64_t scalar_pdep64(uint64_t src, uint64_t mask);
static uint64_t scalar_pclmul64_lo(uint64_t a, uint64_t b);
static uint64_t scalar_gf2p8affine_u64(uint64_t x, uint64_t mat, uint8_t bias);

static inline void patch32(JitBuf* b, uint32_t at, int32_t rel) {
    if (at + 4u > b->cap) return;
    uint32_t u = (uint32_t)rel;
    b->ptr[at + 0] = (uint8_t)(u & 0xFFu);
    b->ptr[at + 1] = (uint8_t)((u >> 8) & 0xFFu);
    b->ptr[at + 2] = (uint8_t)((u >> 16) & 0xFFu);
    b->ptr[at + 3] = (uint8_t)((u >> 24) & 0xFFu);
}

static inline uint8_t mk_rex(int w, uint8_t r, uint8_t x, uint8_t b_reg) {
    return (uint8_t)(0x40u | ((w ? 1u : 0u) << 3) |
                     (((r >> 3) & 1u) << 2) |
                     (((x >> 3) & 1u) << 1) |
                     ((b_reg >> 3) & 1u));
}

static inline uint8_t mk_modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
    return (uint8_t)(((mod & 3u) << 6) | ((reg & 7u) << 3) | (rm & 7u));
}

static inline uint8_t mk_sib(uint8_t scale, uint8_t idx, uint8_t base) {
    return (uint8_t)(((scale & 3u) << 6) | ((idx & 7u) << 3) | (base & 7u));
}

static inline void emit_alu_rr(JitBuf* b, uint8_t opcode, uint8_t dst, uint8_t src) {
    eb(b, mk_rex(1, src, 0, dst));
    eb(b, opcode);
    eb(b, mk_modrm(3, src, dst));
}

static inline void emit_mov_rr(JitBuf* b, uint8_t dst, uint8_t src) { emit_alu_rr(b, 0x89, dst, src); }
static inline void emit_xor_rr(JitBuf* b, uint8_t dst, uint8_t src) { emit_alu_rr(b, 0x31, dst, src); }
static inline void emit_add_rr(JitBuf* b, uint8_t dst, uint8_t src) { emit_alu_rr(b, 0x01, dst, src); }
static inline void emit_sub_rr(JitBuf* b, uint8_t dst, uint8_t src) { emit_alu_rr(b, 0x29, dst, src); }
static inline void emit_and_rr(JitBuf* b, uint8_t dst, uint8_t src) { emit_alu_rr(b, 0x21, dst, src); }
static inline void emit_cmp_rr(JitBuf* b, uint8_t dst, uint8_t src) { emit_alu_rr(b, 0x39, dst, src); }

static inline void emit_rol_r_imm8(JitBuf* b, uint8_t dst, uint8_t shift) {
    eb(b, mk_rex(1, 0, 0, dst));
    eb(b, 0xC1);
    eb(b, mk_modrm(3, 0, dst));
    eb(b, shift);
}

static inline void emit_inc_r64(JitBuf* b, uint8_t dst) {
    eb(b, mk_rex(1, 0, 0, dst));
    eb(b, 0xFF);
    eb(b, mk_modrm(3, 0, dst));
}

static inline void emit_dec_r64(JitBuf* b, uint8_t dst) {
    eb(b, mk_rex(1, 0, 0, dst));
    eb(b, 0xFF);
    eb(b, mk_modrm(3, 1, dst));
}

static inline void emit_and_r32_imm8(JitBuf* b, uint8_t dst, uint8_t imm7) {
    /* imm7 must have bit 7 == 0 (e.g. 0x7F or 0x78) for positive sign-extension */
    eb(b, mk_rex(0, 0, 0, dst));
    eb(b, 0x83);
    eb(b, mk_modrm(3, 4, dst));
    eb(b, imm7);
}

static inline void emit_load_ctx64(JitBuf* b, uint8_t dst, uint32_t off) {
    eb(b, mk_rex(1, dst, 0, X_R14));
    eb(b, 0x8B);
    if (off < 0x80u) {
        eb(b, mk_modrm(1, dst, X_R14));
        eb(b, (uint8_t)off);
    } else {
        eb(b, mk_modrm(2, dst, X_R14));
        e32(b, off);
    }
}

static inline void emit_store_ctx64(JitBuf* b, uint32_t off, uint8_t src) {
    eb(b, mk_rex(1, src, 0, X_R14));
    eb(b, 0x89);
    if (off < 0x80u) {
        eb(b, mk_modrm(1, src, X_R14));
        eb(b, (uint8_t)off);
    } else {
        eb(b, mk_modrm(2, src, X_R14));
        e32(b, off);
    }
}

static inline void emit_load_ctx32(JitBuf* b, uint8_t dst, uint32_t off) {
    eb(b, mk_rex(0, dst, 0, X_R14));
    eb(b, 0x8B);
    if (off < 0x80u) {
        eb(b, mk_modrm(1, dst, X_R14));
        eb(b, (uint8_t)off);
    } else {
        eb(b, mk_modrm(2, dst, X_R14));
        e32(b, off);
    }
}

static inline void emit_store_ctx32(JitBuf* b, uint32_t off, uint8_t src) {
    eb(b, mk_rex(0, src, 0, X_R14));
    eb(b, 0x89);
    if (off < 0x80u) {
        eb(b, mk_modrm(1, src, X_R14));
        eb(b, (uint8_t)off);
    } else {
        eb(b, mk_modrm(2, src, X_R14));
        e32(b, off);
    }
}

static inline void emit_inc_ctx32(JitBuf* b, uint8_t off) {
    /* add dword ptr [r14 + off], 1 */
    eb(b, 0x41);
    eb(b, 0x83);
    eb(b, 0x46);
    eb(b, off);
    eb(b, 0x01);
}

static inline void emit_or_ctx_bio(JitBuf* b, uint32_t bitmask) {
    /* or dword ptr [r14 + CTX_OFF_BIO_ACT], imm32 */
    eb(b, 0x41);
    eb(b, 0x81);
    eb(b, 0x4E);
    eb(b, CTX_OFF_BIO_ACT);
    e32(b, bitmask);
}

/* popcnt rax, rax ; add r11, rax */
static inline void emit_popcnt_rax_add_r11(JitBuf* b) {
    eb(b, 0xF3); eb(b, 0x48); eb(b, 0x0F); eb(b, 0xB8); eb(b, 0xC0);
    emit_add_rr(b, X_R11, X_RAX);
}

/* Load byte from tape_a[rbp] -> eax (zero-extended) */
static inline void emit_load_ta_u8_eax(JitBuf* b) {
    eb(b, mk_rex(0, X_RAX, X_RBP, X_R12));
    eb(b, 0x0F); eb(b, 0xB6);
    eb(b, mk_modrm(0, X_RAX, 4));
    eb(b, mk_sib(0, X_RBP, X_R12));
}

/* Load byte from tape_b[r9] -> edx (zero-extended; r13 requires mod=01 disp8=0!) */
static inline void emit_load_tb_u8_edx(JitBuf* b) {
    eb(b, mk_rex(0, X_RDX, X_R9, X_R13));
    eb(b, 0x0F); eb(b, 0xB6);
    eb(b, mk_modrm(1, X_RDX, 4));
    eb(b, mk_sib(0, X_R9, X_R13));
    eb(b, 0x00);
}

/* Store byte dl -> tape_a[rbp] */
static inline void emit_store_ta_u8_dl(JitBuf* b) {
    eb(b, mk_rex(0, X_RDX, X_RBP, X_R12));
    eb(b, 0x88);
    eb(b, mk_modrm(0, X_RDX, 4));
    eb(b, mk_sib(0, X_RBP, X_R12));
}

/* Store byte al -> tape_b[r9] */
static inline void emit_store_tb_u8_al(JitBuf* b) {
    eb(b, mk_rex(0, X_RAX, X_R9, X_R13));
    eb(b, 0x88);
    eb(b, mk_modrm(1, X_RAX, 4));
    eb(b, mk_sib(0, X_R9, X_R13));
    eb(b, 0x00);
}

/* Load qword from tape_a[rax] or tape_b[rax] into rcx */
static inline void emit_load_tape_u64_rcx(JitBuf* b, int is_b) {
    if (!is_b) {
        eb(b, mk_rex(1, X_RCX, X_RAX, X_R12));
        eb(b, 0x8B);
        eb(b, mk_modrm(0, X_RCX, 4));
        eb(b, mk_sib(0, X_RAX, X_R12));
    } else {
        eb(b, mk_rex(1, X_RCX, X_RAX, X_R13));
        eb(b, 0x8B);
        eb(b, mk_modrm(1, X_RCX, 4));
        eb(b, mk_sib(0, X_RAX, X_R13));
        eb(b, 0x00);
    }
}

/* Store qword rdx into tape_a[rax] or tape_b[rax] */
static inline void emit_store_tape_u64_rdx(JitBuf* b, int is_b) {
    if (!is_b) {
        eb(b, mk_rex(1, X_RDX, X_RAX, X_R12));
        eb(b, 0x89);
        eb(b, mk_modrm(0, X_RDX, 4));
        eb(b, mk_sib(0, X_RAX, X_R12));
    } else {
        eb(b, mk_rex(1, X_RDX, X_RAX, X_R13));
        eb(b, 0x89);
        eb(b, mk_modrm(1, X_RDX, 4));
        eb(b, mk_sib(0, X_RAX, X_R13));
        eb(b, 0x00);
    }
}

/* Compact active spliced exons & precompute identical control-flow tables for JIT & Oracle */
typedef struct CompiledPlan {
    uint8_t codons[64];
    uint8_t orig_pc[64];
    int8_t  loop_target[64];
    int     n_active;
} CompiledPlan;

static void build_compiled_plan_ko(const uint8_t expressed[64], uint64_t splice_mask,
                                   int nop_orig_pc, CompiledPlan* plan) {
    plan->n_active = 0;
    for (int pc = 0; pc < 64; pc++) {
        if ((splice_mask >> pc) & 1ULL) {
            int idx = plan->n_active++;
            plan->codons[idx]  = expressed[pc];
            plan->orig_pc[idx] = (uint8_t)pc;
            plan->loop_target[idx] = 0;
        }
    }
    /* Match LOOP_OPEN / LOOP_CLOSE_NZ deterministically (default anchor = 0 for circular plasmids) */
    int8_t stack[64];
    int sp = 0;
    for (int i = 0; i < plan->n_active; i++) {
        if ((int)plan->orig_pc[i] == nop_orig_pc) continue;
        uint8_t op = (uint8_t)(plan->codons[i] & 0x1Fu);
        if (op == OP_LOOP_OPEN) {
            if (sp < 64) stack[sp++] = (int8_t)i;
        } else if (op == OP_LOOP_CLOSE_NZ) {
            if (sp > 0) {
                plan->loop_target[i] = stack[--sp];
            } else {
                plan->loop_target[i] = 0; /* Circular plasmid wrap-around! */
            }
        }
    }
}

static void build_compiled_plan(const uint8_t expressed[64], uint64_t splice_mask, CompiledPlan* plan) {
    build_compiled_plan_ko(expressed, splice_mask, -1, plan);
}

/* ============================================================================
 * JIT COMPILATION ENGINE (EMITS REAL NATIVE x86-64 SILICON BYTES)
 * ============================================================================ */

/* Portable C-helper trampoline used ONLY when CPU lacks GFNI/PCLMUL/BMI2 silicon extensions */
static void emit_call_helper3(JitBuf* b, uintptr_t fn_addr) {
    /* Inputs: RAX=arg0, RCX=arg1, RDX=arg2. Output: RAX.
     * Prologue pushed 8 regs (+8B retaddr = 72B). 7 pushes here = 128B (exact 16B alignment!). */
    eb(b, 0x56);               /* push rsi */
    eb(b, 0x57);               /* push rdi */
    eb(b, 0x41); eb(b, 0x50);  /* push r8  */
    eb(b, 0x41); eb(b, 0x51);  /* push r9  */
    eb(b, 0x41); eb(b, 0x52);  /* push r10 */
    eb(b, 0x41); eb(b, 0x53);  /* push r11 */
    eb(b, 0x53);               /* push rbx */
#if defined(_WIN32)
    eb(b, 0x48); eb(b, 0x83); eb(b, 0xEC); eb(b, 0x20); /* sub rsp, 32 (shadow space) */
    emit_mov_rr(b, X_R8,  X_RDX);
    emit_mov_rr(b, X_RDX, X_RCX);
    emit_mov_rr(b, X_RCX, X_RAX);
#else
    emit_mov_rr(b, X_RDI, X_RAX);
    emit_mov_rr(b, X_RSI, X_RCX);
#endif
    eb(b, 0x48); eb(b, 0xB8); e64(b, (uint64_t)fn_addr); /* movabs rax, fn_addr */
    eb(b, 0xFF); eb(b, 0xD0);                            /* call rax */
#if defined(_WIN32)
    eb(b, 0x48); eb(b, 0x83); eb(b, 0xC4); eb(b, 0x20); /* add rsp, 32 */
#endif
    eb(b, 0x5B);               /* pop rbx */
    eb(b, 0x41); eb(b, 0x5B);  /* pop r11 */
    eb(b, 0x41); eb(b, 0x5A);  /* pop r10 */
    eb(b, 0x41); eb(b, 0x59);  /* pop r9  */
    eb(b, 0x41); eb(b, 0x58);  /* pop r8  */
    eb(b, 0x5F);               /* pop rdi */
    eb(b, 0x5E);               /* pop rsi */
}

static uint32_t jit_emit_program(uint8_t* dst, uint32_t cap,
                                 const uint8_t expressed[64], uint64_t splice_mask,
                                 uint8_t nerf_pct,
                                 int use_gfni, int use_pclmul, int use_bmi2) {
    CompiledPlan plan;
    build_compiled_plan(expressed, splice_mask, &plan);

    JitBuf b;
    b.ptr = dst;
    b.len = 0;
    b.cap = cap;

    /* --- PROLOGUE: Save callee-saved regs (8 pushes) --- */
    eb(&b, 0x53);                   /* push rbx */
    eb(&b, 0x55);                   /* push rbp */
    eb(&b, 0x56);                   /* push rsi */
    eb(&b, 0x57);                   /* push rdi */
    eb(&b, 0x41); eb(&b, 0x54);     /* push r12 */
    eb(&b, 0x41); eb(&b, 0x55);     /* push r13 */
    eb(&b, 0x41); eb(&b, 0x56);     /* push r14 */
    eb(&b, 0x41); eb(&b, 0x57);     /* push r15 */

    /* Move ABI arguments (tape_a, tape_b, ctx) -> (r12, r13, r14) */
#if defined(_WIN32)
    emit_mov_rr(&b, X_R12, X_RCX);
    emit_mov_rr(&b, X_R13, X_RDX);
    emit_mov_rr(&b, X_R14, X_R8);
#else
    emit_mov_rr(&b, X_R12, X_RDI);
    emit_mov_rr(&b, X_R13, X_RSI);
    emit_mov_rr(&b, X_R14, X_RDX);
#endif

    /* Load persistent somatic state from ctx (r14) */
    emit_load_ctx64(&b, ORG_REG[0], CTX_OFF_REG0);
    emit_load_ctx64(&b, ORG_REG[1], CTX_OFF_REG1);
    emit_load_ctx64(&b, ORG_REG[2], CTX_OFF_REG2);
    emit_load_ctx64(&b, ORG_REG[3], CTX_OFF_REG3);
    emit_load_ctx32(&b, X_R15, CTX_OFF_GAS);
    emit_load_ctx32(&b, X_R11, CTX_OFF_ERASED);
    emit_load_ctx32(&b, X_R10, CTX_OFF_REVOPS);
    emit_load_ctx32(&b, X_RBP, CTX_OFF_HEAD0);
    emit_and_r32_imm8(&b, X_RBP, 0x7Fu);
    emit_load_ctx32(&b, X_R9,  CTX_OFF_HEAD1);
    emit_and_r32_imm8(&b, X_R9,  0x7Fu);

    /* Track code offset of each active instruction for loops & conditional skips */
    uint32_t insn_off[65];
    uint32_t skip_patch_at[64];
    int      skip_target_idx[64];
    int      n_skips = 0;

    for (int i = 0; i < plan.n_active; i++) {
        insn_off[i] = b.len;
        if (b.len + 224u > b.cap) {
            /* Safety guard: cleanly terminate remaining slots if ever close to 4KiB page cap */
            for (int k = i; k <= plan.n_active; k++) insn_off[k] = b.len;
            break;
        }

        uint8_t c  = plan.codons[i];
        uint8_t op = (uint8_t)(c & 0x1Fu);
        uint8_t d  = (uint8_t)((c >> 5) & 3u);
        uint8_t b7 = (uint8_t)((c >> 7) & 1u);
        uint8_t s  = (uint8_t)((d + 1u + b7) & 3u);
        uint8_t t  = (uint8_t)((d + 2u + b7) & 3u);

        uint8_t rd = ORG_REG[d];
        uint8_t rs = ORG_REG[s];
        uint8_t rt = ORG_REG[t];

        switch (op) {
        /* === GROUP A: REVERSIBLE OPS (0 Landauer erasure heat!) === */
        case OP_REV_XOR_REG:
            emit_xor_rr(&b, rd, rs);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_REV_ROL_REG:
            emit_rol_r_imm8(&b, rd, b7 ? 13u : 7u);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_REV_SWAP_TAPE:
            emit_load_ta_u8_eax(&b);
            emit_load_tb_u8_edx(&b);
            emit_store_ta_u8_dl(&b);
            emit_store_tb_u8_al(&b);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_REV_TOFFOLI:
            /* R[d] ^= (R[s] & R[t]) */
            emit_mov_rr(&b, X_RAX, rs);
            emit_and_rr(&b, X_RAX, rt);
            emit_xor_rr(&b, rd, X_RAX);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_REV_FREDKIN:
            /* tmp = (R[s] ^ R[t]) & R[d]; R[s] ^= tmp; R[t] ^= tmp; */
            emit_mov_rr(&b, X_RAX, rs);
            emit_xor_rr(&b, X_RAX, rt);
            emit_and_rr(&b, X_RAX, rd);
            emit_xor_rr(&b, rs, X_RAX);
            emit_xor_rr(&b, rt, X_RAX);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_REV_ADD_SUB:
            if (!b7) emit_add_rr(&b, rd, rs);
            else     emit_sub_rr(&b, rd, rs);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_REV_HEAD_STEP:
            if (!b7) {
                emit_inc_r64(&b, X_RBP);
                emit_and_r32_imm8(&b, X_RBP, 0x7Fu);
                emit_inc_r64(&b, X_R9);
                emit_and_r32_imm8(&b, X_R9, 0x7Fu);
            } else {
                emit_dec_r64(&b, X_RBP);
                emit_and_r32_imm8(&b, X_RBP, 0x7Fu);
                emit_inc_r64(&b, X_R9);
                emit_and_r32_imm8(&b, X_R9, 0x7Fu);
            }
            emit_inc_r64(&b, X_R10);
            break;

        case OP_REV_GFNI_FOLD:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                uint8_t imm_bias = (uint8_t)(plan.orig_pc[i] ^ (b7 ? 0x5Au : 0x00u));
                if (use_gfni) {
                    /* Save old rd in rdx; run native GF2P8AFFINEQB into rax -> rd */
                    emit_mov_rr(&b, X_RAX, rd);
                    emit_mov_rr(&b, X_RDX, rd);
                    eb(&b, 0x66); eb(&b, 0x48); eb(&b, 0x0F); eb(&b, 0x6E); eb(&b, 0xC0); /* movq xmm0, rax */
                    eb(&b, 0xF3); eb(&b, 0x41); eb(&b, 0x0F); eb(&b, 0x7E); eb(&b, 0x4E); eb(&b, (uint8_t)CTX_OFF_RIBOMAT); /* movq xmm1, [r14+0x28] */
                    eb(&b, 0x66); eb(&b, 0x0F); eb(&b, 0x3A); eb(&b, 0xCE); eb(&b, 0xC1); eb(&b, imm_bias); /* gf2p8affineqb xmm0, xmm1, imm */
                    eb(&b, 0x66); eb(&b, 0x48); eb(&b, 0x0F); eb(&b, 0x7E); eb(&b, 0xC0); /* movq rax, xmm0 */
                    emit_mov_rr(&b, rd, X_RAX);
                } else {
                    emit_mov_rr(&b, X_RAX, rd);
                    emit_load_ctx64(&b, X_RCX, CTX_OFF_RIBOMAT);
                    eb(&b, 0xBA); e32(&b, (uint32_t)imm_bias); /* mov edx, imm_bias */
                    emit_call_helper3(&b, (uintptr_t)&scalar_gf2p8affine_u64);
                    emit_mov_rr(&b, X_RDX, rd);
                    emit_mov_rr(&b, rd, X_RAX);
                }
                /* Landauer check: reversible iff ctx->ribosome_rank == 8; else charge popcnt(old^new)! */
                emit_load_ctx32(&b, X_RCX, CTX_OFF_RIBO_RANK);
                eb(&b, 0x83); eb(&b, 0xF9); eb(&b, 0x08); /* cmp ecx, 8 */
                eb(&b, 0x74); /* je +rel8 -> rev_branch */
                uint32_t je_patch = b.len;
                eb(&b, 0);
                /* Singular ribosome (rank < 8): irreversible bit erasure + irr_ops++ */
                emit_xor_rr(&b, X_RAX, X_RDX);
                emit_popcnt_rax_add_r11(&b);
                emit_inc_ctx32(&b, CTX_OFF_IRROPS);
                eb(&b, 0xEB); /* jmp +rel8 -> end */
                uint32_t jmp_patch = b.len;
                eb(&b, 0);
                /* Full-rank ribosome (rank == 8): strictly reversible rev_ops++ */
                if (je_patch < b.cap) b.ptr[je_patch] = (uint8_t)(b.len - (je_patch + 1u));
                emit_inc_r64(&b, X_R10);
                if (jmp_patch < b.cap) b.ptr[jmp_patch] = (uint8_t)(b.len - (jmp_patch + 1u));
            }
            /* When nerfed: true NOP — do NOT credit fake rev_ops++! */
            break;

        /* === GROUP B: IRREVERSIBLE DISSIPATIVE OPS (Exact POPCNT Landauer Erasure!) === */
        case OP_IRR_COPY_A_TO_B:
            emit_load_ta_u8_eax(&b);     /* eax = val = tape_a[head0] */
            emit_load_tb_u8_edx(&b);     /* edx = old = tape_b[head1] */
            emit_store_tb_u8_al(&b);     /* tape_b[head1] = al */
            emit_xor_rr(&b, X_RAX, X_RDX);
            emit_popcnt_rax_add_r11(&b); /* r11 += popcnt(old ^ val) */
            emit_inc_r64(&b, X_RBP);
            emit_and_r32_imm8(&b, X_RBP, 0x7Fu);
            emit_inc_r64(&b, X_R9);
            emit_and_r32_imm8(&b, X_R9, 0x7Fu);
            emit_inc_ctx32(&b, CTX_OFF_COPY_A2B);
            emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            break;

        case OP_IRR_COPY_B_TO_A:
            emit_load_ta_u8_eax(&b);     /* eax = old = tape_a[head0] */
            emit_load_tb_u8_edx(&b);     /* edx = val = tape_b[head1] */
            emit_store_ta_u8_dl(&b);     /* tape_a[head0] = dl */
            emit_xor_rr(&b, X_RAX, X_RDX);
            emit_popcnt_rax_add_r11(&b); /* r11 += popcnt(old ^ val) */
            emit_inc_r64(&b, X_RBP);
            emit_and_r32_imm8(&b, X_RBP, 0x7Fu);
            emit_inc_r64(&b, X_R9);
            emit_and_r32_imm8(&b, X_R9, 0x7Fu);
            emit_inc_ctx32(&b, CTX_OFF_COPY_B2A);
            emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            break;

        case OP_IRR_CLMUL_SYNTH:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                if (use_pclmul) {
                    emit_mov_rr(&b, X_RAX, rd);
                    emit_mov_rr(&b, X_RDX, rs);
                    eb(&b, 0x66); eb(&b, 0x48); eb(&b, 0x0F); eb(&b, 0x6E); eb(&b, 0xC0); /* movq xmm0, rax */
                    eb(&b, 0x66); eb(&b, 0x48); eb(&b, 0x0F); eb(&b, 0x6E); eb(&b, 0xCA); /* movq xmm1, rdx */
                    eb(&b, 0x66); eb(&b, 0x0F); eb(&b, 0x3A); eb(&b, 0x44); eb(&b, 0xC1); eb(&b, 0x00); /* pclmulqdq xmm0, xmm1, 0 */
                    eb(&b, 0x66); eb(&b, 0x48); eb(&b, 0x0F); eb(&b, 0x7E); eb(&b, 0xC0); /* movq rax, xmm0 */
                } else {
                    emit_mov_rr(&b, X_RAX, rd);
                    emit_mov_rr(&b, X_RCX, rs);
                    emit_call_helper3(&b, (uintptr_t)&scalar_pclmul64_lo);
                }
                emit_mov_rr(&b, X_RDX, rd);  /* save old */
                emit_mov_rr(&b, rd, X_RAX);  /* R[d] = res */
                emit_xor_rr(&b, X_RAX, X_RDX);
                emit_popcnt_rax_add_r11(&b);
                emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            }
            /* When nerfed: true NOP (no fake rev_ops++) */
            break;

        case OP_IRR_PEXT_SPLICE:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                if (use_bmi2) {
                    emit_mov_rr(&b, X_RDX, rs);
                    emit_load_ctx64(&b, X_RCX, CTX_OFF_SPLICE);
                    /* pext rax, rdx, rcx */
                    eb(&b, 0xC4); eb(&b, 0xE2); eb(&b, 0xEA); eb(&b, 0xF5); eb(&b, 0xC1);
                } else {
                    emit_mov_rr(&b, X_RAX, rs);
                    emit_load_ctx64(&b, X_RCX, CTX_OFF_SPLICE);
                    emit_call_helper3(&b, (uintptr_t)&scalar_pext64);
                }
                emit_mov_rr(&b, X_RDX, rd);
                emit_mov_rr(&b, rd, X_RAX);
                emit_xor_rr(&b, X_RAX, X_RDX);
                emit_popcnt_rax_add_r11(&b);
                emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            }
            break;

        case OP_IRR_PDEP_SCATTER:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                if (use_bmi2) {
                    emit_mov_rr(&b, X_RDX, rs);
                    emit_load_ctx64(&b, X_RCX, CTX_OFF_SPLICE);
                    /* pdep rax, rdx, rcx */
                    eb(&b, 0xC4); eb(&b, 0xE2); eb(&b, 0xEB); eb(&b, 0xF5); eb(&b, 0xC1);
                } else {
                    emit_mov_rr(&b, X_RAX, rs);
                    emit_load_ctx64(&b, X_RCX, CTX_OFF_SPLICE);
                    emit_call_helper3(&b, (uintptr_t)&scalar_pdep64);
                }
                emit_mov_rr(&b, X_RDX, rd);
                emit_mov_rr(&b, rd, X_RAX);
                emit_xor_rr(&b, X_RAX, X_RDX);
                emit_popcnt_rax_add_r11(&b);
                emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            }
            break;

        case OP_IRR_AND_MASK:
            emit_mov_rr(&b, X_RAX, rd); /* old */
            emit_and_rr(&b, rd, rs);    /* R[d] &= R[s] */
            emit_xor_rr(&b, X_RAX, rd); /* bits flipped/cleared */
            emit_popcnt_rax_add_r11(&b);
            emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            break;

        case OP_IRR_LOAD_TAPE:
            emit_mov_rr(&b, X_RAX, b7 ? X_R9 : X_RBP);
            emit_and_r32_imm8(&b, X_RAX, 0x78u); /* 8-byte aligned offset in [0..120] */
            emit_load_tape_u64_rcx(&b, b7);      /* rcx = new */
            emit_mov_rr(&b, X_RAX, rd);          /* rax = old */
            emit_mov_rr(&b, rd, X_RCX);          /* R[d] = new */
            emit_xor_rr(&b, X_RAX, X_RCX);
            emit_popcnt_rax_add_r11(&b);
            emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            break;

        case OP_IRR_STORE_TAPE:
            emit_mov_rr(&b, X_RAX, b7 ? X_R9 : X_RBP);
            emit_and_r32_imm8(&b, X_RAX, 0x78u);
            emit_load_tape_u64_rcx(&b, b7);      /* rcx = old */
            emit_mov_rr(&b, X_RDX, rd);          /* rdx = val */
            emit_store_tape_u64_rdx(&b, b7);     /* tape[off] = rdx */
            emit_mov_rr(&b, X_RAX, X_RCX);
            emit_xor_rr(&b, X_RAX, X_RDX);
            emit_popcnt_rax_add_r11(&b);
            emit_inc_ctx32(&b, CTX_OFF_IRROPS);
            break;

        /* === GROUP C: CONTROL FLOW & 2-ADIC KIN-SENSING === */
        case OP_LOOP_OPEN:
            emit_inc_r64(&b, X_R10);
            break;

        case OP_LOOP_CLOSE_NZ: {
            /* sub r15, 1 ; jle +skip ; test cond ; jnz target */
            eb(&b, 0x49); eb(&b, 0x83); eb(&b, 0xEF); eb(&b, 0x01); /* sub r15, 1 */
            eb(&b, 0x0F); eb(&b, 0x8E);                             /* jle rel32 */
            uint32_t jle_patch = b.len;
            e32(&b, 0);

            if (!b7) {
                emit_load_ta_u8_eax(&b);
                eb(&b, 0x84); eb(&b, 0xC0); /* test al, al */
            } else {
                emit_mov_rr(&b, X_RAX, rd);
                eb(&b, 0x84); eb(&b, 0xC0); /* test al, al */
            }
            eb(&b, 0x0F); eb(&b, 0x85);     /* jnz target */
            int32_t tgt_off = (int32_t)insn_off[plan.loop_target[i]];
            int32_t rel_back = tgt_off - (int32_t)(b.len + 4u);
            e32(&b, (uint32_t)rel_back);

            /* Patch jle target to right here */
            patch32(&b, jle_patch, (int32_t)(b.len - (jle_patch + 4u)));
            break;
        }

        case OP_BRANCH_EQ_SKIP:
            emit_inc_r64(&b, X_R10);
            if (i + 1 < plan.n_active) {
                if (!b7) {
                    emit_load_ta_u8_eax(&b);
                    emit_load_tb_u8_edx(&b);
                    eb(&b, 0x38); eb(&b, 0xD0); /* cmp al, dl */
                } else {
                    emit_cmp_rr(&b, rd, rs);
                }
                eb(&b, 0x0F); eb(&b, 0x84); /* je rel32 -> insn_off[i+2] */
                skip_patch_at[n_skips]   = b.len;
                skip_target_idx[n_skips] = (i + 2 < plan.n_active) ? (i + 2) : plan.n_active;
                n_skips++;
                e32(&b, 0);
            }
            break;

        case OP_BRANCH_TZCNT:
            emit_load_ctx64(&b, X_RAX, CTX_OFF_CLADEXOR);
            /* tzcnt rax, rax (returns 64 cleanly on 0) */
            eb(&b, 0xF3); eb(&b, 0x48); eb(&b, 0x0F); eb(&b, 0xBC); eb(&b, 0xC0);
            emit_xor_rr(&b, rd, X_RAX);
            emit_inc_r64(&b, X_R10);
            if (b7 && i + 1 < plan.n_active) {
                /* cmp rax, 8 ; jae skip_next (close 2-adic kin branch!) */
                eb(&b, 0x48); eb(&b, 0x83); eb(&b, 0xF8); eb(&b, 0x08);
                eb(&b, 0x0F); eb(&b, 0x83);
                skip_patch_at[n_skips]   = b.len;
                skip_target_idx[n_skips] = (i + 2 < plan.n_active) ? (i + 2) : plan.n_active;
                n_skips++;
                e32(&b, 0);
            }
            break;

        /* === GROUP D: BIOLOGICAL / META-GENETIC SILICON TRAPS === */
        case OP_METAB_HARVEST:
            /* Store internal prediction R[d] BEFORE reversible env_wave sensing (no double-XOR cancellation!) */
            emit_store_ctx64(&b, CTX_OFF_HARVEST_PARAM, rd);
            emit_load_ctx64(&b, X_RAX, CTX_OFF_ENVWAVE);
            emit_add_rr(&b, rd, X_RAX);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_METAB_HARVEST);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_METAB_RADIATE:
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_METAB_RADIATE);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_MEMBRANE_SEAL:
            emit_store_ctx64(&b, CTX_OFF_MEMBRANE_PARAM, rd);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_MEMBRANE_SEAL);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_SELF_HEAL_ECC:
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_SELF_HEAL_ECC);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_RETRO_TRANSCRIBE:
            emit_store_ctx64(&b, CTX_OFF_RETRO_PARAM, rd);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_RETRO_TRANSCRIBE);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_RIBOSOME_MUTATE:
            emit_store_ctx64(&b, CTX_OFF_RIBO_PARAM, rd);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_RIBOSOME_MUTATE);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_CENTROMERE_DRIFT:
            emit_store_ctx64(&b, CTX_OFF_CENTRO_PARAM, rd);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_CENTROMERE_DRIFT);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_HOLOBIONT_BOND:
            emit_store_ctx64(&b, CTX_OFF_HOLO_PARAM, rd);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_HOLOBIONT_BOND);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_STIGMERGY_EMIT:
            emit_store_ctx64(&b, CTX_OFF_STIG_OUT, rd);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_STIGMERGY_EMIT);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_STIGMERGY_SENSE:
            emit_load_ctx64(&b, X_RAX, CTX_OFF_STIG_IN);
            emit_xor_rr(&b, rd, X_RAX);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_MITOSIS_FORK:
            emit_store_ctx64(&b, CTX_OFF_MITOSIS_PARAM, rd);
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_MITOSIS_FORK);
            emit_inc_r64(&b, X_R10);
            break;

        case OP_SLEEP_SPORE:
            emit_or_ctx_bio(&b, (uint32_t)BIO_ACT_SLEEP_SPORE);
            emit_inc_r64(&b, X_R10);
            break;
        }
    }

    insn_off[plan.n_active] = b.len;

    /* Patch all forward conditional skips cleanly */
    for (int k = 0; k < n_skips; k++) {
        uint32_t pat = skip_patch_at[k];
        uint32_t tgt = insn_off[skip_target_idx[k]];
        patch32(&b, pat, (int32_t)(tgt - (pat + 4u)));
    }

    /* --- EPILOGUE: Store somatic state back into ctx (r14) --- */
    emit_store_ctx64(&b, CTX_OFF_REG0, ORG_REG[0]);
    emit_store_ctx64(&b, CTX_OFF_REG1, ORG_REG[1]);
    emit_store_ctx64(&b, CTX_OFF_REG2, ORG_REG[2]);
    emit_store_ctx64(&b, CTX_OFF_REG3, ORG_REG[3]);
    emit_store_ctx32(&b, CTX_OFF_GAS,    X_R15);
    emit_store_ctx32(&b, CTX_OFF_ERASED, X_R11);
    emit_store_ctx32(&b, CTX_OFF_REVOPS, X_R10);
    emit_store_ctx32(&b, CTX_OFF_HEAD0,  X_RBP);
    emit_store_ctx32(&b, CTX_OFF_HEAD1,  X_R9);

    /* Return value in RAX = rev_ops + erased_bits (quick summary) */
    emit_mov_rr(&b, X_RAX, X_R10);
    emit_add_rr(&b, X_RAX, X_R11);

    /* Restore callee-saved registers & RET */
    eb(&b, 0x41); eb(&b, 0x5F);     /* pop r15 */
    eb(&b, 0x41); eb(&b, 0x5E);     /* pop r14 */
    eb(&b, 0x41); eb(&b, 0x5D);     /* pop r13 */
    eb(&b, 0x41); eb(&b, 0x5C);     /* pop r12 */
    eb(&b, 0x5F);                   /* pop rdi */
    eb(&b, 0x5E);                   /* pop rsi */
    eb(&b, 0x5D);                   /* pop rbp */
    eb(&b, 0x5B);                   /* pop rbx */
    eb(&b, 0xC3);                   /* ret */

    return b.len;
}

uint32_t jit_emit_readonly(uint8_t* dst, uint32_t cap,
                           const uint8_t expressed[64], uint64_t splice_mask,
                           uint8_t nerf_pct) {
    return jit_emit_program(dst, cap, expressed, splice_mask, nerf_pct, 1, 1, 1);
}

/* ============================================================================
 * 100% BIT-EXACT PORTABLE SCALAR REFERENCE ORACLE
 * Must agree bit-for-bit on EVERY register, tape byte, erasure count & action!
 * ============================================================================ */

static uint64_t scalar_popcnt64(uint64_t x) {
    uint64_t c = 0;
    while (x) { x &= (x - 1ULL); c++; }
    return c;
}

static uint64_t scalar_tzcnt64(uint64_t x) {
    if (x == 0ULL) return 64ULL;
    uint64_t c = 0;
    while ((x & 1ULL) == 0ULL) { x >>= 1; c++; }
    return c;
}

static uint64_t scalar_pext64(uint64_t src, uint64_t mask) {
    uint64_t res = 0, bit = 1ULL;
    for (int i = 0; i < 64; i++) {
        if ((mask >> i) & 1ULL) {
            if ((src >> i) & 1ULL) res |= bit;
            bit <<= 1;
        }
    }
    return res;
}

static uint64_t scalar_pdep64(uint64_t src, uint64_t mask) {
    uint64_t res = 0, sbit = 0;
    for (int i = 0; i < 64; i++) {
        if ((mask >> i) & 1ULL) {
            if ((src >> sbit) & 1ULL) res |= (1ULL << i);
            sbit++;
        }
    }
    return res;
}

static uint64_t scalar_pclmul64_lo(uint64_t a, uint64_t b) {
    uint64_t lo = 0;
    for (int i = 0; i < 64; i++) {
        if ((b >> i) & 1ULL) lo ^= (a << i);
    }
    return lo;
}

static uint64_t scalar_gf2p8affine_u64(uint64_t x, uint64_t mat, uint8_t bias) {
    uint64_t out = 0;
    for (int b = 0; b < 8; b++) {
        uint8_t byte_in  = (uint8_t)((x >> (b * 8)) & 0xFFu);
        uint8_t byte_out = gf2_mat_apply_byte(mat, byte_in, bias);
        out |= ((uint64_t)byte_out) << (b * 8);
    }
    return out;
}

uint64_t jit_reference_oracle_with_nop(const uint8_t expressed[64], uint64_t splice_mask,
                                       uint8_t nerf_pct,
                                       uint8_t tape_a[128], uint8_t tape_b[128],
                                       AutophageExecCtx* ctx, int nop_orig_pc) {
    CompiledPlan plan;
    build_compiled_plan_ko(expressed, splice_mask, nop_orig_pc, &plan);

    uint64_t R[4] = { ctx->regs[0], ctx->regs[1], ctx->regs[2], ctx->regs[3] };
    int64_t  gas  = (int64_t)(uint64_t)ctx->gas;
    uint64_t erased = (uint64_t)ctx->erased_bits;
    uint64_t rev    = (uint64_t)ctx->rev_ops;
    uint32_t head0  = ctx->head0 & 0x7Fu;
    uint32_t head1  = ctx->head1 & 0x7Fu;

    int i = 0;
    while (i < plan.n_active) {
        if ((int)plan.orig_pc[i] == nop_orig_pc) {
            i++;
            continue;
        }
        uint8_t c  = plan.codons[i];
        uint8_t op = (uint8_t)(c & 0x1Fu);
        uint8_t d  = (uint8_t)((c >> 5) & 3u);
        uint8_t b7 = (uint8_t)((c >> 7) & 1u);
        uint8_t s  = (uint8_t)((d + 1u + b7) & 3u);
        uint8_t t  = (uint8_t)((d + 2u + b7) & 3u);

        switch (op) {
        case OP_REV_XOR_REG:
            R[d] ^= R[s];
            rev++;
            break;

        case OP_REV_ROL_REG:
            R[d] = hw_rol64(R[d], b7 ? 13u : 7u);
            rev++;
            break;

        case OP_REV_SWAP_TAPE: {
            uint8_t a = tape_a[head0];
            uint8_t b = tape_b[head1];
            tape_a[head0] = b;
            tape_b[head1] = a;
            rev++;
            break;
        }

        case OP_REV_TOFFOLI:
            R[d] ^= (R[s] & R[t]);
            rev++;
            break;

        case OP_REV_FREDKIN: {
            uint64_t tmp = (R[s] ^ R[t]) & R[d];
            R[s] ^= tmp;
            R[t] ^= tmp;
            rev++;
            break;
        }

        case OP_REV_ADD_SUB:
            if (!b7) R[d] += R[s];
            else     R[d] -= R[s];
            rev++;
            break;

        case OP_REV_HEAD_STEP:
            if (!b7) {
                head0 = (head0 + 1u) & 0x7Fu;
                head1 = (head1 + 1u) & 0x7Fu;
            } else {
                head0 = (head0 - 1u) & 0x7Fu;
                head1 = (head1 + 1u) & 0x7Fu;
            }
            rev++;
            break;

        case OP_REV_GFNI_FOLD:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                uint8_t imm_bias = (uint8_t)(plan.orig_pc[i] ^ (b7 ? 0x5Au : 0x00u));
                uint64_t old = R[d];
                uint64_t nw  = scalar_gf2p8affine_u64(old, ctx->ribosome_mat, imm_bias);
                R[d] = nw;
                if (ctx->ribosome_rank == 8u) {
                    rev++;
                } else {
                    erased += scalar_popcnt64(old ^ nw);
                    ctx->irr_ops++;
                }
            }
            break;

        case OP_IRR_COPY_A_TO_B: {
            uint8_t val = tape_a[head0];
            uint8_t old = tape_b[head1];
            tape_b[head1] = val;
            erased += scalar_popcnt64((uint64_t)(val ^ old));
            head0 = (head0 + 1u) & 0x7Fu;
            head1 = (head1 + 1u) & 0x7Fu;
            ctx->copy_a2b_count++;
            ctx->irr_ops++;
            break;
        }

        case OP_IRR_COPY_B_TO_A: {
            uint8_t old = tape_a[head0];
            uint8_t val = tape_b[head1];
            tape_a[head0] = val;
            erased += scalar_popcnt64((uint64_t)(old ^ val));
            head0 = (head0 + 1u) & 0x7Fu;
            head1 = (head1 + 1u) & 0x7Fu;
            ctx->copy_b2a_count++;
            ctx->irr_ops++;
            break;
        }

        case OP_IRR_CLMUL_SYNTH:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                uint64_t old = R[d];
                uint64_t res = scalar_pclmul64_lo(R[d], R[s]);
                R[d] = res;
                erased += scalar_popcnt64(old ^ res);
                ctx->irr_ops++;
            }
            break;

        case OP_IRR_PEXT_SPLICE:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                uint64_t old = R[d];
                uint64_t res = scalar_pext64(R[s], ctx->splice_mask);
                R[d] = res;
                erased += scalar_popcnt64(old ^ res);
                ctx->irr_ops++;
            }
            break;

        case OP_IRR_PDEP_SCATTER:
            if (hw_ext_allowed(plan.orig_pc[i], op, nerf_pct)) {
                uint64_t old = R[d];
                uint64_t res = scalar_pdep64(R[s], ctx->splice_mask);
                R[d] = res;
                erased += scalar_popcnt64(old ^ res);
                ctx->irr_ops++;
            }
            break;

        case OP_IRR_AND_MASK: {
            uint64_t old = R[d];
            R[d] &= R[s];
            erased += scalar_popcnt64(old ^ R[d]);
            ctx->irr_ops++;
            break;
        }

        case OP_IRR_LOAD_TAPE: {
            uint32_t off = (b7 ? head1 : head0) & 0x78u;
            uint64_t nw = 0;
            memcpy(&nw, (b7 ? tape_b : tape_a) + off, 8);
            uint64_t old = R[d];
            R[d] = nw;
            erased += scalar_popcnt64(old ^ nw);
            ctx->irr_ops++;
            break;
        }

        case OP_IRR_STORE_TAPE: {
            uint32_t off = (b7 ? head1 : head0) & 0x78u;
            uint64_t old = 0;
            memcpy(&old, (b7 ? tape_b : tape_a) + off, 8);
            uint64_t val = R[d];
            memcpy((b7 ? tape_b : tape_a) + off, &val, 8);
            erased += scalar_popcnt64(old ^ val);
            ctx->irr_ops++;
            break;
        }

        case OP_LOOP_OPEN:
            rev++;
            break;

        case OP_LOOP_CLOSE_NZ:
            gas -= 1;
            if (gas > 0) {
                int nz = b7 ? ((R[d] & 0xFFu) != 0u) : (tape_a[head0] != 0u);
                if (nz) {
                    i = plan.loop_target[i];
                    continue;
                }
            }
            break;

        case OP_BRANCH_EQ_SKIP:
            rev++;
            if (i + 1 < plan.n_active) {
                int eq = b7 ? (R[d] == R[s]) : (tape_a[head0] == tape_b[head1]);
                if (eq) {
                    i += 2;
                    continue;
                }
            }
            break;

        case OP_BRANCH_TZCNT: {
            uint64_t tz = scalar_tzcnt64(ctx->clade_xor);
            R[d] ^= tz;
            rev++;
            if (b7 && i + 1 < plan.n_active && tz >= 8ULL) {
                i += 2;
                continue;
            }
            break;
        }

        case OP_METAB_HARVEST:
            ctx->harvest_param = R[d];
            R[d] += ctx->env_wave;
            ctx->bio_actions |= BIO_ACT_METAB_HARVEST;
            rev++;
            break;

        case OP_METAB_RADIATE:
            ctx->bio_actions |= BIO_ACT_METAB_RADIATE;
            rev++;
            break;

        case OP_MEMBRANE_SEAL:
            ctx->membrane_param = R[d];
            ctx->bio_actions |= BIO_ACT_MEMBRANE_SEAL;
            rev++;
            break;

        case OP_SELF_HEAL_ECC:
            ctx->bio_actions |= BIO_ACT_SELF_HEAL_ECC;
            rev++;
            break;

        case OP_RETRO_TRANSCRIBE:
            ctx->retro_param = R[d];
            ctx->bio_actions |= BIO_ACT_RETRO_TRANSCRIBE;
            rev++;
            break;

        case OP_RIBOSOME_MUTATE:
            ctx->ribo_param = R[d];
            ctx->bio_actions |= BIO_ACT_RIBOSOME_MUTATE;
            rev++;
            break;

        case OP_CENTROMERE_DRIFT:
            ctx->centro_param = R[d];
            ctx->bio_actions |= BIO_ACT_CENTROMERE_DRIFT;
            rev++;
            break;

        case OP_HOLOBIONT_BOND:
            ctx->holo_param = R[d];
            ctx->bio_actions |= BIO_ACT_HOLOBIONT_BOND;
            rev++;
            break;

        case OP_STIGMERGY_EMIT:
            ctx->stigmergy_out = R[d];
            ctx->bio_actions |= BIO_ACT_STIGMERGY_EMIT;
            rev++;
            break;

        case OP_STIGMERGY_SENSE:
            R[d] ^= ctx->stigmergy_in;
            rev++;
            break;

        case OP_MITOSIS_FORK:
            ctx->mitosis_param = R[d];
            ctx->bio_actions |= BIO_ACT_MITOSIS_FORK;
            rev++;
            break;

        case OP_SLEEP_SPORE:
            ctx->bio_actions |= BIO_ACT_SLEEP_SPORE;
            rev++;
            break;
        }
        i++;
    }

    ctx->regs[0]     = R[0];
    ctx->regs[1]     = R[1];
    ctx->regs[2]     = R[2];
    ctx->regs[3]     = R[3];
    ctx->gas         = (uint32_t)gas;
    ctx->erased_bits = (uint32_t)erased;
    ctx->rev_ops     = (uint32_t)rev;
    ctx->head0       = head0;
    ctx->head1       = head1;
    return (rev + erased);
}

uint64_t jit_reference_oracle(const uint8_t expressed[64], uint64_t splice_mask,
                              uint8_t nerf_pct,
                              uint8_t tape_a[128], uint8_t tape_b[128],
                              AutophageExecCtx* ctx) {
    return jit_reference_oracle_with_nop(expressed, splice_mask, nerf_pct, tape_a, tape_b, ctx, -1);
}

/* ============================================================================
 * ZERO-ALLOCATION DIRECT-MAPPED RWX CODE CACHE (4096 SLOTS x 4 KiB = 16 MiB)
 * ============================================================================ */

int jit_cache_init(AutophageJitCache* cache, const AutophageHwCaps* caps) {
    memset(cache, 0, sizeof(*cache));
    cache->arena_size = AUTOPHAGE_JIT_ARENA_BYTES;
    cache->rwx_arena  = (uint8_t*)hw_alloc_rwx(cache->arena_size);
    if (!cache->rwx_arena) return 0;

    cache->use_gfni   = caps->has_gfni;
    cache->use_pclmul = caps->has_pclmulqdq;
    cache->use_bmi2   = caps->has_bmi2;

    for (uint32_t i = 0; i < AUTOPHAGE_JIT_CACHE_SLOTS; i++) {
        cache->entries[i].code_ptr = cache->rwx_arena + (size_t)i * AUTOPHAGE_JIT_MAX_BYTES;
    }
    return 1;
}

void jit_cache_destroy(AutophageJitCache* cache) {
    if (cache->rwx_arena) {
        hw_free_rwx(cache->rwx_arena, cache->arena_size);
        cache->rwx_arena = NULL;
    }
}

uint64_t jit_genome_key(const uint8_t expressed[64], uint64_t splice_mask, uint8_t nerf_pct) {
    uint64_t h = 0xCBF29CE484222325ULL ^ splice_mask ^ ((uint64_t)nerf_pct << 56) ^ (uint64_t)nerf_pct;
    for (int i = 0; i < 8; i++) {
        uint64_t w = 0;
        memcpy(&w, expressed + i * 8, 8);
        h ^= w;
        h *= 0x100000001B3ULL;
        h ^= (h >> 29);
    }
    if (h == 0ULL) h = 1ULL;
    return h;
}

autophage_jit_fn jit_compile_or_fetch(AutophageJitCache* cache,
                                      const uint8_t expressed[64],
                                      uint64_t splice_mask,
                                      uint8_t nerf_pct,
                                      uint32_t* out_code_len,
                                      uint8_t* out_code_copy,
                                      size_t copy_cap) {
    uint64_t key = jit_genome_key(expressed, splice_mask, nerf_pct);
    uint32_t slot = (uint32_t)((key ^ (key >> 24) ^ (key >> 40)) & (AUTOPHAGE_JIT_CACHE_SLOTS - 1u));
    AutophageJitEntry* e = &cache->entries[slot];

    if (e->key_hash == key && e->code_len > 0 &&
        e->splice_mask == splice_mask && e->nerf_pct == nerf_pct &&
        memcmp(e->expressed, expressed, 64u) == 0) {
        cache->hit_count++;
        e->use_count++;
    } else {
        uint32_t len = jit_emit_program(e->code_ptr, AUTOPHAGE_JIT_MAX_BYTES,
                                        expressed, splice_mask, nerf_pct,
                                        (int)cache->use_gfni, (int)cache->use_pclmul, (int)cache->use_bmi2);
        hw_flush_icache(e->code_ptr, len);
        e->key_hash    = key;
        e->splice_mask = splice_mask;
        memcpy(e->expressed, expressed, 64u);
        e->nerf_pct    = nerf_pct;
        e->code_len    = len;
        e->use_count   = 1u;
        cache->compile_count++;
    }

    if (out_code_len) *out_code_len = e->code_len;
    if (out_code_copy && copy_cap > 0) {
        size_t n = e->code_len < copy_cap ? (size_t)e->code_len : copy_cap;
        memcpy(out_code_copy, e->code_ptr, n);
    }
    return (autophage_jit_fn)(uintptr_t)e->code_ptr;
}

/* ============================================================================
 * HUMAN-READABLE x86-64 SILICON + BIO-OPCODE DISASSEMBLER
 * ============================================================================ */

static const char* const OPCODE_NAMES[32] = {
    "REV_XOR_REG",      "REV_ROL_REG",      "REV_SWAP_TAPE",    "REV_TOFFOLI",
    "REV_FREDKIN",      "REV_ADD_SUB",      "REV_HEAD_STEP",    "REV_GFNI_FOLD",
    "IRR_COPY_A_TO_B",  "IRR_COPY_B_TO_A",  "IRR_CLMUL_SYNTH",  "IRR_PEXT_SPLICE",
    "IRR_PDEP_SCATTER", "IRR_AND_MASK",     "IRR_LOAD_TAPE",    "IRR_STORE_TAPE",
    "LOOP_OPEN",        "LOOP_CLOSE_NZ",    "BRANCH_EQ_SKIP",   "BRANCH_TZCNT",
    "METAB_HARVEST",    "METAB_RADIATE",    "MEMBRANE_SEAL",    "SELF_HEAL_ECC",
    "RETRO_TRANSCRIBE", "RIBOSOME_MUTATE",  "CENTROMERE_DRIFT", "HOLOBIONT_BOND",
    "STIGMERGY_EMIT",   "STIGMERGY_SENSE",  "MITOSIS_FORK",     "SLEEP_SPORE"
};

static const char* const REG_NAMES[4] = { "rbx", "rsi", "rdi", "r8" };

const char* opcode_mnemonic(uint8_t op5) {
    return OPCODE_NAMES[op5 & 0x1Fu];
}

size_t jit_disassemble_expressed(const uint8_t expressed[64], uint64_t splice_mask,
                                 const uint8_t* machine_code, uint32_t code_len,
                                 uint32_t total_code_len,
                                 char* buf, size_t cap) {
    if (!buf || cap == 0) return 0;
    CompiledPlan plan;
    build_compiled_plan(expressed, splice_mask, &plan);

    size_t pos = 0;
    uint32_t hdr_len = total_code_len >= code_len ? total_code_len : code_len;
    int n = snprintf(buf + pos, cap - pos,
                     "; AUTOPHAGE v1.5.0 Live Native x86-64 Genome Disassembly (%d active exons / 64, %u JIT bytes)\n"
                     "; Prologue: save rbx..r15, map r12=tape_a, r13=tape_b, r14=ctx, rbp=h0&7F, r9=h1&7F, r11=popcnt\n",
                     plan.n_active, (unsigned)hdr_len);
    if (n < 0) { buf[0] = '\0'; return 0; }
    if ((size_t)n >= cap - pos) { buf[cap - 1u] = '\0'; return cap - 1u; }
    pos += (size_t)n;

    for (int i = 0; i < plan.n_active && pos + 4u < cap; i++) {
        uint8_t c  = plan.codons[i];
        uint8_t op = (uint8_t)(c & 0x1Fu);
        uint8_t d  = (uint8_t)((c >> 5) & 3u);
        uint8_t b7 = (uint8_t)((c >> 7) & 1u);
        uint8_t s  = (uint8_t)((d + 1u + b7) & 3u);
        uint8_t t  = (uint8_t)((d + 2u + b7) & 3u);

        char detail[96];
        switch (op) {
        case OP_REV_XOR_REG:      snprintf(detail, sizeof(detail), "xor %s, %s", REG_NAMES[d], REG_NAMES[s]); break;
        case OP_REV_ROL_REG:      snprintf(detail, sizeof(detail), "rol %s, %u", REG_NAMES[d], b7 ? 13u : 7u); break;
        case OP_REV_SWAP_TAPE:    snprintf(detail, sizeof(detail), "xchg byte [r12+rbp], [r13+r9]"); break;
        case OP_REV_TOFFOLI:      snprintf(detail, sizeof(detail), "mov rax,%s; and rax,%s; xor %s,rax", REG_NAMES[s], REG_NAMES[t], REG_NAMES[d]); break;
        case OP_REV_FREDKIN:      snprintf(detail, sizeof(detail), "fredkin ctrl=%s swap=(%s,%s)", REG_NAMES[d], REG_NAMES[s], REG_NAMES[t]); break;
        case OP_REV_ADD_SUB:      snprintf(detail, sizeof(detail), "%s %s, %s", b7 ? "sub" : "add", REG_NAMES[d], REG_NAMES[s]); break;
        case OP_REV_HEAD_STEP:    snprintf(detail, sizeof(detail), "%s rbp; inc r9; and 0x7F", b7 ? "dec" : "inc"); break;
        case OP_REV_GFNI_FOLD:    snprintf(detail, sizeof(detail), "gf2p8affineqb %s, [r14+ribosome], 0x%02X (rank<8->popcnt)", REG_NAMES[d], (unsigned)(plan.orig_pc[i] ^ (b7 ? 0x5Au : 0))); break;
        case OP_IRR_COPY_A_TO_B:  snprintf(detail, sizeof(detail), "mov [r13+r9],[r12+rbp]; popcnt -> r11; h0++,h1++"); break;
        case OP_IRR_COPY_B_TO_A:  snprintf(detail, sizeof(detail), "mov [r12+rbp],[r13+r9]; popcnt -> r11; h0++,h1++"); break;
        case OP_IRR_CLMUL_SYNTH:  snprintf(detail, sizeof(detail), "pclmulqdq %s, %s, 0x00; popcnt -> r11", REG_NAMES[d], REG_NAMES[s]); break;
        case OP_IRR_PEXT_SPLICE:  snprintf(detail, sizeof(detail), "pext %s, %s, [r14+splice]; popcnt -> r11", REG_NAMES[d], REG_NAMES[s]); break;
        case OP_IRR_PDEP_SCATTER: snprintf(detail, sizeof(detail), "pdep %s, %s, [r14+splice]; popcnt -> r11", REG_NAMES[d], REG_NAMES[s]); break;
        case OP_IRR_AND_MASK:     snprintf(detail, sizeof(detail), "and %s, %s; popcnt(erased) -> r11", REG_NAMES[d], REG_NAMES[s]); break;
        case OP_IRR_LOAD_TAPE:    snprintf(detail, sizeof(detail), "mov %s, qword [%s+(h&0x78)]; popcnt->r11", REG_NAMES[d], b7 ? "r13" : "r12"); break;
        case OP_IRR_STORE_TAPE:   snprintf(detail, sizeof(detail), "mov qword [%s+(h&0x78)], %s; popcnt->r11", b7 ? "r13" : "r12", REG_NAMES[d]); break;
        case OP_LOOP_OPEN:        snprintf(detail, sizeof(detail), "L_%02d: ; loop anchor", i); break;
        case OP_LOOP_CLOSE_NZ:    snprintf(detail, sizeof(detail), "dec r15; jle +skip; test %s; jnz L_%02d", b7 ? REG_NAMES[d] : "[r12+rbp]", (int)plan.loop_target[i]); break;
        case OP_BRANCH_EQ_SKIP:   snprintf(detail, sizeof(detail), "cmp %s; je skip_next", b7 ? "regs" : "tapes"); break;
        case OP_BRANCH_TZCNT:     snprintf(detail, sizeof(detail), "tzcnt rax,[r14+clade_xor]; xor %s,rax%s", REG_NAMES[d], b7 ? "; jae(8) skip" : ""); break;
        case OP_METAB_HARVEST:    snprintf(detail, sizeof(detail), "mov [r14+harvest],%s; add %s,[r14+env]; bio|=HARVEST", REG_NAMES[d], REG_NAMES[d]); break;
        case OP_METAB_RADIATE:    snprintf(detail, sizeof(detail), "or [r14+bio], RADIATE"); break;
        case OP_MEMBRANE_SEAL:    snprintf(detail, sizeof(detail), "mov [r14+mem_p],%s; bio|=MEMBRANE_SEAL", REG_NAMES[d]); break;
        case OP_SELF_HEAL_ECC:    snprintf(detail, sizeof(detail), "or [r14+bio], SELF_HEAL_ECC"); break;
        case OP_RETRO_TRANSCRIBE: snprintf(detail, sizeof(detail), "mov [r14+retro_p],%s; bio|=RETRO_M_INV", REG_NAMES[d]); break;
        case OP_RIBOSOME_MUTATE:  snprintf(detail, sizeof(detail), "mov [r14+ribo_p],%s; bio|=RIBO_REWRITE", REG_NAMES[d]); break;
        case OP_CENTROMERE_DRIFT: snprintf(detail, sizeof(detail), "mov [r14+centro_p],%s; bio|=POLY_DRIFT", REG_NAMES[d]); break;
        case OP_HOLOBIONT_BOND:   snprintf(detail, sizeof(detail), "mov [r14+holo_p],%s; bio|=HOLOBIONT", REG_NAMES[d]); break;
        case OP_STIGMERGY_EMIT:   snprintf(detail, sizeof(detail), "mov [r14+stig_out],%s; bio|=EMIT", REG_NAMES[d]); break;
        case OP_STIGMERGY_SENSE:  snprintf(detail, sizeof(detail), "xor %s,[r14+stig_in]", REG_NAMES[d]); break;
        case OP_MITOSIS_FORK:     snprintf(detail, sizeof(detail), "mov [r14+mito_p],%s; bio|=MITOSIS_SEX", REG_NAMES[d]); break;
        case OP_SLEEP_SPORE:      snprintf(detail, sizeof(detail), "or [r14+bio], SLEEP_SPORE"); break;
        default:                  snprintf(detail, sizeof(detail), "nop"); break;
        }

        n = snprintf(buf + pos, cap - pos, "  [%02u] 0x%02X  %-17s | %s\n",
                     (unsigned)plan.orig_pc[i], (unsigned)c, OPCODE_NAMES[op], detail);
        if (n < 0) break;
        if ((size_t)n >= cap - pos) { pos = cap - 1u; break; }
        pos += (size_t)n;
    }

    if (machine_code && code_len > 0 && cap > pos && (cap - pos) >= 64u) {
        uint32_t show = code_len < 64u ? code_len : 64u;
        n = snprintf(buf + pos, cap - pos, "; Raw x86-64 RWX machine bytes [0..%u of %u]: ",
                     (unsigned)show, (unsigned)hdr_len);
        if (n > 0 && (size_t)n < cap - pos) {
            pos += (size_t)n;
            for (uint32_t k = 0; k < show && cap > pos && (cap - pos) >= 4u; k++) {
                n = snprintf(buf + pos, cap - pos, "%02X ", (unsigned)machine_code[k]);
                if (n < 0 || (size_t)n >= cap - pos) { pos = cap - 1u; break; }
                pos += (size_t)n;
            }
            if (pos + 2u < cap) buf[pos++] = '\n';
        }
    }
    buf[pos < cap ? pos : cap - 1u] = '\0';
    return pos;
}
