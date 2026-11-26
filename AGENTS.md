# AGENTS.md — AUTOPHAGE v1.4.0 Canonical Engineering & Verification Guide

**Repository:** `0thernes/autophage` (`v1.4.0`, ABI `0x00010400`)
**Author:** Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA (`ORCID 0009-0006-8168-9648`, `0_0@0thernes.art`)
**Licenses:** Apache-2.0 (`LICENSE`, code/Lean/scripts/CI) · CC-BY-4.0 (`LICENSE-DOCS`, white paper/docs/figures)
**Role:** Bare-metal polymorphic x86-64 Galois-field genetic engine (`C11` + live RWX JIT + 48 Pure Lean 4.35.0 6-kernel theorems + 18-gate silicon falsification suite + 6 planted negative mutants).

---

## 1. Hard Repository Rules (Build Standard v1.2.0, R1–R8)

- **R1–R2 (Direct-to-`main` Workflow):** Commit and push directly to `main`. No feature branches, no pull requests, no PR templates, no commit/tag signing, no git hooks, and no blocking branch protection.
- **R3 (Informational CI):** `.github/workflows/ci.yml` runs on every push to `main` and tag `v*` as an informational verification ledger across Windows, Ubuntu Linux, 6-kernel Lean 4.35.0, and Tectonic AMS-LaTeX.
- **R4 (Banned Files):** Never add `.github/dependabot.yml`, `renovate.json`, `PULL_REQUEST_TEMPLATE*`, `.github/copilot-instructions.md`, or `FUNDING.yml`.
- **R6 (Zero Fabrication):** Never invent benchmark numbers, test results, SHA-256 hashes, citations, DOIs, or peer-review claims. Every number in `README.md`, `CLAIMS.md`, `docs/benchmarks.md`, and `paper/main.tex` must trace directly to a real execution logged in `results/`.
- **R7 (10 Finding Classes):** Classify every hardening finding into exactly one of: `bug`, `error`, `issue`, `problem`, `concern`, `dilemma`, `conflict`, `gap`, `hole`, `blindspot` (see `docs/glossary.md` and `.github/labels.tsv`).
- **Identity & Affiliation Rule:** Use `Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA` across all active metadata, licenses, papers, and documentation.
- **Mega-Atlas Non-Retirement Rule:** Explicitly maintain `0 / 1,900` overarching Mega-Atlas open problems solved or retired (see `docs/MEGA_ATLAS_CROSSCHECK.md`).
- **PowerShell Bracket-Safety Rule:** Local paths under `Z:\[Vibe Coded (AI)]\...` contain literal `[` and `]`. Always pass `-LiteralPath` to PowerShell file cmdlets (`Test-Path -LiteralPath`, `Get-Content -LiteralPath`, `Set-Location -LiteralPath`, `Copy-Item -LiteralPath`).

---

## 2. Canonical Commands (Clean Clone → Full Verification)

| Task | Windows (PowerShell 7) | Linux / WSL2 (Bash) |
|---|---|---|
| **Build C11 + 18 Gates + 48 Lean Proofs** | `pwsh -NoProfile -File build.ps1` | `bash build_linux.sh` |
| **Sanitizer Build (ASan + UBSan)** | See Ubuntu CI (`-fsanitize=address,undefined`) | `AUTOPHAGE_SANITIZE=1 bash build_linux.sh` |
| **Multi-Kernel Lean 4.35.0 Audit** | `pwsh -NoProfile -File scripts/verify_lean_multikernel.ps1` | `cd lean && lake build && lake build Challenge && lake check --paranoid && lake comparator --config comparator.json --paranoid` |
| **Python Property Checks (`prop_check`)** | `python scripts/prop_check.py` | `python3 scripts/prop_check.py` |
| **README Axiom Parity Check** | `python scripts/check_readme_axioms.py` | `python3 scripts/check_readme_axioms.py` |
| **Live Citation & DOI Check** | `python scripts/check_citations.py` | `python3 scripts/check_citations.py` |
| **Compile AMS-LaTeX White Paper** | `python scripts/build_paper.py` | `python3 scripts/build_paper.py` |
| **Build Standard v1.2.0 Self-Audit** | `pwsh -NoProfile -File scripts/Audit-Repo.ps1` | `bash scripts/audit-repo.sh .` |
| **Launch Silicon Observatory UI** | `node web/server.mjs` (`http://127.0.0.1:8787`) | `node web/server.mjs` (`http://127.0.0.1:8787`) |

---

## 3. Repository Layout & Critical Invariants

- `include/autophage_abi.h`, `include/autophage_internal.h`, `include/bare_crt.h`: Public C11 ABI (`0x00010400`) and internal cache-line structs (`_Static_assert(sizeof(AutophageCell) == 256)`, `_Static_assert(sizeof(AutophageExecCtx) == 192)`).
- `src/`: Zero-dependency C11 engine (`hw_isa.c`, `galois_math.c`, `jit_x86_64.c`, `soup_engine.c`, `metrology_ovbs.c`, `abi_exports.c`, `main_cli.c`).
- `test/test_falsification.c`, `test/fuzz_jit_libfuzzer.c`: 18 bare-metal falsification gates (`T01`–`T18`) + 6 planted negative mutants + libFuzzer differential JIT/scalar harness.
- `lean/`: Pure Lean 4.35.0 Palomar formalization (`AutophageProofs/Spec.lean`, `AutophageProofs.lean`, `Challenge.lean`, `Solution.lean`, `VerifyAxioms.lean`, `comparator.json`, `formalization.yaml`, `lakefile.toml`, `lake-manifest.json`). Strictly 48 theorems, `0` `sorry` in `AutophageProofs`/`Solution`, `0` `native_decide`/`bv_decide`, axioms strictly within `{propext, Classical.choice, Quot.sound}`.
- `paper/` & `dist/`: Publication-grade AMS-LaTeX white paper (`paper/main.tex`, `paper/refs.bib`), frozen PDF (`dist/Donahue_2026_autophage.pdf`), and deterministic clean-room verified `dist/arxiv_submission.tar.gz`.
- `results/`: Real measured outputs regenerated directly from `bin/autophage.exe` and `lean/` (never hand-edited).
- `docs/`: Full engineering, verification, statement-fidelity, Mega-Atlas cross-check, AI disclosure, benchmarks, threat model, preregistration, ADRs, and 10-pass red-team review.

---

## 4. Mandatory Post-Edit Checklist

1. After any C11 change: rebuild (`build.ps1` / `build_linux.sh`), verify `18/18` gates + `6/6` mutants pass, verify `0`-bit JIT/Scalar SHA-256 diff, and check `post_warmup_allocs == 0`.
2. After any Lean change: run `scripts/verify_lean_multikernel.ps1`, sync verbatim axioms into `README.md`, and run `scripts/check_readme_axioms.py`.
3. After any `paper/` change: run `scripts/check_citations.py` and `scripts/build_paper.py` (`0` Overfull `\hbox`, `0` undefined refs/citations, clean-room tarball pass).
4. Before committing: run `pwsh -NoProfile -File scripts/Audit-Repo.ps1` and ensure `100.0%` Structure / `100.0%` Completion / `100.0%` Core, `0` placeholder files, and updated `CHECKSUMS.sha256` & `data/MANIFEST.sha256`.
