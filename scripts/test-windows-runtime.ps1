param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [string]$LanguageServer = '',

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
    if ($LanguageServer) {
        $resolvedLanguageServer = (Resolve-Path -LiteralPath $LanguageServer).Path
        $probeInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $probeInfo.FileName = $resolvedLanguageServer
        $probeInfo.Arguments = '--version'
        $probeInfo.UseShellExecute = $false
        $probeInfo.CreateNoWindow = $true
        $probeInfo.RedirectStandardOutput = $true
        $probeInfo.RedirectStandardError = $true
        $probe = [System.Diagnostics.Process]::new()
        $probe.StartInfo = $probeInfo
        if (!$probe.Start()) {
            throw 'Bundled Baa-LSP could not be started.'
        }
        $probeOutput = $probe.StandardOutput.ReadToEnd()
        $probeError = $probe.StandardError.ReadToEnd()
        $probe.WaitForExit()
        if ($probe.ExitCode -ne 0) {
            throw "Bundled Baa-LSP failed with exit code $($probe.ExitCode): $probeError"
        }
        if (("$probeOutput`n$probeError") -notmatch 'Baa-LSP') {
            throw 'Bundled language server did not identify itself as Baa-LSP.'
        }
        $probe.Dispose()
    }

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

if ($LanguageServer) {
    Write-Host 'Qalam and its bundled language server passed the isolated runtime check.' -ForegroundColor Green
} else {
    Write-Host 'Qalam started without external Qt or MinGW runtime paths.' -ForegroundColor Green
}
