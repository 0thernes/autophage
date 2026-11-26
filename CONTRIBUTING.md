# Contributing to AUTOPHAGE

Thank you for your interest in auditing, falsifying, replicating, or extending **AUTOPHAGE**.

## How to Report Findings or Replications

1. **Bug, mathematical error, or verification gap:** Open a GitHub Issue using the **Bug or finding** form (`.github/ISSUE_TEMPLATE/bug.yml`) and select one of the 10 finding classes (`bug`, `error`, `issue`, `problem`, `concern`, `dilemma`, `conflict`, `gap`, `hole`, `blindspot`).
2. **Independent replication:** Follow [`REPLICATION.md`](REPLICATION.md) and open an issue with the `replication` label reporting your CPU model, OS/compiler versions, `18/18` gate timings, `bench` SHA-256 digest, and multi-kernel Lean 4.35.0 output.
3. **Security vulnerabilities:** Never disclose publicly; follow [`SECURITY.md`](SECURITY.md).

## Engineering & Epistemic Standards

- **Zero runtime dependencies:** Do not introduce external libraries into `src/`, `include/`, `web/`, or `lean/` (no Mathlib in `lean/`).
- **Zero compiler warnings:** All C11 code must build cleanly with `-Wall -Wextra -Werror`.
- **Lean 4.35.0 purity:** All proof modules (`AutophageProofs.lean`, `Solution.lean`) must have `0` `sorry`, `0` `admit`, `0` `native_decide`, `0` `bv_decide`, `0` custom axioms, and axioms strictly within `{propext, Classical.choice, Quot.sound}`. Every new theorem must also be declared in `Challenge.lean` and `comparator.json`, audited in `VerifyAxioms.lean`, and paired with a fidelity row in `docs/statement-fidelity.md`.
- **Licensing:** Code contributions are accepted under Apache-2.0 (`LICENSE`); documentation and paper contributions under CC-BY-4.0 (`LICENSE-DOCS`).
- **Maintainer workflow:** This repository commits directly to `main` (see `AGENTS.md`, R1–R2). Discussions and issue diffs are always welcome.
