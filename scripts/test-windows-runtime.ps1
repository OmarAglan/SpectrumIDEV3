param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [int]$StartupSeconds = 2
)

$ErrorActionPreference = 'Stop'

$resolvedExecutable = (Resolve-Path $Executable).Path
$applicationDirectory = Split-Path -Parent $resolvedExecutable
$requiredFiles = @(
    'libstdc++-6.dll',
    'libwinpthread-1.dll',
    'Qt6Core.dll',
    'Qt6Gui.dll',
    'Qt6Widgets.dll',
    'platforms\qwindows.dll'
)

foreach ($requiredFile in $requiredFiles) {
    $requiredPath = Join-Path $applicationDirectory $requiredFile
    if (!(Test-Path $requiredPath)) {
        throw "Required Windows runtime file is missing: $requiredPath"
    }
}

$gccRuntimeCandidates = @(
    'libgcc_s_seh-1.dll',
    'libgcc_s_dw2-1.dll'
)
if (!($gccRuntimeCandidates | Where-Object {
        Test-Path (Join-Path $applicationDirectory $_)
    })) {
    throw "A MinGW GCC runtime DLL is missing beside $resolvedExecutable."
}

$previousPath = $env:Path
$previousPlatform = $env:QT_QPA_PLATFORM
$windowsDirectory = if ($env:SystemRoot) { $env:SystemRoot } else { 'C:\Windows' }
$env:Path = "$windowsDirectory\System32;$windowsDirectory"
$env:QT_QPA_PLATFORM = 'offscreen'
$process = $null
try {
    $process = Start-Process -FilePath $resolvedExecutable `
        -WindowStyle Hidden `
        -PassThru
    Start-Sleep -Seconds $StartupSeconds
    if ($process.HasExited) {
        throw "Qalam exited during runtime startup with code $($process.ExitCode)."
    }
} finally {
    if ($process -and !$process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
    $env:Path = $previousPath
    $env:QT_QPA_PLATFORM = $previousPlatform
}

Write-Host 'Qalam started without external Qt or MinGW runtime paths.' -ForegroundColor Green
