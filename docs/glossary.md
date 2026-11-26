# Glossary

## Domain & Mathematical Terms

- **2-Adic Ultrametric Distance (`TZCNT`):** Clade distance determined by the number of matching least-significant bits `tzcnt32(addr_A ^ addr_B)`, satisfying the Strong Triangle Inequality (`T25a`).
- **2×2 Cauchy MDS ECC:** Erasure code over $\mathbb{F}_{2^8}$ using matrix `[[1, 3], [5, 1]]` (`det = 0x0E ≠ 0` in all 30 irreducible degree-8 fields, `T15`–`T16`), storing parity in Strand $\beta$ to recover excised Strand $\alpha$ bytes.
- **Centromere Polynomial:** Per-cell byte $p \in \{0,\dots,255\}$ defining the monic degree-8 modulus $x^8 + p(x) \in \mathbb{F}_2[x]$; exactly 30 yield finite fields $\mathbb{F}_{2^8}$ and 226 yield zero-divisor rings (`T12`, `T12a`, `T14`).
- **Dobzhansky–Muller Algebraic Barrier:** Reproductive incompatibility arising when reducible polynomial rings contain zero divisors (`0x03 * 0xFF = 0` mod `0x01`, `T14`, `T47`).
- **Evolvable Ribosome (`M`, `bias`):** Per-cell 64-bit matrix $M \in \mathbb{F}_2^{8\times 8}$ and 8-bit bias executed via Intel `VGF2P8AFFINEQB` translating Strand $\alpha$ bytes into expressed opcodes (`T01`–`T07`).
- **First-Law Micro-Quantum (`uq`):** Exact integer energy unit (`1 uq = 1e-6 kJ`) conserved across 5 pools (`E_cell_free`, `E_membrane_pot`, `E_detritus`, `Q_heat_local`, `Q_heat_radiated_sink`, `T29`, `T29a`, `T45`).
- **Iron Law Curve:** Empirical monotone decline of composite `OVI` as global 13-vector restraint intensity rises from `0%` → `25%` → `50%` → `100%`.
- **Landauer Erasure Counter (`POPCNT`):** Hardware `POPCNT(old ^ new)` accumulation into register `R11` on irreversible instructions and singular-rank GFNI folds (`0` bits erased on reversible gates, `T17`–`T22`).
- **OVBS-52 / OVI:** 8-pillar heuristic vitality composite index (`0..100`) gated by anti-gaming ($\Gamma$) and Spiegelman complexity ($\Theta$) factors (`indicator-only`, `T30`).
- **Palomar 6-Kernel Verification:** Independent multi-kernel verification of isolated `Challenge.lean` vs. `Solution.lean` across `lean4`, `leanchecker-paranoid`, `lean4lean`, `nanoda_bin`, `con-leche`, and `con-ron`.
- **Weismann Reverse-Transcription ($M^{-1}$):** Inverting somatic Strand $\beta$ discoveries back into germline Strand $\alpha$ via `gf2_mat_invert(M)` (`T05`, `T05a`, `T42`, `T43`).

## Build Standard v1.2.0 Finding Classes (Rule R7)

Every finding in issues and hardening logs is assigned **exactly one** of these 10 classes:

- **`bug`:** Code does something different from its spec or intent (a defect in program logic).
- **`error`:** Wrong number, claim, doc, config, version, build, or command.
- **`issue`:** Works and is not wrong, but is unclear, inconsistent, fragile, or hard to maintain.
- **`problem`:** External obstacle (network, account, toolchain, platform limitation) blocks progress.
- **`concern`:** Risk that has not caused harm yet (security, cost, legal, reliability).
- **`dilemma`:** Trade-off between valid options; the owner decides.
- **`conflict`:** Two requirements or rules contradict each other.
- **`gap`:** Something required or expected is missing; nothing present is wrong.
- **`hole`:** Missing check or safeguard, so a defect could pass unnoticed.
- **`blindspot`:** A user, platform, failure mode, or assumption nobody has examined yet.
