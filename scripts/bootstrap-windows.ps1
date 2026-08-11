param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [string]$QtVersion = '6.10.2',

    [string]$QtArch = 'win64_mingw',

    [string]$QtRoot = 'C:\Qt',

    [string]$BuildDir = '',

    [switch]$NoPackage,

    [switch]$BuildTests,

    [switch]$SkipWinget,

    [switch]$SkipQtInstall,

    [switch]$ForceQtInstall,

    [string]$BaaLspSourceDir = '',

    [string]$BaaLspExecutable = $env:QALAM_BAA_LSP_PATH,

    [switch]$SkipLanguageServer
)

$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)

function Write-Step {
    param([string]$Message)
    Write-Host "`n==> $Message" -ForegroundColor Cyan
}

function Refresh-Path {
    $machinePath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $env:PATH = "$machinePath;$userPath;$env:PATH"

    $commonPaths = @(
        'C:\Program Files\CMake\bin',
        'C:\Program Files\Git\cmd',
        'C:\Program Files\Git\bin'
    )

    $pythonRoots = @(
        "$env:LOCALAPPDATA\Programs\Python",
        'C:\Program Files',
        'C:\Program Files (x86)'
    )

    foreach ($root in $pythonRoots) {
        if (!$root -or !(Test-Path $root)) { continue }
        Get-ChildItem $root -Directory -Filter 'Python3*' -ErrorAction SilentlyContinue | ForEach-Object {
            $commonPaths += $_.FullName
            $commonPaths += (Join-Path $_.FullName 'Scripts')
        }
    }

    foreach ($path in $commonPaths) {
        if ($path -and (Test-Path $path)) {
            $env:PATH = "$path;$env:PATH"
        }
    }
}

function Get-CommandOrNull {
    param([string]$Name)
    return Get-Command $Name -ErrorAction SilentlyContinue
}

function Invoke-Native {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [switch]$AllowFailure
    )

    & $FilePath @Arguments
    $exitCode = $LASTEXITCODE
    if ($null -eq $exitCode) { $exitCode = 0 }

    if (($exitCode -ne 0) -and !$AllowFailure) {
        throw "$FilePath failed with exit code $exitCode."
    }

    return $exitCode
}

function Require-Winget {
    if (Get-CommandOrNull winget.exe) { return }
    throw @'
winget.exe was not found.
Install/enable Windows Package Manager first, or install these manually:
- Python 3.12+
- CMake 3.21+
- Qt 6 MinGW kit
Then run scripts/build-windows.ps1.
'@
}

function Test-PythonExecutable {
    param(
        [string]$PythonPath,
        [switch]$UseLauncher
    )

    if (!$PythonPath) { return $false }
    if ($PythonPath -match '\\WindowsApps\\python(3)?\.exe$') { return $false }

    $probeCode = "import sys; print(sys.executable); print(str(sys.version_info[0]) + '.' + str(sys.version_info[1]))"
    $probeArguments = @('-c', $probeCode)
    if ($UseLauncher) {
        $probeArguments = @('-3') + $probeArguments
    }

    try {
        $output = & $PythonPath @probeArguments 2>&1
        $exitCode = $LASTEXITCODE
        if ($null -eq $exitCode) { $exitCode = 0 }
    } catch {
        return $false
    }

    if ($exitCode -ne 0) { return $false }

    $lines = @($output | ForEach-Object { ($_ -as [string]).Trim() } | Where-Object { $_ })
    if ($lines.Count -lt 2) { return $false }

    $versionText = $lines[-1]
    if ($versionText -notmatch '^3\.(\d+)$') { return $false }

    $minor = [int]$Matches[1]
    if ($minor -lt 8) { return $false }

    return $true
}

function Get-PythonCommand {
    Refresh-Path

    # Prefer real python.exe/python3.exe first. Some machines have a broken or stale py.exe launcher,
    # and Windows Store aliases can pretend Python exists when it does not.
    $candidates = @('python.exe', 'python3.exe')
    foreach ($candidate in $candidates) {
        $commands = @(Get-Command $candidate -ErrorAction SilentlyContinue -All)
        foreach ($cmd in $commands) {
            if (!$cmd.Source) { continue }
            if ($cmd.Source -match '\\WindowsApps\\python(3)?\.exe$') { continue }
            if (Test-PythonExecutable -PythonPath $cmd.Source) {
                return @{ Path = $cmd.Source; UseLauncher = $false }
            }
        }
    }

    # Fall back to the Python launcher only if it can actually run Python 3.
    $pyLauncher = Get-CommandOrNull 'py.exe'
    if ($pyLauncher -and (Test-PythonExecutable -PythonPath $pyLauncher.Source -UseLauncher)) {
        return @{ Path = $pyLauncher.Source; UseLauncher = $true }
    }

    return $null
}

function Invoke-Python {
    param([string[]]$Arguments)
    $python = Get-PythonCommand
    if (!$python) {
        throw @'
Python is installed incorrectly or only the Microsoft Store alias exists.
Fix options:
  1. Run: winget install --id Python.Python.3.12 --exact --source winget --accept-package-agreements --accept-source-agreements
  2. Close and reopen PowerShell, then rerun build-qalam-windows.cmd
  3. Or disable the App Installer python.exe/python3.exe aliases from Windows Settings > Apps > Advanced app settings > App execution aliases.
'@
    }

    if ($python.UseLauncher) {
        Invoke-Native -FilePath $python.Path -Arguments (@('-3') + $Arguments) | Out-Null
    } else {
        Invoke-Native -FilePath $python.Path -Arguments $Arguments | Out-Null
    }
}

function Install-WingetPackage {
    param(
        [string]$Id,
        [string]$CommandName
    )

    if ($Id -like 'Python.*') {
        if (Get-PythonCommand) {
            Write-Host 'Already installed: working Python 3'
            return
        }
    } elseif ($CommandName -and (Get-CommandOrNull $CommandName)) {
        Write-Host "Already installed: $CommandName"
        return
    }

    if ($SkipWinget) {
        Write-Host "Skipping winget install for $Id because -SkipWinget was supplied." -ForegroundColor Yellow
        return
    }

    Require-Winget
    Write-Step "Installing $Id"
    Invoke-Native -FilePath 'winget.exe' -Arguments @(
        'install', '--id', $Id, '--exact', '--source', 'winget',
        '--accept-package-agreements', '--accept-source-agreements'
    ) | Out-Null
    Refresh-Path
}

function Find-QtRoot {
    param(
        [string]$Root,
        [string]$Version
    )

    $exactCandidates = @(
        (Join-Path $Root "$Version\mingw_64"),
        (Join-Path $Root "$Version\$QtArch")
    )

    foreach ($candidate in $exactCandidates) {
        if (Test-Path (Join-Path $candidate 'lib\cmake\Qt6\Qt6Config.cmake')) {
            return (Resolve-Path $candidate).Path
        }
    }

    if (Test-Path $Root) {
        $matches = Get-ChildItem $Root -Recurse -Filter Qt6Config.cmake -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match [regex]::Escape("$Version") -and $_.FullName -match 'cmake\\Qt6\\Qt6Config\.cmake$' } |
            Sort-Object FullName |
            Select-Object -First 1

        if ($matches) {
            return (Resolve-Path (Join-Path $matches.DirectoryName '..\..\..')).Path
        }
    }

    return $null
}

function Install-QtIfNeeded {
    $currentQt = Find-QtRoot -Root $QtRoot -Version $QtVersion
    if ($currentQt -and !$ForceQtInstall) {
        Write-Host "Qt already installed: $currentQt"
        return $currentQt
    }

    if ($SkipQtInstall) {
        throw "Qt $QtVersion was not found under $QtRoot and -SkipQtInstall was supplied."
    }

    Write-Step "Installing aqtinstall"
    Invoke-Python @('-m', 'pip', 'install', '--user', '--upgrade', 'pip', 'aqtinstall')
    Refresh-Path

    Write-Step "Installing Qt $QtVersion ($QtArch) to $QtRoot"
    Invoke-Python @('-m', 'aqt', 'install-qt', '-O', $QtRoot, 'windows', 'desktop', $QtVersion, $QtArch)

    $installedQt = Find-QtRoot -Root $QtRoot -Version $QtVersion
    if (!$installedQt) {
        throw "Qt installation completed, but Qt6Config.cmake was not found under $QtRoot."
    }

    return $installedQt
}

function Find-MingwBin {
    param([string]$Root)

    $candidates = @(
        (Join-Path $Root 'Tools\mingw1310_64\bin'),
        (Join-Path $Root 'Tools\mingw1120_64\bin'),
        (Join-Path $Root 'Tools\mingw1100_64\bin'),
        (Join-Path $Root 'Tools\mingw900_64\bin')
    )

    if (Test-Path (Join-Path $Root 'Tools')) {
        $candidates += Get-ChildItem (Join-Path $Root 'Tools') -Directory -Filter 'mingw*_64' -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'bin' }
    }

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path (Join-Path $candidate 'g++.exe'))) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Install-MingwIfNeeded {
    $mingw = Find-MingwBin -Root $QtRoot
    if ($mingw -and !$ForceQtInstall) {
        Write-Host "MinGW already installed: $mingw"
        return $mingw
    }

    if ($SkipQtInstall) {
        throw "MinGW was not found under $QtRoot and -SkipQtInstall was supplied."
    }

    Write-Step 'Installing Qt MinGW toolchain'
    Invoke-Python @('-m', 'pip', 'install', '--user', '--upgrade', 'aqtinstall')

    $attempts = @(
        @('tools_mingw1310', 'qt.tools.win64_mingw1310'),
        @('tools_mingw', 'qt.tools.win64_mingw1310'),
        @('tools_mingw1120', 'qt.tools.win64_mingw1120'),
        @('tools_mingw90', ''),
        @('tools_mingw', 'qt.tools.win64_mingw1120')
    )

    $lastError = $null
    foreach ($attempt in $attempts) {
        $tool = $attempt[0]
        $variant = $attempt[1]
        try {
            Write-Host "Trying aqt tool package: $tool $variant"
            if ($variant) {
                Invoke-Python @('-m', 'aqt', 'install-tool', '-O', $QtRoot, 'windows', 'desktop', $tool, $variant)
            } else {
                Invoke-Python @('-m', 'aqt', 'install-tool', '-O', $QtRoot, 'windows', 'desktop', $tool)
            }
            $mingw = Find-MingwBin -Root $QtRoot
            if ($mingw) { return $mingw }
        } catch {
            $lastError = $_
            Write-Host "Failed: $tool $variant" -ForegroundColor Yellow
        }
    }

    throw "Could not install/find MinGW. Last error: $lastError"
}

Write-Step 'Checking base Windows build tools'
Refresh-Path
Install-WingetPackage -Id 'Python.Python.3.12' -CommandName 'python.exe'
Install-WingetPackage -Id 'Kitware.CMake' -CommandName 'cmake.exe'
Refresh-Path

if (!(Get-PythonCommand)) { throw 'A working Python 3 was not found. The Microsoft Store alias does not count as a real Python install.' }
if (!(Get-CommandOrNull 'cmake.exe')) { throw 'CMake is required but was not found.' }

$resolvedQtRoot = Install-QtIfNeeded
$mingwBin = Install-MingwIfNeeded

if (!$BuildDir) {
    $BuildDir = "build/windows-$($Configuration.ToLowerInvariant())"
}

$env:QALAM_QT_DIR = $resolvedQtRoot
$env:PATH = "$mingwBin;$resolvedQtRoot\bin;$env:PATH"

Write-Step 'Building Qalam IDE'
& (Join-Path $PSScriptRoot 'build-windows.ps1') `
    -Configuration $Configuration `
    -QtRoot $resolvedQtRoot `
    -BuildDir $BuildDir `
    -BuildTests:$BuildTests

if (!$NoPackage) {
    if (!$SkipLanguageServer -and !$BaaLspExecutable) {
        $sourceCandidates = [System.Collections.Generic.List[string]]::new()
        if ($BaaLspSourceDir) { $sourceCandidates.Add($BaaLspSourceDir) }
        $sourceCandidates.Add((Join-Path (Get-Location) '..\Baa-LSP'))
        $sourceCandidates.Add((Join-Path (Get-Location) 'ecosystem\Baa-LSP'))
        $resolvedServerSource = $sourceCandidates |
            Where-Object {
                $_ -and (Test-Path -LiteralPath (Join-Path $_ 'CMakeLists.txt')) -and
                (Test-Path -LiteralPath (Join-Path $_ 'scripts\build-windows.ps1'))
            } |
            ForEach-Object { (Resolve-Path -LiteralPath $_).Path } |
            Select-Object -First 1
        if (!$resolvedServerSource) {
            throw @'
Baa-LSP source was not found. A production Qalam package includes the language
server. Clone it beside Qalam, pass -BaaLspSourceDir/-BaaLspExecutable, or use
-SkipLanguageServer only for an intentional UI-only development package.
'@
        }

        Write-Step 'Building the bundled Baa-LSP server'
        Push-Location $resolvedServerSource
        try {
            & (Join-Path $resolvedServerSource 'scripts\build-windows.ps1') `
                -Configuration $Configuration `
                -BuildDir "build/windows-$($Configuration.ToLowerInvariant())" `
                -MingwBin $mingwBin `
                -SkipTests
        } finally {
            Pop-Location
        }
        $BaaLspExecutable = (Resolve-Path -LiteralPath (
            Join-Path $resolvedServerSource (
                "build/windows-$($Configuration.ToLowerInvariant())/baa-lsp.exe"))).Path
    }

    Write-Step 'Packaging portable Windows ZIP'
    & (Join-Path $PSScriptRoot 'package-windows.ps1') `
        -QtRoot $resolvedQtRoot `
        -BuildDir $BuildDir `
        -BaaLspExecutable $BaaLspExecutable `
        -SkipLanguageServer:$SkipLanguageServer `
        -SkipBuild
}

Write-Host "`nQalam IDE is ready." -ForegroundColor Green
Write-Host "Executable: $BuildDir/qalam/Qalam.exe"
if (!$NoPackage) {
    Write-Host 'Portable ZIP: dist/Qalam-win64.zip'
}
