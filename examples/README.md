# AUTOPHAGE v1.4.0 Examples

Runnable examples demonstrating standalone CLI execution, dual-kernel SHA-256 parity verification, and C11 ABI integration.

## Available Examples

1. **[`run_quick_experiment.py`](run_quick_experiment.py)** — Zero-dependency Python wrapper that invokes the compiled `autophage` native binary (`bin/autophage.exe` on Windows or `bin/autophage_linux` on Linux), verifies live silicon capabilities (`probe`), confirms `0`-bit diff JIT vs. Scalar SHA-256 parity (`bench`), and runs a short deterministic zero-seed abiogenesis trajectory (`abiogenesis --epochs 60 --seed 5000 --size 16`).

```bash
# First build the native binary (`pwsh -File build.ps1 -SkipLean` or `bash build_linux.sh`)
python3 examples/run_quick_experiment.py
```

2. **Direct C11 ABI Embedding:** See the minimal C11 embedding snippet in [`../README.md`](../README.md) linking against `bin/autophage.dll` (`libautophage.dll.a`), `bin/libautophage.a`, or `bin/libautophage.so` with `include/autophage_abi.h`.
