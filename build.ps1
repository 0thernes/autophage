param(
    [switch]$SkipTests,
    [switch]$SkipLean
)

$ErrorActionPreference = "Stop"
$proj  = Split-Path -Parent $MyInvocation.MyCommand.Path
$root  = "$env:USERPROFILE\.elan\toolchains\leanprover--lean4---v4.35.0"
$clang = "$root\bin\clang.exe"
$lean  = "$root\bin\lean.exe"
$ar    = "$root\bin\llvm-ar.exe"

if (-not (Test-Path -LiteralPath $clang)) {
    throw "Bundled Clang toolchain not found at $clang"
}

[System.IO.Directory]::CreateDirectory("$proj\bin") | Out-Null
[System.IO.Directory]::CreateDirectory("$proj\results") | Out-Null

$core = @(
    "$proj\src\hw_isa.c",
    "$proj\src\galois_math.c",
    "$proj\src\jit_x86_64.c",
    "$proj\src\soup_engine.c",
    "$proj\src\metrology_ovbs.c",
    "$proj\src\abi_exports.c"
)

$cflags = @(
    "-O3", "-Wall", "-Wextra", "-Wno-unused-parameter",
    "-march=native", "-mgfni", "-mvpclmulqdq", "-mpclmul", "-mbmi", "-mbmi2", "-mpopcnt", "-mavx2", "-mavxvnni",
    "-ffp-contract=off",
    "--target=x86_64-w64-windows-gnu",
    "-DAUTOPHAGE_BARE_WIN64",
    "-I$proj\include",
    "-isystem", "$root\include\clang"
)
$lto = @("-flto=thin")
$ldflags = @("-L$root\lib", "-fuse-ld=lld")
$libs = @("-lkernel32", "-lucrtbase", "-lm", "-lmingw32", "-lmingwex")

Write-Host "[1/4] Building bin\autophage.exe (Win64 AVX2/VNNI/GFNI + ThinLTO) ..."
& $clang @cflags @lto @core "$proj\src\main_cli.c" -o "$proj\bin\autophage.exe" @ldflags @libs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[2/4] Building bin\autophage.dll (+ import lib, ThinLTO) ..."
& $clang @cflags @lto -shared -DAUTOPHAGE_BUILD_DLL @core -o "$proj\bin\autophage.dll" "-Wl,--out-implib,$proj\bin\libautophage.dll.a" @ldflags @libs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[3/4] Building bin\test_falsification.exe (ThinLTO) ..."
& $clang @cflags @lto @core "$proj\test\test_falsification.c" -o "$proj\bin\test_falsification.exe" @ldflags @libs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (Test-Path -LiteralPath $ar) {
    Write-Host "[4/4] Building static archive bin\libautophage.a ..."
    $objs = @()
    foreach ($src in $core) {
        $obj = "$proj\bin\" + [System.IO.Path]::GetFileNameWithoutExtension($src) + ".o"
        & $clang -c @cflags $src -o $obj
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $objs += $obj
    }
    & $ar rcs "$proj\bin\libautophage.a" @objs
    Remove-Item -Force -LiteralPath $objs
} else {
    Write-Host "[4/4] llvm-ar not present; skipping static .a archive (DLL + implib built cleanly)."
}

if (-not $SkipTests) {
    Write-Host "`nRunning 18-Gate Bare-Metal Silicon Falsification Suite..."
    Push-Location -LiteralPath "$proj\bin"
    & ".\test_falsification.exe"
    $rc = $LASTEXITCODE
    Remove-Item -Force -LiteralPath "test_tmp_snapshot.apbin" -ErrorAction SilentlyContinue
    Pop-Location
    if ($rc -ne 0) { exit $rc }
}

if (-not $SkipLean -and (Test-Path -LiteralPath $lean)) {
    Write-Host "`nVerifying 42 Pure-Kernel Lean 4 Mathematical Proofs (strict axiom allowlist)..."
    $leanOut = & $lean "$proj\lean\AutophageProofs.lean" 2>&1
    $leanRc = $LASTEXITCODE
    foreach ($line in $leanOut) { Write-Host $line }
    if ($leanRc -ne 0) {
        throw "Lean 4 verification failed with exit code $leanRc"
    }

    $allowed = @("propext", "Classical.choice", "Quot.sound")
    $audited = 0
    $cleanAuditLines = @()
    foreach ($raw in $leanOut) {
        $line = "$raw".Trim()
        if ($line -eq "") { continue }
        if ($line -match "(?i)\b(warning|error)\b|sorryAx|declaration uses 'sorry'|bv_decide|native_decide|ofReduceBool|trustCompiler") {
            throw "Lean 4 kernel integrity violation detected: $line"
        }
        if ($line -match "^'([^']+)' does not depend on any axioms$") {
            $audited++
            $cleanAuditLines += $line
            continue
        }
        if ($line -match "^'([^']+)' depends on axioms: \[([^\]]*)\]$") {
            $thm = $Matches[1]
            $axList = $Matches[2].Split(",") | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
            foreach ($ax in $axList) {
                if ($allowed -notcontains $ax) {
                    throw "Forbidden axiom '$ax' on theorem '$thm'! Allowed: {propext, Classical.choice, Quot.sound}"
                }
            }
            $audited++
            $cleanAuditLines += $line
            continue
        }
        throw "Unexpected Lean 4 output line: $line"
    }
    if ($audited -lt 42) {
        throw "Expected >= 42 audited Lean 4 theorems, found only $audited"
    }
    $auditPath = "$proj\results\lean4-audit.txt"
    [System.IO.File]::WriteAllText($auditPath, ($cleanAuditLines -join "`n") + "`n", [System.Text.UTF8Encoding]::new($false))
    Write-Host "Lean 4 Formal Verification PASSED ($audited pure-kernel theorems, strict allowlist {propext, Classical.choice, Quot.sound}, 0 warnings)!"
}

Write-Host "`nAUTOPHAGE v1.3.0 BUILD & VERIFICATION COMPLETE."
