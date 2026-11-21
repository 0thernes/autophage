#!/usr/bin/env bash
set -euo pipefail

PROJ="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$PROJ/bin" "$PROJ/results"

CC="${CC:-gcc}"
FLAGS=(-O3 -Wall -Wextra -Wno-unused-parameter -march=native -mgfni -mvpclmulqdq -mpclmul -mbmi -mbmi2 -mpopcnt -mavx2 -I"$PROJ/include")
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

echo "[4/4] Building & running Linux ELF64 18-gate falsification suite ..."
"$CC" "${FLAGS[@]}" "${CORE[@]}" "$PROJ/test/test_falsification.c" -o "$PROJ/bin/test_falsification_linux" -lm

cd "$PROJ/bin"
./test_falsification_linux
rm -f test_tmp_snapshot.apbin
echo "LINUX ELF64 BUILD & 18-GATE VERIFICATION PASSED!"
