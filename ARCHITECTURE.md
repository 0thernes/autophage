# AUTOPHAGE v1.0 — Bare-Metal Architecture & Silicon Specification

## 1. Exact Cache-Line Memory Layouts

### 1.1 `AutophageCell` (`256 bytes`, 64-byte aligned, `_Static_assert == 256`)

| Cache Line | Byte Range | Fields & Hardware Purpose |
|---|---|---|
| **Line 0 (Nuclear Header)** | `0x00 .. 0x3F` | `ribosome_mat` (`u64`, fed directly to `VGF2P8AFFINEQB`), `ribosome_bias` (`u8`), `centromere_poly` (`u8`, $\mathbb{F}_2[x]$ modulus), `flags` (`u16`, heritable morphogenetic & kill-switch loci), `ultrametric_addr` (`u32`, 2-adic clade address for `TZCNT`), `splice_mask` (`u64`, hardware `PEXT`/`PDEP` exon mask), `membrane_key` (`u64`, `PCLMULQDQ` Markov blanket tag), `energy_uq` / `heat_uq` (`i32`), `age_ticks`, `lineage_id`, `parent_id`, `holobiont_root`, `generation`, `mutations_survived`, `rev_ratio_q16`, `somatic_writes`. |
| **Line 1 (Strand $\alpha$)** | `0x40 .. 0x7F` | 64-byte germline machine genome. Translated in 2 AVX2 instructions (`VGF2P8AFFINEQB` + `VPXOR`) through `(ribosome_mat, ribosome_bias)` into expressed codons. |
| **Line 2 (Strand $\beta$)** | `0x80 .. 0xBF` | 64-byte somatic scratchpad & holographic Planarian Cauchy ECC parity over $\mathbb{F}_{2^8}$. Source for `OP_RETRO_TRANSCRIBE` ($M^{-1}$) Weismann barrier shattering. |
| **Line 3 (Somatic & Metrology)** | `0xC0 .. 0xFF` | Persistent somatic registers `regs[0..3]` (`RBX, RSI, RDI, R8`), `last_rdtsc_cycles` (excluded from canonical SHA-256), `last_erased_bits`, `last_rev_ops`, `causal_active_bytes`, `assembly_index`, `offspring_count`, `mate_count`, `mate_refusals`, `hgt_exchanges`. |

### 1.2 `AutophageExecCtx` (`128 bytes`, 64-byte aligned, `_Static_assert == 128`)

Every offset lies within `[0x00 .. 0x7C]`, guaranteeing single-byte `disp8` `[r14 + off]` addressing across the entire JIT backend:
- `0x00..0x1F`: `regs[0..3]`
- `0x20`: `splice_mask` (`u64`)
- `0x28`: `ribosome_mat` (`u64`)
- `0x30`: `env_wave` (`u64`, `WORLD_EXOG` Ashby variety flux)
- `0x38`: `clade_xor` (`u64`, `A.ultrametric_addr ^ B.ultrametric_addr`)
- `0x40..0x4F`: `stigmergy_in` / `stigmergy_out`
- `0x50..0x5F`: `bio_actions` bitmask / `bio_param`
- `0x60..0x7F`: `gas`, `erased_bits`, `rev_ops`, `irr_ops`, `copy_a2b_count`, `copy_b2a_count`, `head0`, `head1` (`u32` each).

---

## 2. Polymorphic Native x86-64 RWX JIT Compiler (`src/jit_x86_64.c`)

### 2.1 Dual-ABI Register Architecture (Win64 PE & Linux SysV AMD64)
The prologue pushes `rbx, rbp, rsi, rdi, r12, r13, r14, r15` and normalizes incoming ABI arguments (`rcx, rdx, r8` on Win64; `rdi, rsi, rdx` on SysV AMD64) into identical body registers:

| Hardware Reg | Role Inside Native Organism Body |
|---|---|
| `R12` / `R13` | `tape_a[128]` / `tape_b[128]` (read/write organism & partner genomes; `[r13 + idx + 0]` uses `mod=01 disp8=0` SIB encoding) |
| `R14` | `AutophageExecCtx*` |
| `R15` | Hardware gas countdown (`sub r15, 1; jle +skip` on every backward jump) |
| `RBP` / `R9` | `head0` / `head1` — invariant `and reg32, 0x7F` after every update (provably `[0..127]`, Lean `T26`) |
| `R10` | Reversible operations counter (`rev_ops`) |
| `R11` | Hardware `POPCNT` Landauer erased-bit accumulator (`erased_bits`) |
| `RBX, RSI, RDI, R8` | Organism persistent somatic registers `R[0..3]` |
| `RAX, RCX, RDX` | Scratch GPRs |
| `XMM0, XMM1` | Scratch 128-bit registers for `GF2P8AFFINEQB` and `PCLMULQDQ` (caller-saved on **both** Win64 and SysV AMD64) |

Zero external `call` instructions are emitted inside the JIT body; biological traps write bitflags directly into `[r14 + 0x50]` (`or dword [r14+0x50], imm32`).

### 2.2 Total 32-Class Polymorphic Bare-Metal Opcode Table
Each active codon byte `c` (selected where `(splice_mask >> pc) & 1 == 1`) decodes totally with no invalid opcodes:
- `op = c & 0x1F` (32 classes), `d = (c >> 5) & 3`, `b7 = (c >> 7) & 1`, `s = (d + 1 + b7) & 3`, `t = (d + 2 + b7) & 3` (guarantees `{d, s, t}` are **3 pairwise distinct registers** so Toffoli & Fredkin are always exact involutions!).

| Range | Group | Silicon Instructions & Thermodynamics |
|---|---|---|
| `0x00..0x07` | **A: Reversible** | `XOR`, `ROL(7/13)`, `SWAP_TAPE`, `TOFFOLI`, `FREDKIN`, `ADD/SUB`, `HEAD_STEP(&0x7F)`, `GF2P8AFFINEQB` — **0 Landauer heat (`r11` untouched)** |
| `0x08..0x0F` | **B: Irreversible** | `COPY_A_TO_B` (auto-advancing polymerase), `COPY_B_TO_A`, `PCLMULQDQ`, `PEXT`, `PDEP`, `AND`, `LOAD_QWORD(&0x78)`, `STORE_QWORD(&0x78)` — **exact `popcnt(old ^ new)` added to `r11`** |
| `0x10..0x13` | **C: Control & 2-Adic** | `LOOP_OPEN`, `LOOP_CLOSE_NZ` (gas-bounded circular plasmid anchor), `BRANCH_EQ_SKIP`, `BRANCH_TZCNT` (2-adic ultrametric kin branch at `tzcnt >= 8`) |
| `0x14..0x1F` | **D: Bio / Meta Traps** | `METAB_HARVEST`, `METAB_RADIATE`, `MEMBRANE_SEAL`, `SELF_HEAL_ECC`, `RETRO_TRANSCRIBE` ($M^{-1}$), `RIBOSOME_MUTATE`, `CENTROMERE_DRIFT`, `HOLOBIONT_BOND`, `STIGMERGY_EMIT`, `STIGMERGY_SENSE`, `MITOSIS_FORK`, `SLEEP_SPORE` |

---

## 3. Galois Algebra & Planarian Cauchy ECC (`src/galois_math.c`)

- **Intel `VGF2P8AFFINEQB` Bit Convention:** Row `i` (`0..7`) is stored in byte `7 - i` of `uint64_t mat`, giving `y.bit[i] = parity(row[i] & x) ^ bias.bit[i]`. The identity matrix is `0x0102040810204080ULL`.
- **Weismann Inverse:** When `rank(M) == 8`, Gaussian elimination over $\mathbb{F}_2$ yields exact $M^{-1}$ and $c^{-1} = M^{-1} c$, satisfying $M^{-1}(Mx \oplus c) \oplus c^{-1} = x$ for all $x, c \in \mathbb{F}_2^8$.
- **Degree-8 Irreducibility:** Exact trial division over $\mathbb{F}_2[x]$ partitions all 256 monic degree-8 polynomials into **30 irreducible finite fields** $\mathbb{F}_{2^8}$ and **226 reducible zero-divisor rings**.
- **Holographic Planarian Cauchy ECC:** For each index $i \in [0..31]$, Strand $\beta$ stores:
  $$P_0[i] = \alpha[i] \oplus 3 \cdot \alpha[i+32], \qquad P_1[i] = 5 \cdot \alpha[i] \oplus \alpha[i+32]$$
  over $\mathbb{F}_{2^8}$. The $2\times 2$ determinant $1 \oplus (3 \cdot 5) = \text{0x0E} \neq 0$ across all 30 irreducible fields, enabling exact syndrome repair under 25%, 50%, and 75% excision of Strand $\alpha$.

---

## 4. Thermodynamic Conservation, Domain-Isolated PRNG & Observer Independence

- **Conserved Energy Ledger (`P2`):** Every energy mutation passes through closed-sum integer micro-quantum (`1 uq = 1e-6 kJ`) transfer primitives across 5 pools (`E_cell_free`, `E_membrane_pot`, `E_detritus`, `Q_heat_local`, `Q_heat_radiated_sink`) plus compensated Kahan-Neumaier FP64 accumulators. Spatial heat diffusion uses symmetric pairwise edge fluxes summing identically to `0`.
- **Domain-Isolated `Philox4x32-10` (`P4`):** Keyed strictly by `(master_seed, domain, entity_id, epoch, purpose, draw_idx)` across four disjoint domains (`WORLD_EXOG = 1`, `ENTITY_INT = 2`, `SHADOW_NULL = 3`, `INIT_SOUP = 4`).
- **Observer Independence (`P9`) & Zero Post-Warmup Allocation (`P10`):** All telemetry, 8-pillar OVBS-52, 64-locus causal knockout, apex disassembly, 112-trait projection, grid export, and streaming SHA-256 take `const AutophageWorld*` and allocate `0` bytes during `autophage_step_epochs()`.
