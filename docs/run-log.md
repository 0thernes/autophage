# AUTOPHAGE Verification & Release Run Log (`B40`)

Chronological audit trail of major build, verification, and hardening executions.

| Date (UTC) | Version | Host / Environment | Command / Milestone | Key Recorded Outcome |
|---|---|---|---|---|
| 2026-11-20 | `v1.0.0` | Win64 Intel Core Ultra 9 275HX | Initial C11 + x86-64 RWX JIT + 18-gate suite + Lean 4 release | Initial 18/18 gates PASS; 31 Lean theorems (with `bv_decide`). |
| 2026-11-23 | `v1.2.0` | Win64 Clang 23.1.0 + WSL2 GCC `-Werror` | Deep forensic remediation & elimination of all 19 `_native.bv_decide.*` SAT axioms | 38/38 Pure Lean 4 theorems; 18/18 gates + 6/6 mutants PASS; 4-way Win64/Linux JIT/Scalar SHA-256 parity `4bd29f39...3c20612`. |
| 2026-11-24 | `v1.3.0` | Win64 Clang 23.1.0 ThinLTO (`-ffp-contract=off`) | Added Pure Lean 4 theorems `T39`–`T42` + Arrow Lake AVX2/AVX-VNNI/GFNI + ThinLTO | 42/42 Pure Lean 4 theorems; 18/18 gates PASS; `0` warnings. |
| 2026-11-25 | `v1.4.0` | Win64 Intel Core Ultra 9 275HX · Clang 23.1.0 + lld · Lean 4.35.0 · Python 3.13 · Tectonic | Full Build Standard v1.2.0 + 6-Kernel Palomar upgrade (`Spec.lean`, `Challenge.lean`, `Solution.lean`, `T43`–`T48` → 48/48 theorems) + live regeneration of `results/01`–`07` + `prop_check.py` + `check_citations.py` (`17/17` live) + `build_paper.py` (`0` Overfull `\hbox`, clean-room arXiv pass) | 18/18 gates (`2.486s`) + 6/6 mutants PASS; 48/48 Pure Lean 4.35.0 theorems verified across `lean4`, `leanchecker`, `leanchecker-paranoid`, `nanoda_bin` (2,784 decls), `con-leche` (2,546 decls), `con-ron` (2,546 decls); `bench` SHA-256 `4bd29f39...3c20612`; Build Standard v1.2.0 `100.0%` Structure / `100.0%` Completion / `100.0%` Core. |
