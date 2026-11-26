# AUTOPHAGE v1.4.0 — Operations, Snapshots, Backups & Offsite Mirroring

## 1. Local Build, Verification & Observatory Operations

- **Windows native build + 18 gates + 48 Lean proofs:** `pwsh -NoProfile -ExecutionPolicy Bypass -File build.ps1`
- **Windows multi-kernel Lean audit:** `pwsh -NoProfile -ExecutionPolicy Bypass -File scripts/verify_lean_multikernel.ps1`
- **Linux/WSL2 native build + ASan/UBSan:** `AUTOPHAGE_SANITIZE=1 AUTOPHAGE_CI_PORTABLE=1 bash build_linux.sh`
- **Loopback Observatory server:** `node web/server.mjs` (binds strictly to `127.0.0.1:8787`; health probe `GET http://127.0.0.1:8787/healthz`; full state `GET /api/state`; SSE stream `GET /api/events`; structured stderr logging; graceful `SIGINT`/`SIGTERM` shutdown).

## 2. Canonical `.apbin` v2 State Snapshots

- `autophage_save_snapshot(world, path)` writes a deterministic binary `.apbin` v2 file (`"APBN0002"`) containing complete world state, 5-pool ledger accumulators, detritus grid, cells, and a trailing 32-byte SHA-256 integrity digest.
- `autophage_load_snapshot(path, kernel_mode)` validates header magic, dimensions, and full-payload SHA-256 digest before instantiating the world; any tampered byte causes fail-closed `NULL` return (`Gate T11`).
- Snapshots are strictly cross-kernel portable: a snapshot saved under `NATIVE_JIT` resumes bit-exact under `SCALAR_REF` (`0`-bit diff after 25 additional epochs, `Gate T11`).

## 3. Repository Backups, Git Bundles & Offsite Mirroring (`A35`)

To prevent single-point-of-failure data loss, AUTOPHAGE maintains a three-tier backup and offsite mirror discipline:

1. **Primary Working Drive (`Z:\` ReFS Dev Drive):** Canonical active repository at `Z:\[Vibe Coded (AI)]\ANTIGRAVITY\autophage`.
2. **Primary Offsite Git Remote & Software Heritage Mirror:** Every release and verification commit is pushed to GitHub (`https://github.com/0thernes/autophage`) and registered in the master archive (`0thernes/0thernes-research-archive`) with frozen SHA-256 digests and Software Heritage (`save.softwareheritage.org`) archival snapshots.
3. **Offline Self-Contained Git Bundle Backup:** Create an air-gapped single-file repository backup bundle (recoverable with zero network access via `git clone autophage-v1.4.0.bundle`) at any time:
   ```bash
   git bundle create ../autophage-v1.4.0.bundle --all
   git bundle verify ../autophage-v1.4.0.bundle
   ```
4. **Checksum Verification:** Verify frozen release artifacts, white paper PDFs, arXiv tarball, and empirical datasets against [`CHECKSUMS.sha256`](../CHECKSUMS.sha256) and [`data/MANIFEST.sha256`](../data/MANIFEST.sha256).
