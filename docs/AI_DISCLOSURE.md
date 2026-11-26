# AI-Assistance Transparency & CRediT Authorship Disclosure (`v1.4.0`)

**Principal Author & Sole Responsible Human-in-the-Loop:** Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA (`ORCID 0009-0006-8168-9648`, `0_0@0thernes.art`, `https://github.com/0thernes`).

## 1. Transparent Disclosure of Human + AI-Assisted Research Engineering

AUTOPHAGE is an independent solo research software and formal-verification project developed with frontier AI coding and proof-engineering assistance (`Gemini / Antigravity` and `Claude`). Rather than concealing AI tooling or misrepresenting AI review as institutional peer review, this repository discloses exact contributions under the CRediT taxonomy:

- **Human Author Role (Alexander Stephen Donahue — sole author):** Conceptualization, Methodology, Investigation, Supervision, Project Administration, Software Architecture Direction, Epistemic Verification Policy, Visual/Typesetting Sign-Off, and Sole Scholarly Accountability.
- **Disclosed AI Tooling Role (Non-Author Assistants):** Assisted drafting of C11 engine/JIT/falsification modules, Lean 4.35.0 tactic proof refactoring and Palomar `Spec`/`Challenge`/`Solution` scaffolding, Python property/citation/build scripts, Build Standard v1.2.0 documentation, and AMS-LaTeX typesetting under human direction and machine verification.

## 2. Hard Anti-Hallucination & Integrity Guardrails

Because AI-assisted scientific engineering carries documented failure modes (citation fabrication, vacuous Lean theorems, silent `sorry`/`native_decide` leakage, stale metrics, or overclaimed scope), AUTOPHAGE enforces **seven concrete, machine-checked guardrails**:

1. **Six-Kernel Lean 4.35.0 Verification (`lean/`):** All 48 theorems compile with `0` `sorry` in `AutophageProofs`/`Solution`, `0` `native_decide`, `0` `bv_decide`, `autoImplicit = false`, concrete Non-Vacuity & Negative-Boundary proofs (`T43`–`T48`), and independent replay across `lean`, `leanchecker`, `leanchecker-paranoid`, `nanoda_bin` (`unpermitted_axiom_hard_error: true`), `con-leche --verified`, `con-ron --verified`, plus `lean4lean` + `lake comparator --paranoid` on Ubuntu CI.
2. **README Verbatim Axiom Parity (`scripts/check_readme_axioms.py`):** Fails CI if `README.md` drifts by even one character from `results/lean4-audit.txt`.
3. **Live Bare-Metal Falsification + 6 Planted Negative Mutants (`test/test_falsification.c`):** Every empirical number is generated live from compiled C11 binaries (`0`-bit JIT == Scalar SHA-256 parity across Win64 and Linux) with explicit planted faults (`M1`–`M6`) required to be caught.
4. **Independent Python Property Verification (`scripts/prop_check.py`) & libFuzzer (`test/fuzz_jit_libfuzzer.c`):** Cross-validates core algebraic laws independently of both C11 and Lean 4.
5. **Live Citation & DOI Verification (`scripts/check_citations.py`):** Verifies `17 / 17` BibTeX URLs and DOIs live via HTTP & DOI Handle REST API.
6. **Zero-Overflow Clean-Room AMS-LaTeX Build (`scripts/build_paper.py`):** Fails on any `Overfull \hbox`, undefined reference/citation, or missing character, and clean-room compiles `dist/arxiv_submission.tar.gz` in a fresh temp directory.
7. **Honest Non-Retirement & Non-Peer-Review Disclosure:** Explicitly states `0 / 1,900` Mega-Atlas open problems retired, `self-assessed` (not peer-reviewed) status, and no fabricated DOIs or institutional affiliations.
