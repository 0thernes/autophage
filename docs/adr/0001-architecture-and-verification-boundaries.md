# ADR-0001: Bare-Metal Dual-Kernel x86-64 Architecture, Zero-Mathlib 6-Kernel Lean 4.35.0 Verification & Dual Open Licensing

- **Status:** Accepted
- **Date:** 2026-11-25
- **Author:** Alexander Stephen Donahue (Independent Researcher, Spokane, WA, USA)

## Context

Artificial-life simulators typically interpret high-level scripts with floating-point approximations, opaque RNG coupling, and unverified energy conservation, making it difficult to separate genuine computational dynamics from numerical drift or implementation bugs. Conversely, full machine-code compiler verification (e.g., CompCert-scale x86-64 backend proofs) exceeds the scope of an independent zero-dependency research codebase.

## Decision

1. **Dual-Kernel Architecture (`NATIVE_JIT` + `SCALAR_REF`):** Implement a single-pass zero-allocation x86-64 RWX JIT compiler (`src/jit_x86_64.c`) paired with a portable C11 reference interpreter (`jit_reference_oracle`) enforcing `0`-bit SHA-256 parity across Win64 PE and Linux ELF64 (`-ffp-contract=off`) and automatic CPUID scalar trampolines on CPUs without hardware GFNI/VPCLMULQDQ.
2. **Exact Integer First-Law Ledger (`1 uq = 1e-6 kJ`) + Kahan-Neumaier FP64:** Perform all conservation accounting on exact `int64_t` micro-quantum pools (`T29`, `T29a`, `T45`) so conservation holds to exact `0 uq` residual while Kahan-Neumaier FP64 tracks relative drift (`< 1e-12`).
3. **Zero-Mathlib 6-Kernel Lean 4.35.0 Palomar Formalization:** Formalize all 48 algebraic, Galois-field, Cauchy MDS, reversible-gate, 2-adic ultrametric, JIT mask and gas arithmetic, ledger, Philox, seed-firewall, OEE, Weismann, and NonVacuity theorems in pure Lean 4.35.0 (`0` Mathlib, `0` `bv_decide`, `0` `native_decide`, strict `Spec.lean` / `Challenge.lean` / `Solution.lean` isolation), checked locally on Windows by the reference kernel and three independent checkers and in Ubuntu Linux CI by `lake check --paranoid` and `lake comparator` under `bubblewrap`.
4. **Dual Open Licensing:** Release all code, Lean 4 proofs, scripts, and CI under **Apache-2.0** (`LICENSE`), and all white papers, PDFs, figures, and documentation under **CC-BY-4.0** (`LICENSE-DOCS`), under sole independent researcher authorship (`Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA`).

## Consequences

- **Positive:** Zero external runtime dependencies, instant reproducibility from clean clone, 0-bit cross-OS determinism, independent 6-kernel trust base, and crisp separation between machine-checked mathematical invariants and empirical evolutionary probes.
- **Tradeoffs:** Rapid mutation reduces JIT cache hit rate (`27.35%`), moderating end-to-end JIT speedup (`1.04×–1.30×`) relative to hot-loop cached execution (`15–18 M execs/s`); raw x86-64 machine-code encoding remains differentially and fuzz-verified rather than proved inside Lean; single-threaded canonical stepping (`threads = 1`) prioritizes exact cross-OS SHA-256 bit-identity over multi-core scaling.
