# AUTOPHAGE v1.5.0 — One-Word Task Runner (Build Standard v1.2.0)
set shell := ["bash", "-cu"]
set windows-shell := ["pwsh", "-NoProfile", "-Command"]

# Full clean-clone reproduction: native C11 build, 19/19 gates, 6/6 mutants, prop_check, 56 Lean proofs, negative tests, blueprint, timestamps, paper & audit
[windows]
reproduce:
    pwsh -NoProfile -ExecutionPolicy Bypass -File build.ps1
    pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/verify_lean_multikernel.ps1
    python scripts/test_lean_negative.py
    python scripts/prop_check.py
    python scripts/check_readme_axioms.py
    python scripts/build_blueprint.py
    python scripts/check_blueprint.py
    python scripts/stamp_timestamps.py --verify
    python scripts/check_citations.py
    python scripts/build_paper.py
    pwsh -NoProfile -File scripts/Audit-Repo.ps1

[unix]
reproduce:
    bash build_linux.sh
    python3 scripts/test_lean_negative.py
    python3 scripts/prop_check.py
    python3 scripts/check_readme_axioms.py
    python3 scripts/build_blueprint.py
    python3 scripts/check_blueprint.py
    python3 scripts/stamp_timestamps.py --verify
    python3 scripts/check_citations.py
    python3 scripts/build_paper.py
    bash scripts/audit-repo.sh .

# Native C11 build + 19-gate bare-metal falsification suite
[windows]
build:
    pwsh -NoProfile -ExecutionPolicy Bypass -File build.ps1 -SkipLean

[unix]
build:
    bash build_linux.sh

# Run falsification suite + Python property-based checks + blueprint & timestamp checks
[windows]
test:
    pwsh -NoProfile -ExecutionPolicy Bypass -File build.ps1 -SkipLean
    python scripts/prop_check.py
    python scripts/check_readme_axioms.py
    python scripts/check_blueprint.py
    python scripts/stamp_timestamps.py --verify

[unix]
test:
    bash build_linux.sh
    python3 scripts/prop_check.py
    python3 scripts/check_readme_axioms.py
    python3 scripts/check_blueprint.py
    python3 scripts/stamp_timestamps.py --verify

# Linux AddressSanitizer + UndefinedBehaviorSanitizer build & test (B16 + B07 Gate T19)
[unix]
sanitize:
    AUTOPHAGE_SANITIZE=1 AUTOPHAGE_CI_PORTABLE=1 bash build_linux.sh

# Build & run libFuzzer differential JIT vs scalar oracle harness (B15, requires Linux clang)
[unix]
fuzz seconds="30":
    mkdir -p bin
    clang -O1 -g -fsanitize=fuzzer,address,undefined -DAUTOPHAGE_CI_PORTABLE -Iinclude src/hw_isa.c src/galois_math.c src/jit_x86_64.c src/soup_engine.c src/metrology_ovbs.c src/abi_exports.c test/fuzz_jit_libfuzzer.c -o bin/fuzz_jit_libfuzzer -lm
    ./bin/fuzz_jit_libfuzzer -max_total_time={{seconds}}

# Report-only static analysis & syntax linting (B17)
[unix]
lint:
    node --check web/server.mjs
    node --check web/app.js
    if command -v cppcheck >/dev/null 2>&1; then cppcheck --enable=warning,style --suppress=missingIncludeSystem -Iinclude src/ test/; fi
    if command -v ruff >/dev/null 2>&1; then ruff check scripts/; fi

[windows]
lint:
    node --check web/server.mjs
    node --check web/app.js
    if (Get-Command ruff -ErrorAction SilentlyContinue) { ruff check scripts/ }

# Pure Lean 4.35.0 build, multi-kernel Palomar verification & negative tamper tests
[windows]
lean:
    pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/verify_lean_multikernel.ps1
    python scripts/test_lean_negative.py

[unix]
lean:
    cd lean && lake build && lake build Challenge && lake env lean VerifyAxioms.lean > ../results/lean4-audit.txt && lake check --paranoid && lake comparator --config comparator.json --paranoid
    python3 scripts/test_lean_negative.py

# Build & verify interactive Lean 4.35.0 Blueprint & GitHub Pages portal
blueprint:
    python3 scripts/build_blueprint.py
    python3 scripts/check_blueprint.py

# Compile AMS-LaTeX white paper + live Crossref/DataCite citation audit + clean-room arXiv tarball verification
paper:
    python3 scripts/check_citations.py
    python3 scripts/build_paper.py

# Build Standard v1.2.0 self-audit (report-only)
[windows]
audit:
    pwsh -NoProfile -File scripts/Audit-Repo.ps1

[unix]
audit:
    bash scripts/audit-repo.sh .

# Launch local loopback Silicon Observatory UI (http://127.0.0.1:8787)
web:
    node web/server.mjs
