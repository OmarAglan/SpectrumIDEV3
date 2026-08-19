param(
    [string]$QtRoot = $env:QALAM_QT_DIR,
    [string]$BuildDir = 'build/windows-release',
    [string]$PackageDir = 'dist/Qalam-win64',
    [string]$ZipPath = 'dist/Qalam-win64.zip',
    [string]$BaaLspExecutable = $env:QALAM_BAA_LSP_PATH,
    [string]$BaaCompilerExecutable = $env:QALAM_BAA_PATH,
    [string]$BaaSourceDir = '',
    [string]$NazmExecutable = $env:QALAM_NAZM_PATH,
    [string]$NazmSourceDir = '',
    [string]$GccRoot = $env:QALAM_GCC_ROOT,
    [switch]$SkipLanguageServer,
    [switch]$SkipCompiler,
    [switch]$SkipBuild,
    [switch]$SkipArchive
)

$ErrorActionPreference = 'Stop'
$nazmArabicExecutableName =
    (-join [char[]](0x0646, 0x0638, 0x0645)) + '.exe'
Set-Location (Split-Path -Parent $PSScriptRoot)

if (!$SkipBuild) {
    & (Join-Path $PSScriptRoot 'build-windows.ps1') `
        -Configuration Release `
        -QtRoot $QtRoot `
        -BuildDir $BuildDir
}

$exe = Join-Path $BuildDir 'qalam/Qalam.exe'
if (!(Test-Path $exe)) {
    throw "Qalam.exe was not found at $exe. Build the project first."
}

$cachePath = Join-Path $BuildDir 'CMakeCache.txt'
if (!(Test-Path $cachePath)) {
    throw "CMakeCache.txt was not found at $cachePath."
}
$compilerMatch = Select-String -Path $cachePath `
    -Pattern '^CMAKE_CXX_COMPILER(?::[^=]+)?=(.+)$' |
    Select-Object -First 1
if (!$compilerMatch) {
    throw "The configured C++ compiler was not recorded in $cachePath."
}
$compilerPath = $compilerMatch.Matches[0].Groups[1].Value.Trim()
$compilerRuntimeDir = Split-Path -Parent $compilerPath
$compilerRuntimeDlls = @(
    'libgcc_s_seh-1.dll',
    'libgcc_s_dw2-1.dll',
    'libstdc++-6.dll',
    'libwinpthread-1.dll'
) | ForEach-Object { Join-Path $compilerRuntimeDir $_ } |
    Where-Object { Test-Path $_ }
if (!$compilerRuntimeDlls) {
    throw "No MinGW runtime DLLs were found beside $compilerPath."
}

if (Test-Path $PackageDir) { Remove-Item $PackageDir -Recurse -Force }
New-Item -ItemType Directory -Path $PackageDir | Out-Null
Copy-Item $exe $PackageDir

if (!$QtRoot) { $QtRoot = $env:QALAM_QT_DIR }
if (!$QtRoot -and $env:QTDIR) { $QtRoot = $env:QTDIR }
if (!$QtRoot -and (Test-Path 'C:/Qt')) {
    $QtRoot = Get-ChildItem 'C:/Qt' -Directory |
        Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName 'mingw_64' } |
        Where-Object { Test-Path (Join-Path $_ 'bin/windeployqt.exe') } |
        Select-Object -First 1
}

$windeployqt = if ($QtRoot) { Join-Path $QtRoot 'bin/windeployqt.exe' } else { 'windeployqt.exe' }
if (!(Get-Command $windeployqt -ErrorAction SilentlyContinue)) {
    throw 'windeployqt.exe was not found. Set QALAM_QT_DIR to your Qt MinGW kit path.'
}

& $windeployqt --no-compiler-runtime --no-system-dxc-compiler --dir $PackageDir (Join-Path $PackageDir 'Qalam.exe')
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE."
}

# windeployqt must not select a compiler runtime from PATH. Copy the exact
# runtime recorded by CMake for this build instead.
Copy-Item $compilerRuntimeDlls $PackageDir -Force

$packagedLanguageServer = $null
if (!$SkipLanguageServer) {
    $serverCandidates = [System.Collections.Generic.List[string]]::new()
    if ($BaaLspExecutable) { $serverCandidates.Add($BaaLspExecutable) }
    $serverCandidates.Add('..\Baa-LSP\build\windows-release\baa-lsp.exe')
    $serverCandidates.Add('..\Baa-LSP\build\ci-admission\baa-lsp.exe')
    $serverCandidates.Add('ecosystem\Baa-LSP\build\windows-release\baa-lsp.exe')
    $serverCandidates.Add('Baa-LSP\build\windows-release\baa-lsp.exe')
    $pathServer = Get-Command baa-lsp.exe -ErrorAction SilentlyContinue
    if ($pathServer) { $serverCandidates.Add($pathServer.Source) }

    $resolvedServer = $serverCandidates |
        Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } |
        ForEach-Object { (Resolve-Path -LiteralPath $_).Path } |
        Select-Object -First 1
    if (!$resolvedServer) {
        throw @'
Baa-LSP was not found, so a complete Qalam package cannot be created.
Build the sibling Baa-LSP repository, pass -BaaLspExecutable, set
QALAM_BAA_LSP_PATH, or use -SkipLanguageServer only for an intentional UI-only
development package.
'@
    }

    $serverDirectory = Join-Path $PackageDir 'baa-lsp'
    New-Item -ItemType Directory -Path $serverDirectory -Force | Out-Null
    $packagedLanguageServer = Join-Path $serverDirectory 'baa-lsp.exe'
    Copy-Item -LiteralPath $resolvedServer -Destination $packagedLanguageServer -Force
}

$packagedCompiler = $null
$packagedNazm = $null
$packagedToolchain = $null
if (!$SkipCompiler) {
    $compilerCandidates = [System.Collections.Generic.List[string]]::new()
    if ($BaaCompilerExecutable) { $compilerCandidates.Add($BaaCompilerExecutable) }
    $compilerCandidates.Add('..\Baa\build\windows-release\baa.exe')
    $compilerCandidates.Add('..\Baa\build\baa.exe')
    $compilerCandidates.Add('ecosystem\Baa\build\windows-release\baa.exe')
    $compilerCandidates.Add('ecosystem\Baa\build\baa.exe')
    $pathCompiler = Get-Command baa.exe -ErrorAction SilentlyContinue
    if ($pathCompiler) { $compilerCandidates.Add($pathCompiler.Source) }

    $resolvedCompiler = $compilerCandidates |
        Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } |
        ForEach-Object { (Resolve-Path -LiteralPath $_).Path } |
        Select-Object -First 1
    if (!$resolvedCompiler) {
        throw @'
Baa was not found, so a functional Qalam package cannot be created. Build the
sibling Baa repository, pass -BaaCompilerExecutable, set QALAM_BAA_PATH, or use
-SkipCompiler only for an intentional UI/LSP development package.
'@
    }

    $sourceCandidates = [System.Collections.Generic.List[string]]::new()
    if ($BaaSourceDir) { $sourceCandidates.Add($BaaSourceDir) }
    $sourceCandidates.Add('..\Baa')
    $sourceCandidates.Add('ecosystem\Baa')
    $resolvedSource = $sourceCandidates |
        Where-Object {
            $_ -and (Test-Path -LiteralPath (Join-Path $_ 'stdlib') -PathType Container)
        } |
        ForEach-Object { (Resolve-Path -LiteralPath $_).Path } |
        Select-Object -First 1
    if (!$resolvedSource) {
        throw 'Baa stdlib was not found. Pass -BaaSourceDir for the compiler source tree.'
    }

    $runtimeCandidates = @(
        (Join-Path (Split-Path -Parent $resolvedCompiler) 'libbaa_runtime.a'),
        (Join-Path $resolvedSource 'build\windows-release\libbaa_runtime.a'),
        (Join-Path $resolvedSource 'build\libbaa_runtime.a')
    )
    $resolvedRuntime = $runtimeCandidates |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        ForEach-Object { (Resolve-Path -LiteralPath $_).Path } |
        Select-Object -First 1
    if (!$resolvedRuntime) {
        throw 'Baa runtime archive libbaa_runtime.a was not found beside the selected compiler.'
    }

    $compilerDirectory = Join-Path $PackageDir 'baa'
    New-Item -ItemType Directory -Path $compilerDirectory -Force | Out-Null
    $packagedCompiler = Join-Path $compilerDirectory 'baa.exe'
    Copy-Item -LiteralPath $resolvedCompiler -Destination $packagedCompiler -Force
    Copy-Item -LiteralPath $resolvedRuntime `
        -Destination (Join-Path $compilerDirectory 'libbaa_runtime.a') -Force
    Copy-Item -LiteralPath (Join-Path $resolvedSource 'stdlib') `
        -Destination (Join-Path $compilerDirectory 'stdlib') -Recurse -Force

    $nazmCandidates = [System.Collections.Generic.List[string]]::new()
    if ($NazmExecutable) { $nazmCandidates.Add($NazmExecutable) }
    if ($NazmSourceDir) {
        $nazmCandidates.Add((Join-Path $NazmSourceDir (
            'build\windows-release\' + $nazmArabicExecutableName)))
        $nazmCandidates.Add((Join-Path $NazmSourceDir 'build\windows-release\nazm.exe'))
        $nazmCandidates.Add((Join-Path $NazmSourceDir (
            'build\' + $nazmArabicExecutableName)))
        $nazmCandidates.Add((Join-Path $NazmSourceDir 'build\nazm.exe'))
    }
    $nazmCandidates.Add((Join-Path '..\Nazm\build\windows-release' $nazmArabicExecutableName))
    $nazmCandidates.Add('..\Nazm\build\windows-release\nazm.exe')
    $nazmCandidates.Add((Join-Path '..\Nazm\build' $nazmArabicExecutableName))
    $nazmCandidates.Add('..\Nazm\build\nazm.exe')
    $nazmCandidates.Add((Join-Path 'ecosystem\Nazm\build\windows-release' $nazmArabicExecutableName))
    $nazmCandidates.Add('ecosystem\Nazm\build\windows-release\nazm.exe')
    $pathNazm = Get-Command $nazmArabicExecutableName -ErrorAction SilentlyContinue
    if (!$pathNazm) { $pathNazm = Get-Command nazm.exe -ErrorAction SilentlyContinue }
    if ($pathNazm) { $nazmCandidates.Add($pathNazm.Source) }

    $resolvedNazm = $nazmCandidates |
        Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Leaf) } |
        ForEach-Object { (Resolve-Path -LiteralPath $_).Path } |
        Select-Object -First 1
    if (!$resolvedNazm) {
        throw @'
Nazm was not found, so the bundled Baa compiler could not produce objects.
Build the sibling Nazm repository or pass -NazmExecutable/-NazmSourceDir.
'@
    }
    $packagedNazm = Join-Path $compilerDirectory $nazmArabicExecutableName
    Copy-Item -LiteralPath $resolvedNazm -Destination $packagedNazm -Force

    $gccCandidates = [System.Collections.Generic.List[string]]::new()
    if ($GccRoot) { $gccCandidates.Add($GccRoot) }
    $gccCandidates.Add((Split-Path -Parent $compilerRuntimeDir))
    $resolvedGccRoot = $gccCandidates |
        Where-Object {
            $_ -and
            (Test-Path -LiteralPath (Join-Path $_ 'bin\gcc.exe') -PathType Leaf) -and
            (Test-Path -LiteralPath (Join-Path $_ 'lib') -PathType Container) -and
            (Test-Path -LiteralPath (Join-Path $_ 'libexec') -PathType Container) -and
            (Test-Path -LiteralPath (Join-Path $_ 'x86_64-w64-mingw32') -PathType Container) -and
            (Test-Path -LiteralPath (Join-Path $_ 'licenses') -PathType Container)
        } |
        ForEach-Object { (Resolve-Path -LiteralPath $_).Path } |
        Select-Object -First 1
    if (!$resolvedGccRoot) {
        throw @'
A complete relocatable MinGW-w64 GCC root was not found, so the Qalam package
would still depend on an arbitrary linker from PATH. Pass -GccRoot or set
QALAM_GCC_ROOT to a directory containing bin, lib, libexec, licenses, and the
x86_64-w64-mingw32 target tree.
'@
    }

    $sourceGcc = Join-Path $resolvedGccRoot 'bin\gcc.exe'
    $gccTarget = (& $sourceGcc -dumpmachine | Out-String).Trim()
    $gccVersion = (& $sourceGcc -dumpfullversion | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $gccTarget -ne 'x86_64-w64-mingw32' -or !$gccVersion) {
        throw "Unsupported portable GCC toolchain at $resolvedGccRoot."
    }

    $packagedToolchain = Join-Path $compilerDirectory 'gcc'
    New-Item -ItemType Directory -Path $packagedToolchain -Force | Out-Null
    Get-ChildItem -LiteralPath $resolvedGccRoot -Force |
        Copy-Item -Destination $packagedToolchain -Recurse -Force

    $gccHash = (Get-FileHash -LiteralPath $sourceGcc -Algorithm SHA256).Hash
    [IO.File]::WriteAllLines(
        (Join-Path $packagedToolchain 'BAA-TOOLCHAIN-MANIFEST.txt'),
        @(
            'format=baa-portable-toolchain-v1',
            "target=$gccTarget",
            "gcc_version=$gccVersion",
            "gcc_sha256=$gccHash",
            'unicode_paths=direct',
            'pei386_runtime_relocator=retain',
            'purpose=Baa hosted linking until the native Nazm linker is admitted'
        ),
        [Text.UTF8Encoding]::new($false))
}

$runtimeTestArguments = @{
    Executable = (Join-Path $PackageDir 'Qalam.exe')
    StartupSeconds = 1
}
if ($packagedLanguageServer) {
    $runtimeTestArguments.LanguageServer = $packagedLanguageServer
}
if ($packagedCompiler) {
    $runtimeTestArguments.Compiler = $packagedCompiler
    $runtimeTestArguments.Nazm = $packagedNazm
    $runtimeTestArguments.ToolchainRoot = $packagedToolchain
}
& (Join-Path $PSScriptRoot 'test-windows-runtime.ps1') @runtimeTestArguments

if (!$SkipArchive) {
    if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
    Compress-Archive -Path (Join-Path $PackageDir '*') -DestinationPath $ZipPath
}

Write-Host "Packaged Qalam successfully:" -ForegroundColor Green
if (!$SkipArchive) { Write-Host "  $ZipPath" }
else { Write-Host "  $PackageDir" }
if ($packagedLanguageServer) {
    Write-Host "  Baa-LSP: $packagedLanguageServer"
}
if ($packagedCompiler) {
    Write-Host "  Baa: $packagedCompiler"
    Write-Host "  Nazm: $packagedNazm"
    Write-Host "  GCC: $packagedToolchain"
}
