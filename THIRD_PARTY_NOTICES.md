# Third-Party Notices & Tooling Attribution

AUTOPHAGE (`v1.4.0`) ships with **zero runtime third-party source dependencies** in its C11 engine (`src/`, `include/`), standalone web observatory (`web/`), and pure Lean 4.35.0 proof suite (`lean/`, zero Mathlib).

The following external toolchains, algorithms, and standards are used at build, verification, or typesetting time, or referenced as published algorithms:

| Component / Standard | Role | License / Source |
|---|---|---|
| **Lean 4 (`v4.35.0`) & Palomar Checkers** (`lean`, `lake`, `leanchecker`, `leanchecker-paranoid`, `lean4lean`, `nanoda_bin`, `con-leche`, `con-ron`) | Pure-kernel formal proof verification (`lean/`) | Apache-2.0 (`https://github.com/leanprover/lean4`) |
| **LLVM / Clang (`clang`, `lld`, `llvm-ar`)** & **GCC** | Native C11 x86-64 compilation (`build.ps1`, `build_linux.sh`, CI) | Apache-2.0 with LLVM Exception / GPL-3.0-or-later with GCC Runtime Exception |
| **Node.js (`node:http`, `node:child_process`)** | Zero-dependency loopback Observatory bridge (`web/server.mjs`) | MIT (`https://nodejs.org`) |
| **Tectonic (`tectonic`)** | Self-contained AMS-LaTeX white-paper typesetting (`scripts/build_paper.py`) | MIT (`https://tectonic-typesetting.github.io`) |
| **Philox4x32-10** (Salmon et al., SC 2011) | Counter-based PRNG algorithm implemented from scratch in C11 (`src/hw_isa.c`) | Published algorithm (`DOI: 10.1145/2063384.2063405`) |
| **NIST FIPS 180-4 SHA-256** | Cryptographic state hashing implemented from scratch in C11 (`src/hw_isa.c`) | Public Domain (NIST FIPS PUB 180-4) |
| **Build Standard for LLMs (`v1.2.0`)** | Repository audit scripts (`scripts/Audit-Repo.ps1`, `scripts/audit-repo.sh`, `scripts/audit-checks.tsv`, `repo-standard.schema.json`) | Apache-2.0 / Build Standard template |
| **Contributor Covenant v2.1** | Community standards (`CODE_OF_CONDUCT.md`) | CC-BY-4.0 (`https://www.contributor-covenant.org`) |
