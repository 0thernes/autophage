# AUTOPHAGE v1.4.0 — Formal & Empirical Verification Overview

Canonical verification documentation (synchronized with root [`VERIFICATION.md`](../VERIFICATION.md) and [`EVIDENCE.md`](../EVIDENCE.md)).

## 1. Six-Kernel Lean 4.35.0 Palomar Verification (`48 / 48` Theorems)

- **Pure Definitions:** `lean/AutophageProofs/Spec.lean` (`0` proofs, `0` `sorry`).
- **Proof Library:** `lean/AutophageProofs.lean` (`48` `public theorem` declarations across Parts I–VII, `0` `sorry`, `0` `admit`, `0` `native_decide`, `0` `bv_decide`, `0` warnings under `autoImplicit = false`, `relaxedAutoImplicit = false`, `linter.unusedVariables = true`, `linter.deprecated = true`).
- **Isolated Challenge / Solution Contract:** `lean/Challenge.lean` (statement-only `sorry` specifications importing ONLY `AutophageProofs.Spec`) vs. `lean/Solution.lean` (`public import AutophageProofs`), manifest in `lean/comparator.json`, metadata in `lean/formalization.yaml`.
- **Axiom Allowlist:** Strictly `⊆ {propext, Classical.choice, Quot.sound}` (`results/lean4-audit.txt`), automatically parity-checked against `README.md` by `scripts/check_readme_axioms.py`.
- **Independent Kernel Replay (`results/lean4-multi-kernel.txt`):**
  1. `lean4` kernel (`lake build` + `VerifyAxioms.lean`): **PASS** (`48/48`)
  2. `leanchecker --fresh Solution`: **PASS**
  3. `leanchecker-paranoid --fresh Solution`: **PASS**
  4. `nanoda_bin` (Rust kernel, `unpermitted_axiom_hard_error: true`): **PASS** (`2,784` declarations checked, `0` typechecker errors)
  5. `con-leche --verified` (Rust verified kernel): **PASS** (`2,546` declarations accepted)
  6. `con-ron --verified` (Rust verified kernel): **PASS** (`2,546` declarations accepted); plus `lean4lean` + `lake check --paranoid` + `lake comparator --config comparator.json --paranoid` on Ubuntu Linux with `bubblewrap` in `.github/workflows/ci.yml`.

## 2. 18-Gate Bare-Metal Silicon Falsification Suite + 6 Planted Mutants

- **Win64 PE (`clang 23.1.0`, ThinLTO, `-ffp-contract=off`):** `18 / 18 PASSED`, `6 / 6` planted negative mutants caught (`M1` energy leak, `M2` Second-Law backflow, `M3a/3b` intron bloat + dead scratch, `M4` white noise, `M5` 1-bit observer write, `M6` coupled PRNG) + tampered `.apbin` rejected + negative-control allocation monitor trip verified (`T12`).
- **Linux ELF64 (`gcc -Wall -Wextra -Werror` + ASan/UBSan):** `18 / 18 PASSED`, `0` warnings, `0`-bit diff canonical SHA-256 (`4bd29f39e96270cfbc659e1970e1ab875c4c0e807d93e48ef05b45e373c20612`) matching Win64 JIT and Scalar.
- **Independent Property Verification:** `scripts/prop_check.py` independently verifies `30×32` Cauchy MDS fields + `500×` GFNI, Toffoli, Fredkin, and 2-adic `tzcnt32` laws (`PASS`).
