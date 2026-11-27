#!/usr/bin/env bash
set -euo pipefail

PROJ="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$PROJ/bin" "$PROJ/results"

CC="${CC:-gcc}"
if [[ "${AUTOPHAGE_CI_PORTABLE:-0}" == "1" ]]; then
  FLAGS=(-O3 -Wall -Wextra -Werror -Wno-unused-parameter -march=x86-64 -ffp-contract=off -DAUTOPHAGE_CI_PORTABLE -I"$PROJ/include")
else
  FLAGS=(-O3 -Wall -Wextra -Werror -Wno-unused-parameter -march=native -ffp-contract=off -mgfni -mvpclmulqdq -mpclmul -mbmi -mbmi2 -mpopcnt -mavx2 -I"$PROJ/include")
fi

if [[ "${AUTOPHAGE_SANITIZE:-0}" == "1" ]]; then
  # Disable AVX-512 codegen under ASan to avoid GCC PR101000: ASan FakeStack
  # places local 64B arrays at +0x20 (32B-aligned) inside 64B-aligned frames,
  # causing 64B-aligned vmovdqa64 stores to #GP (SIGSEGV) on AVX-512 hosts.
  FLAGS+=(-g -fsanitize=address,undefined -fno-omit-frame-pointer -mno-avx512f)
fi

CORE=(
  "$PROJ/src/hw_isa.c"
  "$PROJ/src/galois_math.c"
  "$PROJ/src/jit_x86_64.c"
  "$PROJ/src/soup_engine.c"
  "$PROJ/src/metrology_ovbs.c"
  "$PROJ/src/abi_exports.c"
)

echo "[1/4] Building Linux ELF64 bin/autophage_linux ..."
"$CC" "${FLAGS[@]}" "${CORE[@]}" "$PROJ/src/main_cli.c" -o "$PROJ/bin/autophage_linux" -lm

echo "[2/4] Building Linux shared library bin/libautophage.so ..."
"$CC" "${FLAGS[@]}" -fPIC -shared "${CORE[@]}" -o "$PROJ/bin/libautophage.so" -lm

echo "[3/4] Building Linux static library bin/libautophage_linux.a ..."
OBJS=()
for s in "${CORE[@]}"; do
  o="$PROJ/bin/$(basename "$s" .c)_linux.o"
  "$CC" -c -fPIC "${FLAGS[@]}" "$s" -o "$o"
  OBJS+=("$o")
done
ar rcs "$PROJ/bin/libautophage_linux.a" "${OBJS[@]}"
rm -f "${OBJS[@]}"

echo "[4/4] Building & running Linux ELF64 19-gate falsification suite (18 core + B07 regression) ..."
"$CC" "${FLAGS[@]}" "${CORE[@]}" "$PROJ/test/test_falsification.c" -o "$PROJ/bin/test_falsification_linux" -lm

(
  cd "$PROJ/bin"
  ./test_falsification_linux
  rm -f test_tmp_snapshot.apbin
)

if command -v lake >/dev/null 2>&1; then
  echo "[5/5] Verifying 56 Pure-Kernel Lean 4.35.0 proofs (Palomar Challenge/Solution + AdversarialTests + strict axiom allowlist) ..."
  LOG="$(mktemp)"
  trap 'rm -f "$LOG"' EXIT
  (
    cd "$PROJ/lean"
    lake build
    lake build Challenge
    lake env lean VerifyAxioms.lean 2>&1 | tee "$LOG"
  )
  python3 - "$LOG" <<'PY'
import re, sys
allowed = {"propext", "Classical.choice", "Quot.sound"}
forbidden = re.compile(r"(warning|error|sorryAx|declaration uses 'sorry'|bv_decide|native_decide|ofReduceBool|trustCompiler)", re.I)
audited = 0
with open(sys.argv[1], "r", encoding="utf-8") as f:
    for raw in f:
        line = raw.strip()
        if not line:
            continue
        if forbidden.search(line):
            sys.exit(f"Lean integrity violation: {line}")
        m0 = re.match(r"^'([^']+)' does not depend on any axioms$", line)
        if m0:
            audited += 1
            continue
        m1 = re.match(r"^'([^']+)' depends on axioms: \[([^\]]*)\]$", line)
        if m1:
            axs = [a.strip() for a in m1.group(2).split(",") if a.strip()]
            bad = [a for a in axs if a not in allowed]
            if bad:
                sys.exit(f"Forbidden axioms {bad} on {m1.group(1)}")
            audited += 1
            continue
        sys.exit(f"Unexpected Lean output line: {line}")
if audited < 56:
    sys.exit(f"Expected >= 56 audited theorems, found {audited}")
print(f"PASS: {audited} pure-kernel Lean 4.35.0 theorems verified strictly within {sorted(allowed)}.")
PY
fi

echo "AUTOPHAGE v1.5.0 LINUX ELF64 BUILD & VERIFICATION PASSED!"
