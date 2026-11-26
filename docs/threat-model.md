# AUTOPHAGE v1.4.0 — Threat Model (STRIDE)

AUTOPHAGE executes dynamically generated x86-64 machine code in an RWX arena and provides an optional local-only HTTP/SSE web observatory (`web/server.mjs`). This threat model documents assets, trust boundaries, STRIDE risks, and concrete verified mitigations.

## 1. Assets & Trust Boundaries

1. **Host Process Memory Safety (RWX JIT Arena):** Untrusted/evolved 64-byte organism genomes compile into native x86-64 code running inside the process address space.
2. **Canonical Simulation Integrity (`.apbin` v2 & SHA-256):** Deterministic state snapshots and scientific telemetry must resist accidental corruption or tampering.
3. **Localhost Control Plane (`web/server.mjs`):** Bridges browser UI commands (`POST /api/cmd`) to native binary `stdin` over loopback (`127.0.0.1:8787`).

## 2. STRIDE Matrix & Verified Mitigations

| Category | Threat Scenario | Mitigation Implemented | Verification Evidence |
|---|---|---|---|
| **Spoofing / CSRF** | Malicious web page issues cross-origin `POST /api/cmd` to `127.0.0.1:8787` (DNS rebinding or CSRF). | `web/server.mjs` binds strictly to `127.0.0.1`, enforces loopback `Host` header regex, checks `Origin` and `Sec-Fetch-Site` (`same-origin` / `none`), and requires `Content-Type: application/json`. | `web/server.mjs` (`isLoopbackHost`, `isAuthorizedPost`) |
| **Tampering** | Corrupted or maliciously modified `.apbin` snapshot causes OOB allocation or silent state corruption on load. | `.apbin` v2 verifies header magic, power-of-two dimension bounds, and trailing full-payload NIST SHA-256 digest; returns `NULL` on any 1-byte flip. | `test/test_falsification.c` Gate `T11` (tamper test) |
| **Tampering (Memory OOB)** | Evolved organism genome accesses memory outside its 128-byte `tape_a` / `tape_b` buffers. | Every emitted byte access masks index `& 0x7F` (`< 128`, Lean `T26`); every qword access masks `& 0x78` (`off + 8 <= 128`, Lean `T27`, necessity proved in `T48`); zero arbitrary pointer arithmetic or syscalls in the 32-opcode ISA. | Lean `T26`, `T27`, `T48` (mask arithmetic only; emission is not proved in Lean); Gate `T05` (`1,000,000` fuzz execs + 64B guard canaries); `test/fuzz_jit_libfuzzer.c` |
| **Information Disclosure** | Path traversal or static file request leaks `web/server.mjs` source or arbitrary host files. | `ALLOWED_STATIC` strict allowlist (`index.html`, `app.js`, `styles.css`); all other paths return `404`. | `web/server.mjs:157,341-346` |
| **Denial of Service (Infinite Loop)** | Evolved backward loop hangs CPU indefinitely. | Every backward branch emits `sub r15, 1; jle +skip` against finite `base_gas` (`T28`). | Lean `T28` (the lemma `g - 1 < g`; emission is not proved in Lean); Gate `T04` & `T05` |
| **Denial of Service (HTTP / SSE)** | Unbounded `POST /api/cmd` body or SSE client flood exhausts memory. | `MAX_CMD_BODY_BYTES = 65536` (`413`), `MAX_SSE_CLIENTS = 16` (`429`), overlong `stdin` line draining in `main_cli.c`, strict numeric clamping (`parseFiniteInt`, `parseUnsignedSeed64`). | `web/server.mjs:15-16,210-265`; `src/main_cli.c:734-738` |
| **Elevation of Privilege** | JIT code executes arbitrary OS syscalls or leaks `W^X` surface. | No `syscall`/`int 0x80`/arbitrary `call` opcode exists in the 32-opcode ISA (only fixed internal CPUID fallback trampoline addresses when GFNI is absent). Known residual risk: arena uses RWX rather than `W^X` toggle for zero-syscall single-pass speed — **never run AUTOPHAGE as a privileged multi-tenant service or expose `web/server.mjs` to untrusted networks.** | `src/jit_x86_64.c`, `SECURITY.md` |
