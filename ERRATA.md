# Errata & Corrections Log

All post-release errors or metadata corrections identified during red-team audits are documented here rather than silently erased.

| Date | Version Affected | Finding Class | Description | Resolution (`v1.4.0`) |
|---|---|---|---|---|
| 2026-11-25 | `v1.0.0` – `v1.3.0` | `error` | GitHub repository About description remained pinned at `AUTOPHAGE v1.0` / `31 Lean 4.35.0 formal theorems` and carried legacy LLC affiliation text (no LLC entity exists). | Updated GitHub About & topics via `gh repo edit` and standardized all active files to `Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA`. |
| 2026-11-25 | `v1.3.0` | `issue` | `results/01_silicon_probe.txt` through `results/07_castration_ablation_13vec.*` still carried `v1.2.0` headers and pre-`-ffp-contract=off` FP64 accumulator hashes for `03_abiogenesis` and `04_feral`. | Regenerated all `results/01`–`results/07` live from the compiled `v1.4.0` ThinLTO + `-ffp-contract=off` binary (`bench` SHA-256 remains bit-exact `4bd29f39...3c20612`). |
| 2026-11-25 | `v1.3.0` | `gap` | Missing `LICENSE`, `LICENSE-DOCS`, `CITATION.cff`, `lean/lakefile.toml`, Palomar `Challenge`/`Solution` isolation, NonVacuity witness theorems, AMS-LaTeX white paper, and Build Standard v1.2.0 scaffolding. | Upgraded fully to `v1.4.0` with Apache-2.0 / CC-BY-4.0 dual licensing, 48/48 6-kernel Palomar Lean proofs, Tectonic AMS-LaTeX paper + clean-room arXiv tarball, and `100.0%` Build Standard v1.2.0 audit. |
| 2026-11-23 | `v1.0.0` – `v1.1.0` | `hole` | Initial Lean suite relied on 19 `_native.bv_decide.*` external SAT-solver axioms; `soup_engine.c` had dead-cell Landauer, `bio_param` clobbering, and `e_membrane_pot_uq` graveyard bugs. | Fixed in `v1.2.0`: eliminated 100% of external SAT axioms and closed all thermodynamic/causal-knockout loopholes (locked by Gates `T04`, `T06`, `T07`, `T14`). |
