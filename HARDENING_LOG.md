# Hardening Log — AUTOPHAGE v1.4.0 (3-Cycle Adversarial Audit)

Every finding discovered across Cycles 1–3 is classified into **exactly one** of the 10 Build Standard v1.2.0 finding classes (`bug`, `error`, `issue`, `problem`, `concern`, `dilemma`, `conflict`, `gap`, `hole`, `blindspot`) per Rule R7.

## Cycle 1 — Core Engine, Thermodynamics & Lean Kernel Purity (`v1.0.0` → `v1.3.0`)

| ID | Class | Summary | Concrete Fix & Verification |
|---|---|---|---|
| **H-01** | `hole` | 19 Lean 4 theorems relied on `_native.bv_decide.*` SAT-oracle axioms rather than pure kernel proofs. | Reproved all theorems algebraically/constructively inside Lean 4 (`0` `bv_decide`, `0` `native_decide`); verified by `VerifyAxioms.lean`. |
| **H-02** | `bug` | `OP_REV_GFNI_FOLD` charged `0` Landauer heat even on singular ($\mathrm{rank}(M)<8$) matrices, and `NERFING=100` incremented `rev_ops` on stripped NOPs. | Added runtime `ribosome_rank` check in `AutophageExecCtx` (`+0xB0`) charging exact `POPCNT` heat when `rank < 8`; locked by Gate `T06`. |
| **H-03** | `bug` | Shared `bio_param` slot clobbered arguments across multiple bio-traps in the same tick; `e_membrane_pot_uq` lacked per-cell tracking (graveyard leak on extinction). | Expanded `AutophageExecCtx` to 192B with dedicated per-action parameter slots (`_Static_assert`) and added per-cell `membrane_pot_uq`; locked by Gates `T04`, `T07`. |
| **H-04** | `hole` | Post-warmup heap allocation counter (`P10`/`HW02`) needed live negative-control trip testing and `.apbin` snapshots needed tamper rejection. | Wired `g_ap_in_step_guard` + negative-control `ap_malloc` probe (`T12`) and `.apbin` v2 full-payload SHA-256 verification (`T11`). |
| **H-05** | `gap` | Missing formal proofs for Philox domain separation, 3-tier seed firewall, OEE archive growth, and Weismann soma isolation. | Added `T39`–`T42` in Pure Lean 4 + matching compiled C11 checks in Gates `T08` and `T16`. |

## Cycle 2 — 6-Kernel Palomar Upgrade, Identity & Build Standard v1.2.0 (`v1.3.0` → `v1.4.0`)

| ID | Class | Summary | Concrete Fix & Verification |
|---|---|---|---|
| **H-06** | `problem` | Legacy GitHub About description (`v1.0`, `31 theorems`) and historical references carried a legacy LLC affiliation string (no LLC entity exists). | Removed every LLC affiliation reference from every active file and GitHub About; standardized on `Independent Researcher, Spokane, WA, USA`. |
| **H-07** | `gap` | Lean formalization lacked `lakefile.toml`, `lake-manifest.json`, Palomar `module` headers, isolated `Spec.lean`/`Challenge.lean`/`Solution.lean`, `comparator.json`, `formalization.yaml`, and NonVacuity witnesses. | Created full Palomar package (`48/48` theorems incl. `T43`–`T48`), verified across 5 independent Windows kernels + 6/6 Ubuntu CI bubblewrap job. |
| **H-08** | `bug` | Initial `T44_nonvacuous_cauchy_mds_witness` used bare `decide` on `cauchyRecLo`, hitting Lean `maxRecDepth` during `lake build`. | Composed `T44` directly from `T16_planarian_50pct_exact_recovery` + `T13_aes_poly_irreducible` and proved `T45` via `simp` + `omega`; `lake build` and all independent checkers pass with 0 warnings. |
| **H-09** | `issue` | `results/01`–`07` still had `v1.2.0` banners and pre-`-ffp-contract=off` FP64 Kahan-Neumaier hashes on 4 epochs of `03_abiogenesis` and `04_feral`. | Regenerated all empirical `results/` files live from `v1.4.0` binary; verified bit-exact `bench` SHA-256 (`4bd29f39...3c20612`) across JIT and Scalar. |
| **H-10** | `gap` | No standalone libFuzzer differential harness, ASan/UBSan CI step, independent Python `prop_check.py`, or `/healthz` endpoint on `web/server.mjs`. | Added `test/fuzz_jit_libfuzzer.c`, ASan/UBSan Linux CI job, `scripts/prop_check.py`, and `GET /healthz` in `web/server.mjs`. |

## Cycle 3 — Publication-Grade AMS-LaTeX White Paper & 10-Pass Red-Team Audit (`v1.4.0`)

| ID | Class | Summary | Concrete Fix & Verification |
|---|---|---|---|
| **H-11** | `gap` | Repository lacked a formal AMS-LaTeX white paper, verified primary bibliography, clean-room arXiv tarball, and `MEGA_ATLAS_CROSSCHECK.md`. | Authored `paper/main.tex` + `paper/refs.bib` (`17/17` live-verified DOIs/URLs), vector PGFPlots figures, `scripts/build_paper.py` (`0` Overfull `\hbox`, clean-room tarball PASS), and `docs/MEGA_ATLAS_CROSSCHECK.md` (`0/1,900` retired). |
| **H-12** | `concern` | End-to-end evolutionary `NATIVE_JIT` speedup (`1.04×–1.15×` Win64; `1.30×` Linux) is much lower than hot-loop cached JIT throughput (`~15–18 M execs/s`) due to `27.35%` cache hit rate under high mutation. | Disclosed prominently in `docs/benchmarks.md`, `LIMITATIONS.md`, and `paper/main.tex` rather than cherry-picking only the cached `T05` throughput number. |
| **H-13** | `blindspot` | Windows hosts cannot run `lean4lean.exe` (`.olean` header format) or `bubblewrap` (`bwrap`). | Explicitly documented the 5-kernel local Windows vs. 6-kernel Ubuntu Linux CI verification boundary in `results/lean4-multi-kernel.txt`, `DEVIATIONS.md`, `LIMITATIONS.md`, and `paper/main.tex`. |
| **H-14** | `bug` | Visual inspection of rendered `dist/Donahue_2026_autophage.pdf` caught `\nolinkurl` stripping internal spaces inside 26 multi-word `\code{...}` / `\tcode{...}` spans (e.g., `lakecomparator`). | Added `\PassOptionsToPackage{obeyspaces}{url}` before `\usepackage{xurl}` in `paper/main.tex`, rebuilt PDF & clean-room arXiv tarball (`0` Overfull `\hbox`), visually verified rendered PNG, and refreshed `CHECKSUMS.sha256`. |

**Correction (2026-11-26):** H-07 and H-13 above count the local Windows replay as five independent kernels. Of the checkers `scripts/verify_lean_multikernel.ps1` runs, `lean4`, `leanchecker` and `leanchecker-paranoid` replay with the Lean reference kernel; only `nanoda_bin`, `con-leche` and `con-ron` are independent implementations. See `README.md` and `LIMITATIONS.md`.
