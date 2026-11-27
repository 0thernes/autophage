# Changelog

All notable changes to **AUTOPHAGE** are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and Semantic Versioning (`VERSION`).

## [1.5.0] - 2026-11-26

### Fixed
- **Live Ubuntu CI ASan/UBSan AVX-512 FakeStack `SIGSEGV` (`#45904159100`, Build Standard B07):** Root-caused crash in `metrology_excision_regen_cell` (`src/metrology_ovbs.c:164`) to GCC PR101000 (`-O3 -march=native -fsanitize=address` emitting aligned 64-byte `vmovdqa64` stores into 32-byte-aligned ASan FakeStack slots `+0x20` on AVX-512 runners). Added C11 `_Alignas(64)` (`AP_ALIGN64`) on 64B/128B stack genome buffers across `src/metrology_ovbs.c`, `src/soup_engine.c`, `src/hw_isa.c`, and `test/test_falsification.c`; fixed `build_linux.sh` to use `-march=x86-64 -ffp-contract=off` in portable mode and `-mno-avx512f` under `AUTOPHAGE_SANITIZE=1`; added dedicated regression **Gate `T19`** (`test_t19_b07_regression_excision_alignment`); and added `--sysroot=$root` in `build.ps1` to prevent WinLibs MinGW `crt2.o` collision with bundled Lean Clang.
- **Lean 4.35.0 Formal Defect Closure (`T30` & Elementary Helpers):** Replaced trivial free-`Nat` `T30_second_law_monotone_and_noise_gate` with genuine multi-step `T30_ledger_heat_sink_monotone_and_zero_weight_gate` over `Spec.lean`'s `LedgerState` and `applyTransfers` proving radiated-heat sink monotonicity (`s.e_heat_snk ≤ (applyTransfers s ops).e_heat_snk`), exact residual conservation, and `oviGate 0 raw_ovi = 0` (`[propext, Quot.sound]`, zero `Classical.choice`). Explicitly marked elementary arithmetic/XOR helper lemmas (`T21`, `T23`, `T24`, `T28`, `T29`) in Lean docstrings, `docs/statement-fidelity.md`, and `paper/main.tex`.
- **All 5 Frozen White-Paper Errors from `ERRATA.md`:** Corrected Toffoli vs. Fredkin `popcnt64` scope, elementary helper classification vs. genuine `T30`, explicit `Gate Txx` vs. `Lean Txx` notation, `"bit-erasure accounting"` terminology, and explicit `Spec.lean`-model & textbook-fact boundary; added `"not peer-reviewed"`, Software Heritage SWHID (`swh:1:snp:6ea23803655df1f9679deb98d44efbf53321a393`), SLSA/Rekor provenance, and DigiCert RFC 3161 + Bitcoin OTS timestamps.
- **Claim Wording Tightening (`2026-11-26 statement audit`, commits `005502a` & `613c640`):** Preserved all epistemic tightenings across `README.md`, `CLAIMS.md`, `LIMITATIONS.md`, `CITATION.cff`, `HARDENING_LOG.md`, `lean/formalization.yaml`, `docs/MEGA_ATLAS_CROSSCHECK.md`, `docs/adr/0001-*.md`, and `docs/glossary.md`, including exact kernel independence accounting (`lean4`, `leanchecker`, `leanchecker-paranoid` replay with the Lean reference kernel; 3 independent Rust checkers run locally on Windows; 6-kernel Palomar + `lean4lean` + `bwrap` runs in Ubuntu CI).

### Added
- **Adversarial False-Variant Refutation Module (`lean/AdversarialTests.lean`, `AT01`–`AT08` → `56 / 56` Total Declarations):** Eight kernel-checked `¬P` refutations wired into `Challenge.lean`, `Solution.lean`, `VerifyAxioms.lean`, and `comparator.json` (Toffoli popcount counterexample, singular rank-7 ribosome, reducible `0x01` divisor & Cauchy failure, unguarded ledger transfer/negative radiation, nonzero `AND` erasure, wrong-mask `PEXT`/`PDEP` bound, unconstrained Philox/seed-band collisions, and germline write breaking isolation).
- **Live Negative Lean Elaboration & Tamper Rejection Harness (`scripts/test_lean_negative.py`):** Verifies live Lean 4.35.0 catches and rejects false claims (`1 = 2`, false Toffoli popcount) as well as planted `sorryAx`, custom `axiom`, and `native_decide` tampering; wired into `.github/workflows/ci.yml`.
- **Upgraded Live Citation Alignment Audit (`scripts/check_citations.py` → `results/citation-audit.json`):** Verifies HTTP/Crossref/DataCite reachability PLUS title token overlap, author surname alignment, publication year parity, and zero retractions across all `17 / 17` citations (`14 DOIs + 3 URLs`).
- **Interactive Lean 4.35.0 Proof Blueprint & GitHub Pages Portal (`docs/index.html`, `docs/blueprint.html`, `scripts/build_blueprint.py`, `scripts/check_blueprint.py`, `.github/workflows/pages.yml`):** WCAG 2.2 AA, strict CSP, zero external CDNs, synchronized with all 56 audited theorems.
- **Cryptographic Dual Timestamping & Automated Release Workflow:** Added `scripts/stamp_timestamps.py` (DigiCert RFC 3161 `CHECKSUMS.sha256.tsr` + 3-calendar Bitcoin `CHECKSUMS.sha256.ots` verified in CI) and `.github/workflows/release.yml` (plain unsigned tags per R1 + `SHA256SUMS` + SLSA attestation).
- **FAIR Metadata, Datasheet, Readiness Matrix & REUSE/SPDX:** Added `croissant.json`, `ro-crate-metadata.json`, `codemeta.json`, `docs/DATASHEET.md`, `SUBMISSION_READINESS.md`, `REUSE.toml`, `LICENSES/Apache-2.0.txt`, and `LICENSES/CC-BY-4.0.txt`.
- **ABI Bump (`0x00010500u`):** Bumped `AUTOPHAGE_ABI_VERSION` and `VERSION` to `1.5.0` (backwards-compatible struct layouts).

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
