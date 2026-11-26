# AUTOPHAGE v1.4.0 — Configuration & Environment Reference

AUTOPHAGE requires **zero configuration files and zero secrets** to build or run. All optional overrides have safe deterministic defaults (see [`.env.example`](../.env.example)).

## 1. Environment Variables

| Variable | Default | Scope | Description |
|---|---|---|---|
| `AUTOPHAGE_PORT` | `8787` | `web/server.mjs` | Local loopback port (`127.0.0.1`) for the Silicon Observatory HTTP/SSE UI. |
| `AUTOPHAGE_CI_PORTABLE` | `0` | `build_linux.sh` | Set to `1` on Linux x86-64 CPUs lacking hardware GFNI/VPCLMULQDQ to compile with runtime CPUID scalar trampoline fallback. |
| `AUTOPHAGE_SANITIZE` | `0` | `build_linux.sh` | Set to `1` to enable `-g -fsanitize=address,undefined -fno-omit-frame-pointer`. |
| `TECTONIC_BIN` | auto-detected | `scripts/build_paper.py` | Optional explicit path to `tectonic` / `tectonic.exe`. |

## 2. Native CLI Subcommands (`bin/autophage.exe` / `bin/autophage_linux`)

- `probe` — Runs CPUID feature detection and live RWX JIT smoke execution.
- `bench [--epochs N] [--size S]` — Runs `NATIVE_JIT` vs. `SCALAR_REF` on `S×S` grid (default `32×32`, `200` epochs, seed `5055`) and checks `0`-bit SHA-256 parity.
- `abiogenesis [--epochs N] [--seed SEED] [--size S]` — Zero-seed pure random primordial soup (default `300` epochs, seed `5000`).
- `feral [--epochs N] [--seed SEED] [--size S]` — Paired Track B (Feral) vs. Track A (Constitutional) comparison with apex x86-64 disassembly and 112-trait projection.
- `ablation [--epochs N] [--seed SEED] [--out PATH]` — 30-condition 13-vector Omega Castration Lab & Iron Law curve with CSV export.
- `vivisection [--seed SEED] [--epochs N]` — Planarian 25/50/75% Strand-$\alpha$ excision, 32-organism cross-substrate transplant, and 90% mass extinction recovery.
- `stats [--pairs N] [--epochs E]` — Locked paired sign-flip permutation statistical suite (`N=24`, seeds `5000..5023`, `10,000` sign-flip permutations).
- `stream [--size S] [--seed SEED] [--regime R]` — Line-delimited NDJSON stdin/stdout protocol consumed by `web/server.mjs`.

## 3. Public C11 `AutophageConfig` Struct (`include/autophage_abi.h`)

Call `autophage_default_config(&cfg)` to populate safe defaults: `width=32`, `height=32`, `threads=1` (strictly canonical deterministic), `regime=AUTOPHAGE_REGIME_FERAL_SEEDED`, `kernel_mode=AUTOPHAGE_KERNEL_NATIVE_JIT`, `base_gas=96`, `seed=5000`, `restraints[0..12]=0`.
