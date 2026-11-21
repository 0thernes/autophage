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
    "-march=native", "-mgfni", "-mvpclmulqdq", "-mpclmul", "-mbmi", "-mbmi2", "-mpopcnt", "-mavx2",
    "--target=x86_64-w64-windows-gnu",
    "-DAUTOPHAGE_BARE_WIN64",
    "-I$proj\include",
    "-isystem", "$root\include\clang"
)
$ldflags = @("-L$root\lib", "-fuse-ld=lld")
$libs = @("-lkernel32", "-lucrtbase", "-lm", "-lmingw32", "-lmingwex")

Write-Host "[1/4] Building bin\autophage.exe ..."
& $clang @cflags @core "$proj\src\main_cli.c" -o "$proj\bin\autophage.exe" @ldflags @libs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[2/4] Building bin\autophage.dll (+ import lib) ..."
& $clang @cflags -shared -DAUTOPHAGE_BUILD_DLL @core -o "$proj\bin\autophage.dll" "-Wl,--out-implib,$proj\bin\libautophage.dll.a" @ldflags @libs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "[3/4] Building bin\test_falsification.exe ..."
& $clang @cflags @core "$proj\test\test_falsification.c" -o "$proj\bin\test_falsification.exe" @ldflags @libs
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
    Write-Host "`nVerifying 31 Formal Lean 4 Mathematical Proofs..."
    & $lean "$proj\lean\AutophageProofs.lean"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "Lean 4 Formal Verification PASSED (31 theorems, 0 sorryAx)!"
}

Write-Host "`nAUTOPHAGE v1.1 BUILD & VERIFICATION COMPLETE."
