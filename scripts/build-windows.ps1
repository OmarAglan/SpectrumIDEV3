param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [string]$QtRoot = $env:QALAM_QT_DIR,

    [string]$BuildDir = '',

    [switch]$DeployAfterBuild,

    [switch]$SkipDeployAfterBuild,

    [switch]$BuildTests
)

$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)

$script:NormalizedNativePath = $null
$script:CMakeLauncher = $null

if (!$BuildDir) {
    $BuildDir = "build/windows-$($Configuration.ToLowerInvariant())"
}

function Invoke-Native {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    if ($script:NormalizedNativePath -and $script:CMakeLauncher) {
        # Do not inherit the caller's native environment. Some Windows hosts
        # expose both Path and PATH, possibly with different MinGW toolchains.
        # CMake removes both spellings before launching the selected command.
        & $script:CMakeLauncher -E env `
            --unset=Path `
            --unset=PATH `
            "Path=$script:NormalizedNativePath" `
            $FilePath @Arguments
    } else {
        & $FilePath @Arguments
    }
    $exitCode = $LASTEXITCODE
    if ($null -eq $exitCode) { $exitCode = 0 }
    if ($exitCode -ne 0) {
        throw "$FilePath failed with exit code $exitCode."
    }
}

function New-NormalizedNativePath {
    param([string[]]$PreferredDirectories)

    $seen = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $result = [System.Collections.Generic.List[string]]::new()
    $machinePath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $rawEntries = @($PreferredDirectories) + @($machinePath, $userPath)

    foreach ($rawEntry in $rawEntries) {
        if (!$rawEntry) { continue }
        foreach ($entry in ($rawEntry -split ';')) {
            $candidate = $entry.Trim().Trim('"')
            if (!$candidate) { continue }
            if ($seen.Add($candidate)) {
                $result.Add($candidate)
            }
        }
    }

    return ($result -join ';')
}

function Resolve-QtRoot {
    param([string]$ProvidedRoot)

    $candidates = @()
    if ($ProvidedRoot) { $candidates += $ProvidedRoot }
    if ($env:QTDIR) { $candidates += $env:QTDIR }
    if (Test-Path 'C:/Qt') {
        $candidates += Get-ChildItem 'C:/Qt' -Directory |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'mingw_64' }
    }

    foreach ($candidate in $candidates) {
        if (!$candidate) { continue }
        $cmakeConfig = Join-Path $candidate 'lib/cmake/Qt6/Qt6Config.cmake'
        if (Test-Path $cmakeConfig) { return (Resolve-Path $candidate).Path }
    }

    throw 'Qt 6 MinGW kit was not found. Install Qt 6 with MinGW, or set QALAM_QT_DIR to something like C:\Qt\6.10.2\mingw_64.'
}

function Resolve-MingwBin {
    param([string]$ResolvedQtRoot)

    $qtParent = Split-Path -Parent $ResolvedQtRoot
    $qtInstall = Split-Path -Parent $qtParent
    $toolCandidates = @()

    if (Test-Path (Join-Path $qtInstall 'Tools')) {
        $toolCandidates += Get-ChildItem (Join-Path $qtInstall 'Tools') -Directory -Filter 'mingw*_64' |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'bin' }
    }

    foreach ($candidate in $toolCandidates) {
        if (Test-Path (Join-Path $candidate 'g++.exe')) { return (Resolve-Path $candidate).Path }
    }

    $pathGxx = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($pathGxx) { return (Split-Path -Parent $pathGxx.Source) }

    throw 'MinGW g++.exe was not found. Install the MinGW kit from Qt Maintenance Tool, or add MinGW bin to PATH.'
}

$QtRoot = Resolve-QtRoot -ProvidedRoot $QtRoot
$MingwBin = Resolve-MingwBin -ResolvedQtRoot $QtRoot
$MakeProgram = Join-Path $MingwBin 'mingw32-make.exe'
$Gxx = Join-Path $MingwBin 'g++.exe'
$cmakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
if (!$cmakeCommand) {
    throw 'cmake.exe was not found. Install CMake or add it to the machine or user Path.'
}
$CMakeProgram = $cmakeCommand.Source
$CTestProgram = Join-Path (Split-Path -Parent $CMakeProgram) 'ctest.exe'
$script:CMakeLauncher = $CMakeProgram
$script:NormalizedNativePath = New-NormalizedNativePath -PreferredDirectories @(
    $MingwBin,
    (Join-Path $QtRoot 'bin'),
    (Split-Path -Parent $CMakeProgram),
    "$env:SystemRoot\System32",
    $env:SystemRoot
)

$deployFlag = if ($SkipDeployAfterBuild) { 'OFF' } else { 'ON' }
$testsFlag = if ($BuildTests) { 'ON' } else { 'OFF' }

Invoke-Native -FilePath $CMakeProgram -Arguments @(
    '-S', '.',
    '-B', $BuildDir,
    '-G', 'MinGW Makefiles',
    "-DCMAKE_PREFIX_PATH=$QtRoot",
    "-DCMAKE_CXX_COMPILER=$Gxx",
    "-DCMAKE_MAKE_PROGRAM=$MakeProgram",
    "-DCMAKE_BUILD_TYPE=$Configuration",
    '-DCMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders=TRUE',
    "-DQALAM_DEPLOY_AFTER_BUILD=$deployFlag",
    "-DQALAM_BUILD_TESTS=$testsFlag"
)

Invoke-Native -FilePath $CMakeProgram -Arguments @('--build', $BuildDir, '--target', 'Qalam', '--parallel')

if ($BuildTests) {
    # Build the complete configured test graph so newly registered CTest targets
    # cannot be skipped by a stale hard-coded executable list.
    Invoke-Native -FilePath $CMakeProgram -Arguments @('--build', $BuildDir, '--parallel')
    Invoke-Native -FilePath $CTestProgram -Arguments @('--test-dir', $BuildDir, '--output-on-failure')
}

Write-Host "Built Qalam successfully:" -ForegroundColor Green
Write-Host "  $BuildDir/qalam/Qalam.exe"
