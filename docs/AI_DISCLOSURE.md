# AI-Assistance Transparency & CRediT Authorship Disclosure (`v1.5.0`)

**Principal Author & Sole Responsible Human-in-the-Loop:** Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA (`ORCID 0009-0006-8168-9648`, `0_0@0thernes.art`, `https://github.com/0thernes`).

## 1. Transparent Disclosure of Human + AI-Assisted Research Engineering

AUTOPHAGE is an independent solo research software and formal-verification project developed with frontier AI coding and proof-engineering assistance (`Gemini / Antigravity` and `Claude`). Rather than concealing AI tooling or misrepresenting AI review as institutional peer review, this repository discloses exact contributions under the CRediT taxonomy:

- **Human Author Role (Alexander Stephen Donahue — sole author):** Conceptualization, Methodology, Investigation, Supervision, Project Administration, Software Architecture Direction, Epistemic Verification Policy, Visual/Typesetting Sign-Off, and Sole Scholarly Accountability.
- **Disclosed AI Tooling Role (Non-Author Assistants):** Assisted drafting of C11 engine/JIT/falsification modules, Lean 4.35.0 tactic proof refactoring, `AdversarialTests.lean`, and Palomar `Spec`/`Challenge`/`Solution` scaffolding, Python property/citation/blueprint/timestamp scripts, Build Standard v1.2.0 & Playbook v1.3 documentation, and AMS-LaTeX typesetting under human direction and machine verification.

## 2. Hard Anti-Hallucination & Integrity Guardrails

Because AI-assisted scientific engineering carries documented failure modes (citation fabrication, vacuous Lean theorems, silent `sorry`/`native_decide` leakage, stale metrics, or overclaimed scope), AUTOPHAGE enforces **eight concrete, machine-checked guardrails**:

1. **Six-Kernel Lean 4.35.0 Verification & Adversarial Suite (`lean/`):** All 56 declarations (`48` core/non-vacuity theorems incl. upgraded multi-step `T30` + `8` `AdversarialTests.lean` refutations `AT01`–`AT08`) compile with `0` `sorry` in `AutophageProofs`/`AdversarialTests`/`Solution`, `0` `native_decide`, `0` `bv_decide`, `autoImplicit = false`, checked locally on Windows by the Lean reference kernel (`lean4`, `leanchecker`, `leanchecker-paranoid`) + 3 independent Rust checkers (`nanoda_bin`, `con-leche`, `con-ron`) and in Ubuntu CI by all 6 Palomar checkers (`+ lean4lean` + `bubblewrap` `lake comparator --paranoid`) plus `5/5` live negative elaboration & tamper rejections (`scripts/test_lean_negative.py`).
2. **README & Blueprint Verbatim Axiom Parity (`scripts/check_readme_axioms.py`, `scripts/check_blueprint.py`):** Fails CI if `README.md` or `docs/blueprint.html` drifts from `results/lean4-audit.txt`.
3. **Live Bare-Metal Falsification + 6 Planted Mutants + B07 ASan Regression (`test/test_falsification.c`):** All `19/19` gates and `6/6` planted mutants (`M1`–`M6`) run live across normal and ASan/UBSan builds with `0`-bit JIT == Scalar SHA-256 parity across Win64 and Linux.
4. **Independent Python Property Verification (`scripts/prop_check.py`) & libFuzzer (`test/fuzz_jit_libfuzzer.c`):** Cross-validates core algebraic laws independently of both C11 and Lean 4.
5. **Live Crossref/DataCite Citation & Retraction Audit (`scripts/check_citations.py` → `results/citation-audit.json`):** Verifies `17 / 17` BibTeX DOIs and URLs live for HTTP reachability, title token overlap, author surname alignment, publication year parity, and zero retractions.
6. **Zero-Overflow Clean-Room AMS-LaTeX Build (`scripts/build_paper.py`):** Fails on any `Overfull \hbox`, undefined reference/citation, or missing character, and clean-room compiles `dist/arxiv_submission.tar.gz` in a fresh temp directory.
7. **Cryptographic Timestamps & Provenance (`scripts/stamp_timestamps.py`, `.github/workflows/ci.yml`):** Anchors `CHECKSUMS.sha256` with DigiCert RFC 3161 (`.tsr`) and 3-calendar Bitcoin OpenTimestamps (`.ots`), SLSA/Rekor attestations, and Software Heritage (`swh:1:snp:6ea23803655df1f9679deb98d44efbf53321a393`).
8. **Honest Non-Retirement & Non-Peer-Review Disclosure:** Explicitly states `0 / 1,900` Mega-Atlas open problems retired, `self-assessed` (not peer-reviewed) status, and no fabricated DOIs or institutional affiliations.
