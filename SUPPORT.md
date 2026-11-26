# Support

## Getting Help

1. **Documentation:** Start with [`README.md`](README.md), [`docs/index.md`](docs/index.md), [`docs/faq.md`](docs/faq.md), and [`docs/glossary.md`](docs/glossary.md).
2. **Replication & Troubleshooting:** Check [`REPLICATION.md`](REPLICATION.md) and [`docs/operations.md`](docs/operations.md) for Windows Clang, Linux GCC, WSL2, CPUID fallback (`AUTOPHAGE_CI_PORTABLE=1`), Lean 4.35.0, and Tectonic troubleshooting.
3. **Questions:** Open a GitHub Issue using `.github/ISSUE_TEMPLATE/question.yml` or GitHub Discussions (`https://github.com/0thernes/autophage/discussions`).
4. **Bugs & Audit Findings:** Use `.github/ISSUE_TEMPLATE/bug.yml` and tag the appropriate finding class (`bug`..`blindspot`).
5. **Direct Maintainer Contact:** Alexander Stephen Donahue (`0_0@0thernes.art`, ORCID `0009-0006-8168-9648`).

## Scope of Support

AUTOPHAGE is maintained on a best-effort independent research basis. Hardware silicon benchmarks target x86-64 CPUs with AVX2/GFNI/PCLMULQDQ/BMI2/POPCNT; older x86-64 CPUs are supported via the automatic runtime CPUID scalar fallback (`AUTOPHAGE_CI_PORTABLE=1`).
