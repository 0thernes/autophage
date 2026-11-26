# AUTOPHAGE v1.4.0 — Architecture & Silicon Specification

Canonical architectural reference (mirrors root [`ARCHITECTURE.md`](../ARCHITECTURE.md) with v1.4.0 Palomar and ABI `0x00010400` updates).

## 1. Exact Cache-Line Memory Layouts

### 1.1 `AutophageCell` (`256 bytes`, 64-byte aligned, `_Static_assert == 256`)

| Cache Line | Byte Range | Fields & Hardware Purpose |
|---|---|---|
| **Line 0 (Nuclear Header)** | `0x00 .. 0x3F` | `ribosome_mat` (`u64`, fed directly to `VGF2P8AFFINEQB`), `ribosome_bias` (`u8`), `centromere_poly` (`u8`, $\mathbb{F}_2[x]$ modulus), `flags` (`u16`, morphogenetic, kill-switch & `CELL_FLAG_TRANSPLANTED` loci), `ultrametric_addr` (`u32`, 2-adic clade address for `TZCNT`), `splice_mask` (`u64`, hardware `PEXT`/`PDEP` exon mask), `membrane_key` (`u64`, `PCLMULQDQ` Markov blanket tag), `energy_uq` / `heat_uq` (`i32`), `age_ticks`, `lineage_id`, `parent_id`, `holobiont_root`, `generation`, `mutations_survived`, `rev_ratio_q16`, `somatic_writes`. |
| **Line 1 (Strand $\alpha$)** | `0x40 .. 0x7F` | 64-byte germline machine genome. Translated in 2 AVX2 instructions (`VGF2P8AFFINEQB` + `VPXOR`) through `(ribosome_mat, ribosome_bias)` into expressed codons. |
| **Line 2 (Strand $\beta$)** | `0x80 .. 0xBF` | 64-byte somatic scratchpad & 2×2 Cauchy MDS parity over $\mathbb{F}_{2^8}$ (earned via `OP_SELF_HEAL_ECC`). Source for `OP_RETRO_TRANSCRIBE` ($M^{-1}$) Weismann barrier shattering. |
| **Line 3 (Somatic & Metrology)** | `0xC0 .. 0xFF` | Persistent somatic registers `regs[0..3]` (`RBX, RSI, RDI, R8`), `last_rdtsc_cycles` (`+0xE0`, excluded from canonical SHA-256), `last_erased_bits` (`+0xE8`), `last_rev_ops` (`+0xEC`), `membrane_pot_uq` (`+0xF0`, per-cell conserved membrane potential), `offspring_count` (`+0xF4`), `mate_count` (`+0xF8`), `mate_refusals` (`+0xFA`), `hgt_exchanges` (`+0xFC`). |

### 1.2 `AutophageExecCtx` (`192 bytes`, 64-byte aligned, `_Static_assert == 192`)

Dedicated per-action parameter slots eliminate cross-opcode `bio_param` clobbering (`offsetof` checked via `_Static_assert` in `src/jit_x86_64.c`):
- `0x00..0x1F`: `regs[0..3]` (`u64[4]`)
- `0x20..0x5F`: `splice_mask`, `ribosome_mat`, `env_wave`, `clade_xor`, `stigmergy_in`, `stigmergy_out`, `bio_actions`, `harvest_param` (`u64` each)
- `0x60..0x7F`: `gas`, `erased_bits`, `rev_ops`, `irr_ops`, `copy_a2b_count`, `copy_b2a_count`, `head0`, `head1` (`u32` each)
- `0x80..0xAF`: dedicated `u64` per-action slots (`membrane_param`, `retro_param`, `ribo_param`, `centro_param`, `holo_param`, `mitosis_param`)
- `0xB0`: `ribosome_rank` (`u32`, runtime Landauer gate for `OP_REV_GFNI_FOLD`: reversible iff `ribosome_rank == 8`)

## 2. Dual-ABI Polymorphic x86-64 RWX JIT (`src/jit_x86_64.c`)

- **Register Normalization:** Prologue saves `rbx, rbp, rsi, rdi, r12..r15` and maps Win64 (`rcx, rdx, r8`) or SysV AMD64 (`rdi, rsi, rdx`) arguments onto `r12=tape_a`, `r13=tape_b`, `r14=ctx`.
- **Hardware Safety Invariants:** Every byte tape access uses `and reg32, 0x7F`; every qword tape access uses `and reg32, 0x78`; every backward loop executes `sub r15, 1; jle +skip`. Lean proves the arithmetic of these guards (`< 128`, `T26`; `off + 8 <= 128`, `T27`, necessity in `T48`; `g - 1 < g`, `T28`). That the JIT emits them is tested (Gates `T04`, `T05`, `T09`, libFuzzer), not proved.
- **Bit-Exact Reference Oracle:** `jit_reference_oracle` executes the identical 32-opcode semantics in portable C11; `T04`, `T09`, `T11`, and `test/fuzz_jit_libfuzzer.c` verify 0-bit divergence.

## 3. Galois Algebra, Cauchy MDS ECC & 5-Pool Conservation

- **$\mathbb{F}_2^{8\times 8}$ Ribosome (`VGF2P8AFFINEQB`):** Row `i` stored at byte `7-i`, identity `0x0102040810204080ULL`, Gaussian inversion yielding $M^{-1}$ and $c^{-1} = M^{-1} c$ (`T01`–`T07`, `T43`).
- **30 Irreducible Degree-8 Fields & 2×2 Cauchy MDS (`src/galois_math.c`):** Exact partition of 256 monic degree-8 polynomials into 30 irreducible fields and 226 reducible zero-divisor rings (`T12`, `T12a`, `T14`, `T47`). Cauchy matrix `[[1, 3], [5, 1]]` has nonzero determinant `0x0E` in all 30 fields (`T15`, `T15a`, `T16`, `T44`), so in the Lean model both bytes of each pair are recovered from its two parity bytes; the C11 32-pair loop over the 64 Strand $\alpha$ bytes is checked by Gate `T15`.
- **5-Pool Conserved Thermodynamic Ledger (`src/soup_engine.c`):** `E_cell_free`, `E_membrane_pot`, `E_detritus`, `Q_heat_local`, `Q_heat_radiated_sink` updated exclusively through conserved `int64_t` micro-quantum (`1 uq = 1e-6 kJ`) transfers (`T29`, `T29a`, `T45`) paired with Kahan-Neumaier FP64 accumulators (`-ffp-contract=off`).
