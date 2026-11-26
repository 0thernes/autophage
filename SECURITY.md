# Security Policy

## Supported Versions

| Version | Supported |
|---|---|
| `1.4.x` (`main`) | Active |
| `< 1.4.0` | Unsupported (upgrade to `1.4.0`) |

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for security vulnerabilities. Report privately via:

1. GitHub Private Security Advisory: `https://github.com/0thernes/autophage/security/advisories/new`
2. Direct email to the maintainer: `0_0@0thernes.art` (Alexander Stephen Donahue)

Include reproduction steps, target OS/CPU, compiler flags, and proof-of-concept input (e.g., malformed `.apbin` snapshot or HTTP request against `web/server.mjs`).

## Security Boundary Summary

AUTOPHAGE allocates an executable `16 MiB` RWX JIT arena (`VirtualAlloc` / `mmap`) for internally generated x86-64 organism code and ships an optional local-only HTTP/SSE Observatory (`web/server.mjs`). Key enforced mitigations (detailed in [`docs/threat-model.md`](docs/threat-model.md)):

- **JIT Memory & Halting Sandbox:** Every emitted tape byte offset is masked with `and reg32, 0x7F` (`< 128`, Lean `T26`), every 64-bit qword offset with `and reg32, 0x78` (`off + 8 <= 128`, Lean `T27`), and every backward jump strictly decrements hardware gas `r15` (Lean `T28`). Verified across `1,000,000` live RWX fuzz executions with 64-byte guard canaries (`Gate T05`).
- **Snapshot Integrity:** `.apbin` v2 snapshots carry a full-payload SHA-256 digest checked on load; tampered snapshots are rejected (`NULL`, `Gate T11`).
- **Local Web Observatory Hardening (`web/server.mjs`):** Binds strictly to `127.0.0.1`, enforces loopback `Host`/`Origin`/`Sec-Fetch-Site` and `application/json` `Content-Type` on `POST /api/cmd`, caps request body size (`64 KiB`) and SSE clients (`16`), and serves only an explicit static allowlist (`index.html`, `app.js`, `styles.css`). Do **not** expose `web/server.mjs` to untrusted networks without an authenticating reverse proxy.
