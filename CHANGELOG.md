# Changelog

All notable changes to **AUTOPHAGE** are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and Semantic Versioning (`VERSION`).

## [Unreleased] - 2026-11-26

### Fixed
- **Claim wording after the 2026-11-26 statement audit** (documentation only; no Lean, C11, data or paper changes): `README.md`, `CLAIMS.md`, `LIMITATIONS.md`, `CITATION.cff`, `lean/formalization.yaml` and `docs/` now state that the 48 Lean theorems concern Lean models rather than emitted machine code; list `T21`, `T23`, `T24`, `T28`, `T29`, `T30` as one-line identities; drop `T30` as evidence for "0 Second-Law violations" (an empirical gate result); restrict popcount conservation to Fredkin; label C11 test gates as "Gate `Txx`"; state that Lean `T26`–`T28` prove mask and gas arithmetic only; fix the local kernel count; note the textbook status of the finite-field facts. Paper and Lean issues are recorded in `ERRATA.md`.

## [1.4.0] - 2026-11-25

### Added
- **Six-Kernel Lean 4.35.0 Palomar Formalization (`lean/`):** Refactored into `module`-based `AutophageProofs/Spec.lean` (pure definitions, zero proofs, zero `sorry`), `AutophageProofs.lean` (`public` theorems), isolated `Challenge.lean` (statement-only `sorry` specifications importing ONLY `AutophageProofs.Spec`), `Solution.lean`, `VerifyAxioms.lean`, `lakefile.toml` (`autoImplicit = false`, `relaxedAutoImplicit = false`, `linter.unusedVariables = true`, `linter.deprecated = true`), `lake-manifest.json`, `comparator.json`, and `formalization.yaml` (v0.4 schema).
- **6 New Pure-Kernel Non-Vacuity & Negative-Boundary Theorems (`T43`–`T48` → `48 / 48` total):** Concrete witnesses for asymmetric Weismann inversion (`T43`), Cauchy MDS recovery vs. reducible boundary (`T44`), 5-pool multi-step ledger + 1-uq leak detection (`T45`), 3-tier seed firewall + Philox domains (`T46`), plus adversarial refutations of reducible quotient integral domains (`T47`) and unmasked/byte-masked qword heads (`T48`).
- **Local Multi-Kernel & CI Verification:** Added `scripts/verify_lean_multikernel.ps1`, `results/lean4-multi-kernel.txt`, `scripts/check_readme_axioms.py`, `scripts/prop_check.py`, `test/fuzz_jit_libfuzzer.c`, and unified `.github/workflows/ci.yml` (Win64 + Ubuntu C11, ASan/UBSan, 6-kernel bubblewrap Lean 4.35.0, Tectonic PDF, SLSA provenance, `gitleaks`, `cppcheck`).
- **Publication-Grade AMS-LaTeX White Paper (`paper/`, `dist/`):** Authored `paper/main.tex`, `paper/refs.bib` (17 live-verified citations), `scripts/build_paper.py`, `scripts/check_citations.py`, frozen `dist/Donahue_2026_autophage.pdf`, and clean-room verified `dist/arxiv_submission.tar.gz`.
- **Build Standard v1.2.0 Scaffolding (100.0% Structure / Completion / Core):** Added `VERSION`, `LICENSE` (Apache-2.0), `LICENSE-DOCS` (CC-BY-4.0), `NOTICE`, `THIRD_PARTY_NOTICES.md`, `CITATION.cff`, `CITATION.bib`, `.zenodo.json`, `GEMINI.md`, `.cursor/rules/build-standard.mdc`, `llms.txt`, `llms-full.txt`, `CLAIMS.md`, `LIMITATIONS.md`, `REPLICATION.md`, `DEVIATIONS.md`, `ERRATA.md`, `EVIDENCE.md`, `HARDENING_LOG.md`, `ROADMAP.md`, `SECURITY.md`, `SUPPORT.md`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `.editorconfig`, `.env.example`, `justfile`, `repo.yaml`, `CHECKSUMS.sha256`, `data/MANIFEST.sha256`, `examples/`, and complete `docs/` suite (`statement-fidelity.md`, `MEGA_ATLAS_CROSSCHECK.md`, `AI_DISCLOSURE.md`, `benchmarks.md`, `preregistration.md`, `threat-model.md`, `operations.md`, `adr/`, `media/social-preview.svg`, `reviews/`).
- **Observatory Health Endpoint:** Added `GET /healthz` to `web/server.mjs` and bumped ABI macro `AUTOPHAGE_ABI_VERSION` to `0x00010400u` (strictly backwards-compatible struct layouts).

### Changed
- **Author Affiliation Normalization:** Standardized author metadata across all active files to `Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA` (ORCID `0009-0006-8168-9648`), replacing legacy entity wording.
- **Empirical Artifact Regeneration:** Regenerated `results/01`–`results/07` live from the `v1.4.0` ThinLTO + `-ffp-contract=off` binary (`18/18` gates PASS, `6/6` mutants caught, `0`-bit JIT == Scalar SHA-256 parity).

### Removed
- Replaced legacy `.github/workflows/lean-verify.yml` with unified `.github/workflows/ci.yml`.

## [1.3.0] - 2026-11-24

### Added
- Added 4 pure-kernel Lean 4 theorems (`T39`–`T42` → `42/42` total): Philox 8-tuple domain disjointness, 3-tier seed firewall disjointness, OEE archive monotone growth, and Weismann soma-only perturbation germline isolation, plus matching compiled checks in Gates `T08` and `T16`.
- Enabled Win64 Arrow Lake AVX2/AVX-VNNI/BMI2/GFNI + ThinLTO + `-ffp-contract=off` in `build.ps1`.

## [1.2.0] - 2026-11-23

### Fixed
- Eliminated all 19 `_native.bv_decide.*` SAT axioms from `lean/AutophageProofs.lean`, upgrading to 38 pure-kernel theorems strictly within `{propext, Classical.choice, Quot.sound}`.
- Closed dead-cell Landauer free-execution loophole, `OP_METAB_HARVEST` double-XOR cancellation, `bio_param` cross-opcode clobbering (expanded `AutophageExecCtx` to 192B), Markov-blanket partial-write bypass, free Planarian ECC birth gifting, singular-ribosome `OP_REV_GFNI_FOLD` Landauer loophole, fake `NERFING` `rev_ops++`, `e_membrane_pot_uq` graveyard, and spatial locality (`detritus_grid` + 4-quadrant `stigmergy_bus`).
- Smoothed all 13 Omega restraint curves, fixed `BedauPackard_Anew` extinct-lineage eviction, hardened `.apbin` v2 SHA-256 tamper verification, wired live post-warmup allocator guard (`T12`), verified all 4 backlash vectors (`T17`), and hardened `web/server.mjs`.

## [1.0.0] - 2026-11-20

### Added
- Initial release of AUTOPHAGE bare-metal polymorphic x86-64 Galois-field genetic engine, 18-gate falsification suite, zero-dependency Web Silicon Observatory, and initial Lean 4 verification suite.
