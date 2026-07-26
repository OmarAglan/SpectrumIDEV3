param(
    [string]$QtRoot = $env:QALAM_QT_DIR,
    [string]$BuildDir = 'build/windows-release',
    [string]$PackageDir = 'dist/Qalam-win64',
    [string]$ZipPath = 'dist/Qalam-win64.zip',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)

if (!$SkipBuild) {
    & (Join-Path $PSScriptRoot 'build-windows.ps1') -Configuration Release -QtRoot $QtRoot
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

# Optional: bundle the Baa compiler if the repository contains a local compiler folder.
if (Test-Path 'baa') {
    Copy-Item 'baa' (Join-Path $PackageDir 'baa') -Recurse -Force
}

if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Compress-Archive -Path (Join-Path $PackageDir '*') -DestinationPath $ZipPath

Write-Host "Packaged Qalam successfully:" -ForegroundColor Green
Write-Host "  $ZipPath"
