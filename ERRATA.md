# Errata & Corrections Log

All post-release errors or metadata corrections identified during red-team audits are documented here rather than silently erased.

| Date | Version Affected | Finding Class | Description | Resolution (`v1.4.0`) |
|---|---|---|---|---|
| 2026-11-25 | `v1.0.0` – `v1.3.0` | `error` | GitHub repository About description remained pinned at `AUTOPHAGE v1.0` / `31 Lean 4.35.0 formal theorems` and carried legacy LLC affiliation text (no LLC entity exists). | Updated GitHub About & topics via `gh repo edit` and standardized all active files to `Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA`. |
| 2026-11-25 | `v1.3.0` | `issue` | `results/01_silicon_probe.txt` through `results/07_castration_ablation_13vec.*` still carried `v1.2.0` headers and pre-`-ffp-contract=off` FP64 accumulator hashes for `03_abiogenesis` and `04_feral`. | Regenerated all `results/01`–`results/07` live from the compiled `v1.4.0` ThinLTO + `-ffp-contract=off` binary (`bench` SHA-256 remains bit-exact `4bd29f39...3c20612`). |
| 2026-11-25 | `v1.3.0` | `gap` | Missing `LICENSE`, `LICENSE-DOCS`, `CITATION.cff`, `lean/lakefile.toml`, Palomar `Challenge`/`Solution` isolation, NonVacuity witness theorems, AMS-LaTeX white paper, and Build Standard v1.2.0 scaffolding. | Upgraded fully to `v1.4.0` with Apache-2.0 / CC-BY-4.0 dual licensing, 48/48 6-kernel Palomar Lean proofs, Tectonic AMS-LaTeX paper + clean-room arXiv tarball, and `100.0%` Build Standard v1.2.0 audit. |
| 2026-11-23 | `v1.0.0` – `v1.1.0` | `hole` | Initial Lean suite relied on 19 `_native.bv_decide.*` external SAT-solver axioms; `soup_engine.c` had dead-cell Landauer, `bio_param` clobbering, and `e_membrane_pot_uq` graveyard bugs. | Fixed in `v1.2.0`: eliminated 100% of external SAT axioms and closed all thermodynamic/causal-knockout loopholes (locked by Gates `T04`, `T06`, `T07`, `T14`). |

## 2026-11-26 statement audit

An independent statement audit of commit `1956c2c` checked the public claims against the Lean sources, the CI log and the paper. Audit log: <https://github.com/0thernes/0thernes-research-archive/blob/main/docs/reviews/2026-11-26-statement-audit.md>. The repository documents (`README.md`, `CLAIMS.md`, `LIMITATIONS.md`, `CITATION.cff`, `lean/formalization.yaml`, `docs/`) were corrected in place. The frozen paper (`paper/main.tex`, `dist/Donahue_2026_autophage.pdf`) and the Lean code are not changed in this pass; their errors are listed here.

### Errors in the frozen paper (`paper/main.tex`, `dist/Donahue_2026_autophage.pdf`)

| Where | What is wrong | Correction | Affects a theorem statement? |
|---|---|---|---|
| `paper/main.tex` l.155, subsection "Symplectic Reversibility, Landauer Accounting, and 5-Pool Conservation" | The sentence attributes conservation of bitwise XOR, bitwise AND and total `popcnt64` to both Toffoli and Fredkin, citing `T17`–`T20a`. | Toffoli and Fredkin are involutions (`T17`, `T18`); only Fredkin is proved to conserve XOR, AND and popcount (`T19`, `T20`, `T20a`). Toffoli changes popcount in general. | No |
| `paper/main.tex` l.193–207, Table "Statement-Fidelity Audit Matrix", groups IV and V | Groups IV (`T23`–`T25a`) and V (`T26`–`T30`) are described as exact isomorphisms / exact ledger, without saying that `T23`, `T24`, `T28`, `T29`, `T30` are one-line identities. `T30` (`sink ≤ sink + rad ∧ 0 * x = 0`) carries a Second-Law name but does not mention the ledger step. | Add: "`T23`, `T24`, `T28`, `T29`, `T30` (and `T21` in group III) are one-line arithmetic or XOR identities; `T30` is not a second-law statement about the engine." | No |
| `paper/main.tex` l.76–90 (Claims Ledger table, rows C1, C4), l.205, l.220, l.324 | C11 test-gate IDs (`T04`, `T05`, `T07`, `T09`, `T10`, `T12`, `T14`) are typeset exactly like Lean theorem IDs with the same numbers (for example Lean `T04` is `gfni_zero_fixed`, Gate `T04` is the JIT/oracle differential test). | Read these as "Gate `Txx`" where they cite tests. | No |
| `paper/main.tex` l.48 (keywords), l.53 (abstract), l.154 | "Landauer thermodynamics" / "Landauer erasure accounting" name a physical principle for an integer bit-erasure count. `LIMITATIONS.md` sec. 1 already disclaims calorimetry. | Read as "bit-erasure accounting". | No |
| `paper/main.tex` l.85 (Claims Ledger C1) | The 48 theorems are not said to be about Lean models only, and the textbook status of the finite-field facts (30 irreducible binary octics, Toffoli/Fredkin involutions, 2×2 inverse) is not stated. | Add: "about the Lean models in `Spec.lean`, not the emitted code; the finite-field and reversible-gate facts are textbook results checked mechanically against this codebase's definitions." | No |

### Known issues in the formal code (not yet fixed)

| Where | What is wrong | Correction | Affects a theorem statement? |
|---|---|---|---|
| `lean/AutophageProofs.lean` l.994, `T30_second_law_monotone_and_noise_gate` | The statement is `sink ≤ sink + rad ∧ 0 * raw_ovi = 0` over free `Nat` variables. It does not refer to `LedgerState` or `applyTransfer`, so it says nothing about the engine, although its name says "Second Law" and "noise gate". | Replace it with a theorem over the ledger step function showing that the radiated-heat sink never decreases, or rename it as an arithmetic lemma. | Yes (a replacement changes the statement) |
| `lean/AutophageProofs.lean` l.882 (`T21`), l.898 (`T23`), l.902 (`T24`), l.969 (`T28`), l.974 (`T29`) | One-line library or `omega` facts (`x ^^^ x = 0`, `a ^^^ b = b ^^^ a`, `g - 1 < g`, `(a - t) + (b + t) = a + b`) named after Landauer, ultrametric, JIT-gas and First-Law properties. | Report them as elementary lemmas (done in the documents above); optionally rename. | No (names only) |

## 2026-11-26 hardening pass (CI ASan AVX-512 FakeStack fix)

| Date | Finding Class | Location | Defect | Resolution |
|---|---|---|---|---|
| 2026-11-26 | `bug` | `build_linux.sh` | `AUTOPHAGE_CI_PORTABLE=1` passed `-march=native`, allowing GCC on AVX-512 GitHub Actions runners (`ubuntu-26.04`) to emit 64-byte aligned `vmovdqa64` stores into ASan FakeStack frames (`+0x20`, only 32-byte aligned; GCC PR101000), triggering `SIGSEGV` in `metrology_excision_regen_cell` under `AUTOPHAGE_SANITIZE=1` (CI run `45904159100`). | Replaced `-march=native` with portable `-march=x86-64` under `AUTOPHAGE_CI_PORTABLE=1` and added `-mno-avx512f` under `AUTOPHAGE_SANITIZE=1`, preserving full ASan/UBSan + `detect_stack_use_after_return=1` coverage across all runner CPUs. |

## v1.5.0 Closure Table (2026-11-26 Deep Hardening)

Every item logged in the `2026-11-26 statement audit` and `CI ASan AVX-512 FakeStack` sections above has been resolved in **`v1.5.0`** while preserving this historical errata record verbatim:

| Errata Item | Status in `v1.5.0` | Concrete Resolution & Verification Artifact |
|---|---|---|
| Paper: Toffoli vs. Fredkin `popcnt64` scope | **RESOLVED** | `paper/main.tex` restricts XOR/AND/`popcnt64` conservation exclusively to Fredkin (`T19`, `T20`, `T20a`) and cites explicit counterexample `AT01_refute_toffoli_popcnt_conserved` (`lean/AdversarialTests.lean`). |
| Paper: Elementary helpers (`T21, T23, T24, T28, T29`) & `T30` table | **RESOLVED** | `paper/main.tex`, `docs/statement-fidelity.md`, and `lean/AutophageProofs.lean` explicitly tag `T21, T23, T24, T28, T29` as elementary arithmetic/XOR helper lemmas and document the new multi-step `T30` + 8 `AdversarialTests` (`AT01`–`AT08`). |
| Paper: `Gate Txx` vs. `Lean Txx` notation collision | **RESOLVED** | Every C11 falsification test citation in `paper/main.tex` (and `README.md`, `CLAIMS.md`, `LIMITATIONS.md`, `docs/`) is now explicitly prefixed `Gate Txx`. |
| Paper: `"Landauer thermodynamics"` terminology | **RESOLVED** | Replaced `"Landauer thermodynamics"` / `"Landauer erasure accounting"` with `"bit-erasure accounting"` in `paper/main.tex`, `CITATION.cff`, and `.zenodo.json`, retaining the explicit calorimetry disclaimer. |
| Paper: `Spec.lean` model scope & textbook-fact attribution | **RESOLVED** | `paper/main.tex` Claims Ledger `C1`, abstract, and formal verification section explicitly state Lean proves properties of `Spec.lean` models (not emitted machine code) and finite-field/reversible-gate facts are textbook results verified mechanically against this codebase's definitions. |
| Lean: Trivial free-`Nat` `T30_second_law_monotone_and_noise_gate` | **RESOLVED** | Replaced with `T30_ledger_heat_sink_monotone_and_zero_weight_gate` over `Spec.lean`'s `LedgerState` & `applyTransfers` proving heat-sink monotonicity, exact residual conservation, and `oviGate 0 raw_ovi = 0` (`[propext, Quot.sound]`), backed by negative refutation `AT04` and live tamper suite `scripts/test_lean_negative.py`. |
| Lean: Elementary helper documentation (`T21, T23, T24, T28, T29`) | **RESOLVED** | Explicitly labeled `[Elementary helper lemma]` in `lean/AutophageProofs.lean`, `docs/statement-fidelity.md`, `docs/blueprint.html`, and `paper/main.tex`. |
| CI: Ubuntu ASan SEGV `#45904159100` (`metrology_excision_regen_cell`) | **RESOLVED** | Added `AP_ALIGN64` (`_Alignas(64)`) to all 64B/128B stack genome buffers (`src/metrology_ovbs.c`, `src/soup_engine.c`, `src/hw_isa.c`, `test/test_falsification.c`), forced `__asan_stack_malloc_2`, hardened `build_linux.sh` (`-march=x86-64`, `-mno-avx512f` under sanitizer), and added permanent B07 regression **Gate `T19`** (`test_t19_b07_regression_excision_alignment`). |
