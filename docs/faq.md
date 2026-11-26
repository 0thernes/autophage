# Frequently Asked Questions (FAQ)

### 1. Does AUTOPHAGE really execute live native x86-64 machine code?
Yes. `src/jit_x86_64.c` allocates a `16 MiB` executable arena (`VirtualAlloc(..., PAGE_EXECUTE_READWRITE)` on Windows; `mmap(..., PROT_READ|PROT_WRITE|PROT_EXEC)` on Linux), compiles each organism's active codons directly into x86-64 instructions (`VGF2P8AFFINEQB`/`GF2P8AFFINEQB`, `PCLMULQDQ`, `PEXT`, `PDEP`, `POPCNT`, `TZCNT`, `XOR`, `ROL`, `AND`, Toffoli/Fredkin sequences), and calls the emitted function pointer directly. Every memory offset is masked (`& 0x7F` / `& 0x78`) and every backward branch decrements hardware gas (`r15`).

### 2. What if my CPU does not support Intel GFNI (`VGF2P8AFFINEQB`)?
Run Linux builds with `AUTOPHAGE_CI_PORTABLE=1 bash build_linux.sh` (as done in GitHub Actions CI). At runtime, `autophage_probe_hardware()` inspects CPUID and `jit_emit_program()` automatically emits 16-byte-aligned scalar trampoline helpers so the binary runs without `SIGILL` and produces the **exact same 0-bit diff SHA-256 hash**.

### 3. Why is the end-to-end JIT speedup `1.04×–1.30×` when `T05` runs at `15–18 M execs/s`?
Because in a live evolving primordial soup (`bench`), genomes mutate rapidly, yielding a `27.35%` JIT cache hit rate (`126,031` single-pass compiles across `173,484` executions), and torus energy/heat diffusion + mating + metrology dominate wall time. Gate `T05` measures pure hot-loop cached execution (`2,000` genomes × `500` calls). We disclose both numbers transparently in [`docs/benchmarks.md`](benchmarks.md) and [`LIMITATIONS.md`](../LIMITATIONS.md).

### 4. Does AUTOPHAGE solve Origin of Life (`BIO-001`) or Open-Ended Evolution (`AI-045`)?
**No.** Zero (`0 / 1,900`) overarching problems in `0thernes/unsolved-problems-mega-atlas` are solved or retired. See [`docs/MEGA_ATLAS_CROSSCHECK.md`](MEGA_ATLAS_CROSSCHECK.md) and [`LIMITATIONS.md`](../LIMITATIONS.md).

### 5. Why do 5 Lean 4.35.0 kernels run on Windows and 6 on Ubuntu Linux?
On Windows, `lean4lean.exe` cannot parse Windows `.olean` path headers and `lake check` / `lake comparator` require Linux `bubblewrap` (`bwrap`). Locally on Windows we run `lean`, `leanchecker --fresh`, `leanchecker-paranoid --fresh`, `nanoda_bin` (`unpermitted_axiom_hard_error: true`), `con-leche --verified`, and `con-ron --verified` (all PASS); `.github/workflows/ci.yml` executes all `6/6` kernels + `lake check --paranoid` + `lake comparator --config comparator.json --paranoid` on `ubuntu-latest`.

### 6. Why is the author affiliation listed as `Independent Researcher, Spokane, WA, USA`?
No LLC legal entity exists. Under Playbook v1.3 Finding #1, all active files, licenses, citations, white-paper headers, and GitHub metadata are standardized to `Alexander Stephen Donahue — Independent Researcher, Spokane, WA, USA` (ORCID `0009-0006-8168-9648`).
