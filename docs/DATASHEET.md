# Datasheet for AUTOPHAGE Empirical & Formal Audit Datasets (`v1.5.0`)

Following Gebru et al., *Datasheets for Datasets* (*Communications of the ACM*, 2021), this document describes the deterministic machine-generated datasets and audit transcripts shipped under `results/` and indexed in `croissant.json`, `ro-crate-metadata.json`, and `data/MANIFEST.sha256`.

## 1. Motivation
- **Purpose:** Provide bit-exact, seed-locked empirical evidence and machine-checked formal verification transcripts backing Claims `C1`–`C9` of **AUTOPHAGE v1.5.0** (`0thernes/autophage`).
- **Creator:** Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA (`ORCID 0009-0006-8168-9648`, `0_0@0thernes.art`).
- **Funding:** Self-funded independent research; no external grants or commercial sponsors.

## 2. Composition
All dataset files are plain UTF-8 text, CSV, or JSON artifacts generated directly by compiled C11 binaries (`bin/autophage*`), Lean 4.35.0 (`VerifyAxioms.lean`), and `scripts/check_citations.py`:

| File | Format | Records / Scope | Deterministic Seed(s) / Provenance |
|---|---|---|---|
| `results/01_silicon_probe.txt` | Text | CPUID ISA flags, 19-gate + 6-mutant suite output | Hardware probe + deterministic test seeds |
| `results/02_bench_win64.txt` | Text | 200-epoch `32×32` `NATIVE_JIT` vs. `SCALAR_REF` Win64 parity | Seed `5000`, SHA-256 `4bd29f39...3c20612` |
| `results/02_bench_linux.txt` | Text | 200-epoch `32×32` `NATIVE_JIT` vs. `SCALAR_REF` Linux parity | Seed `5000`, SHA-256 `4bd29f39...3c20612` |
| `results/03_abiogenesis_zero_seed.txt` | Text | 300-epoch zero-seed `Philox4x32-10` emergence trace | Seed `5000` (locked held-out band `>= 5000`) |
| `results/04_feral_vs_constitutional.txt` | Text | 250-epoch Track A Constitutional vs. Track B Feral trace | Seed `5007` (locked held-out band) |
| `results/05_vivisection_shock_transplant.txt` | Text | 90% extinction shock, 25/50/75% excision & transplant | Seed `5020` (locked held-out band) |
| `results/06_paired_stats_n24.txt` | Text | `N=24` locked paired seeds × 3 preregistered hypotheses | Seeds `5000..5023`, 10,000 sign-flip perms |
| `results/07_castration_ablation_13vec.csv` | CSV | 30 rows × 24 columns (global `0..100%` + 13 restraints) | Seed `5010`, 140 epochs |
| `results/07_castration_ablation_13vec.txt` | Text | Human-readable mirror of 30-condition ablation table | Seed `5010`, 140 epochs |
| `results/lean4-audit.txt` | Text | 56 verbatim Lean 4.35.0 `#print axioms` lines | `lake env lean VerifyAxioms.lean` |
| `results/lean4-multi-kernel.txt` | Text | Windows reference kernel + 3 Rust checker replay transcript | `scripts/verify_lean_multikernel.ps1` |
| `results/citation-audit.json` | JSON | 17 live Crossref/DataCite/HTTP citation alignment audits | `scripts/check_citations.py` |

- **Personal / Sensitive Data:** None (`personal_data: false`). Every record is synthetic digital simulation output, formal theorem audit output, or bibliographic metadata.
- **Missing Values / Noise:** Zero missing values in `07_castration_ablation_13vec.csv`. All simulation records are deterministic bit-exact snapshots from single-threaded (`threads = 1`) execution with `-ffp-contract=off`.

## 3. Collection & Preprocessing
- Generated directly from clean builds (`build.ps1`, `build_linux.sh`, `scripts/verify_lean_multikernel.ps1`, `scripts/check_citations.py`) without manual editing or post-hoc filtering.
- Preregistered statistical hypotheses (`docs/preregistration.md`) enforce strict separation between Practice (`100..999`), Tuning (`2000..2999`), and Locked Held-Out (`>= 5000`) seed bands (`T40`, `T46`, `AT07`).
- Integrity verified via `data/MANIFEST.sha256`, `CHECKSUMS.sha256`, DigiCert RFC 3161 (`CHECKSUMS.sha256.tsr`), and 3-calendar Bitcoin OpenTimestamps (`CHECKSUMS.sha256.ots`).

## 4. Uses & Limitations
- **Recommended Uses:** Independent replication of AUTOPHAGE's formal/empirical benchmarks, meta-analysis of open-ended artificial-life proxy metrics (`OVBS-52`, `BedauPackard_Anew`), and verification-pipeline benchmarking.
- **Out-of-Scope Uses:** Must NOT be interpreted as biological wet-lab measurements, physical CMOS calorimetry, or resolution of any Unsolved Problems Mega-Atlas open problem (`0 / 1,900` retired; see `LIMITATIONS.md`).

## 5. Distribution & Maintenance
- **License:** CC-BY-4.0 (`LICENSE-DOCS`, `LICENSES/CC-BY-4.0.txt`) for datasets, transcripts, and paper; Apache-2.0 (`LICENSE`, `LICENSES/Apache-2.0.txt`) for generator code and Lean proofs.
- **Archival Copies:** GitHub (`https://github.com/0thernes/autophage`), GitHub Pages (`https://0thernes.github.io/autophage/`), and Software Heritage (`swh:1:snp:6ea23803655df1f9679deb98d44efbf53321a393`).
- **Maintainer:** Alexander Stephen Donahue (`0_0@0thernes.art`). Corrections are recorded append-only in `ERRATA.md`.
