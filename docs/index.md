# AUTOPHAGE v1.4.0 — Documentation Index

Welcome to the documentation hub for **AUTOPHAGE v1.4.0** (Bare-Metal Polymorphic x86-64 Galois-Field Genetic Engine).

## Core Reference
- [Architecture (`docs/architecture.md`)](architecture.md) — Cache-line structs, Dual-ABI x86-64 RWX JIT, 32-opcode table, Galois algebra, Cauchy MDS ECC, 5-pool ledger (also synced with root [`ARCHITECTURE.md`](../ARCHITECTURE.md)).
- [Verification Ledger (`docs/verification.md`)](verification.md) — 48 Pure Lean 4.35.0 proofs, 6-kernel Palomar checks, 18 falsification gates, 6 planted mutants (also synced with root [`VERIFICATION.md`](../VERIFICATION.md)).
- [Benchmarks & Performance Budgets (`docs/benchmarks.md`)](benchmarks.md) — Measured Win64 PE & Linux ELF64 throughputs, cache-hit tradeoff analysis, and explicit regression budgets.
- [Configuration & Environment (`docs/configuration.md`)](configuration.md) — Compile flags, CLI arguments, environment variables, and C11 `AutophageConfig` parameters.
- [Operations, Backups & Mirroring (`docs/operations.md`)](operations.md) — Build workflows, Observatory lifecycle, `.apbin` snapshot backups, and offsite mirror/bundle protocol.

## Formal Audit, Epistemic Integrity & Governance
- [Statement-Fidelity Audit Matrix (`docs/statement-fidelity.md`)](statement-fidelity.md) — Row-by-row correspondence between all 48 Lean 4.35.0 theorems, C11 source, and white paper.
- [Mega-Atlas Non-Retirement Cross-Check (`docs/MEGA_ATLAS_CROSSCHECK.md`)](MEGA_ATLAS_CROSSCHECK.md) — Explicit `0 / 1,900` non-retirement accounting for `AI-045`, `BIO-001`, `CHEM-052`, and `SYS-009`.
- [AI-Assistance Transparency Disclosure (`docs/AI_DISCLOSURE.md`)](AI_DISCLOSURE.md) — Human sole-author CRediT role, disclosed AI tooling, and anti-hallucination guardrails.
- [Experimental Preregistration (`docs/preregistration.md`)](preregistration.md) — Locked seed firewall (`5000..5023`), statistical tests, and Holm-Bonferroni correction plan.
- [Threat Model (`docs/threat-model.md`)](threat-model.md) — STRIDE analysis covering RWX JIT isolation, `.apbin` snapshots, and loopback Web Observatory hardening.
- [10-Pass Red-Team Audit (`docs/reviews/2026-11-25-10-pass-red-team-audit.md`)](reviews/2026-11-25-10-pass-red-team-audit.md) — Complete Playbook v1.3 10-pass review & 3-cycle hardening record.
- [Architecture Decision Records (`docs/adr/README.md`)](adr/README.md) — Recorded architectural tradeoffs (`ADR-0001`).
- [Run Log (`docs/run-log.md`)](run-log.md) — Chronological verification and release execution audit trail.
- [FAQ (`docs/faq.md`)](faq.md) & [Glossary (`docs/glossary.md`)](glossary.md) — Frequently asked questions and precise definitions (including the 10 finding classes).
