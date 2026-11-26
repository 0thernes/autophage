# 10-Pass Red-Team Audit & 3-Cycle Hardening Review (`2026-11-25`)

- **Subject:** `0thernes/autophage` (`v1.3.0` → `v1.4.0`)
- **Framework:** Build Standard for LLMs `v1.2.0` (90 items, Rules R1–R8, 10 finding classes) & Defensible Independent AI Research Playbook `v1.3` (10-Pass Review Protocol)
- **Reviewer Role:** Self-assessed structured adversarial red-team audit (explicitly **not** external institutional peer review).

## 1. 10-Pass Playbook v1.3 Audit Summary

| Pass | Audit Target | Findings & Classification | Disposition (`v1.4.0`) |
|---|---|---|---|
| **Pass 1** | Identity, Affiliation & Legal Hygiene | Legacy GitHub About still read `v1.0` / `31 theorems` / legacy LLC wording (`problem`); missing root `LICENSE`/`LICENSE-DOCS`/`CITATION.cff` (`gap`). | **RESOLVED:** Standardized every active file and GitHub About to `Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA`; dual-licensed `Apache-2.0` + `CC-BY-4.0`. |
| **Pass 2** | Lean 4.35.0 Kernel Purity & Palomar Isolation | Lacked `lakefile.toml`, `lake-manifest.json`, `module` headers, isolated `Spec.lean`/`Challenge.lean`/`Solution.lean`, `comparator.json`, `formalization.yaml`, and NonVacuity witnesses (`gap`). Initial `T44` `decide` hit `maxRecDepth` (`bug`). | **RESOLVED:** Refactored into full Palomar package (`48/48` theorems incl. `T43`–`T48`); `T44` composed from `T16`+`T13`, `T45` proved via `simp`+`omega`; verified across `lean4`, `leanchecker`, `leanchecker-paranoid`, `nanoda_bin` (2,784 decls), `con-leche` (2,546 decls), `con-ron` (2,546 decls) locally + 6/6 `bubblewrap` CI. |
| **Pass 3** | Axiom Hygiene & README Parity | Needed verbatim `#print axioms` output locked inside `README.md` (`hole`). | **RESOLVED:** Added `<!-- LEAN_AXIOMS_START --> ... <!-- LEAN_AXIOMS_END -->` + `scripts/check_readme_axioms.py` (PASS in local & CI). |
| **Pass 4** | Statement-Fidelity Audit | Required explicit row-by-row mapping from Lean declarations to C11 functions and non-formalized gaps (`gap`). | **RESOLVED:** Authored `docs/statement-fidelity.md` + table in `paper/main.tex` explicitly separating bit-exact models from unformalized x86-64 emission & FP64 drift. |
| **Pass 5** | Live Citation & Bibliography Audit | Repository had no AMS-LaTeX bibliography (`gap`). | **RESOLVED:** Authored `paper/refs.bib` (17 primary references) and ran `scripts/check_citations.py` (`17/17` URLs & DOIs verified HTTP 200 live!). |
| **Pass 6** | Native C11 Build, Sanitizers, Fuzzing & Mutants | Lacked standalone libFuzzer harness, ASan/UBSan CI step, and independent Python `prop_check` (`gap`); `results/01`–`07` had stale `v1.2.0` banners (`issue`). | **RESOLVED:** Rebuilt `v1.4.0` ThinLTO + `-ffp-contract=off`, verified `18/18` gates + `6/6` mutants, regenerated `results/01`–`07` live, added `test/fuzz_jit_libfuzzer.c`, `scripts/prop_check.py`, and Linux ASan/UBSan CI job. |
| **Pass 7** | Statistical Rigor & Benchmark Honesty | Modest end-to-end JIT speedup (`1.04×–1.30×`) vs hot-loop `T05` (`15–18 M/s`) needed prominent disclosure (`concern`); Holm-Bonferroni correction needed explicit write-up (`gap`). | **RESOLVED:** Disclosed `27.35%` cache hit tradeoff prominently in `docs/benchmarks.md`, `LIMITATIONS.md`, and `paper/main.tex`; documented Holm-Bonferroni (`p_Holm = 0.00030`) in `docs/preregistration.md` & `CLAIMS.md`. |
| **Pass 8** | Mega-Atlas Non-Retirement & Real-vs-Metaphor | Needed explicit `0/1,900` cross-check and real-vs-metaphor table (`gap`). | **RESOLVED:** Created `docs/MEGA_ATLAS_CROSSCHECK.md` (`AI-045`, `BIO-001`, `CHEM-052`, `SYS-009` all OPEN) and `LIMITATIONS.md`. |
| **Pass 9** | AMS-LaTeX Typesetting & Clean-Room arXiv Verification | Required zero-overflow Tectonic compilation and clean-room `arxiv_submission.tar.gz` verification (`gap`). | **RESOLVED:** `scripts/build_paper.py` compiles `paper/main.tex` with `0` Overfull `\hbox`, `0` undefined refs/citations, `0` missing characters, and verifies clean-room arXiv compilation. |
| **Pass 10** | Build Standard v1.2.0 & Security Hygiene | Windows cannot run `lean4lean.exe` / `bwrap` (`blindspot`); needed `100.0%` audit score, `GET /healthz`, `CHECKSUMS.sha256`, `data/MANIFEST.sha256` (`gap`). | **RESOLVED:** Documented Windows vs. Ubuntu Linux kernel boundary cleanly; added `GET /healthz`, SHA-256 manifests, and verified `Audit-Repo.ps1` = **100.0% Structure / 100.0% Completion / 100.0% Core** (`0` fails, `0` partials, `0` unknowns, `0` placeholder files). |

## 2. Final Sign-Off

All 13 tracked findings (`H-01`–`H-13`, see [`HARDENING_LOG.md`](../../HARDENING_LOG.md)) across Cycles 1–3 are either fixed and locked by automated verification or explicitly bounded and disclosed as non-fabricable technical tradeoffs (`H-12`, `H-13`).
