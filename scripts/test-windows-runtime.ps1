param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [string]$LanguageServer = '',

    [string]$Compiler = '',

    [string]$Nazm = '',

    [int]$StartupSeconds = 2
)

$ErrorActionPreference = 'Stop'
$nazmVersionArgument = '--' +
    (-join [char[]](0x0625, 0x0635, 0x062F, 0x0627, 0x0631))
$smokeDirectoryPrefix =
    (-join [char[]](0x0642, 0x0644, 0x0645, 0x002D, 0x062D, 0x0632, 0x0645, 0x0629, 0x002D))
$smokeFileStem = -join [char[]](0x0631, 0x0626, 0x064A, 0x0633, 0x064A)
$integerKeyword = -join [char[]](0x0635, 0x062D, 0x064A, 0x062D)
$entryPoint = -join [char[]](0x0627, 0x0644, 0x0631, 0x0626, 0x064A, 0x0633, 0x064A, 0x0629)
$returnKeyword = -join [char[]](0x0625, 0x0631, 0x062C, 0x0639)
$arabicZero = [char]0x0660

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

    if ($Compiler) {
        $resolvedCompiler = (Resolve-Path -LiteralPath $Compiler).Path
        $compilerProbeInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $compilerProbeInfo.FileName = $resolvedCompiler
        $compilerProbeInfo.Arguments = '--version'
        $compilerProbeInfo.UseShellExecute = $false
        $compilerProbeInfo.CreateNoWindow = $true
        $compilerProbeInfo.RedirectStandardOutput = $true
        $compilerProbeInfo.RedirectStandardError = $true
        $compilerProbe = [System.Diagnostics.Process]::new()
        $compilerProbe.StartInfo = $compilerProbeInfo
        if (!$compilerProbe.Start()) {
            throw 'Bundled Baa compiler could not be started.'
        }
        $compilerOutput = $compilerProbe.StandardOutput.ReadToEnd()
        $compilerError = $compilerProbe.StandardError.ReadToEnd()
        $compilerProbe.WaitForExit()
        if ($compilerProbe.ExitCode -ne 0) {
            throw "Bundled Baa failed with exit code $($compilerProbe.ExitCode): $compilerError"
        }
        if (("$compilerOutput`n$compilerError") -notmatch 'baa version') {
            throw 'Bundled compiler did not identify itself as Baa.'
        }
        $compilerProbe.Dispose()
    }

    if ($Nazm) {
        $resolvedNazm = (Resolve-Path -LiteralPath $Nazm).Path
        $nazmProbeInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $nazmProbeInfo.FileName = $resolvedNazm
        $nazmProbeInfo.Arguments = $nazmVersionArgument
        $nazmProbeInfo.UseShellExecute = $false
        $nazmProbeInfo.CreateNoWindow = $true
        $nazmProbeInfo.RedirectStandardOutput = $true
        $nazmProbeInfo.RedirectStandardError = $true
        $nazmProbe = [System.Diagnostics.Process]::new()
        $nazmProbe.StartInfo = $nazmProbeInfo
        if (!$nazmProbe.Start()) {
            throw 'Bundled Nazm assembler could not be started.'
        }
        $nazmOutput = $nazmProbe.StandardOutput.ReadToEnd()
        $nazmError = $nazmProbe.StandardError.ReadToEnd()
        $nazmProbe.WaitForExit()
        if ($nazmProbe.ExitCode -ne 0) {
            throw "Bundled Nazm failed with exit code $($nazmProbe.ExitCode): $nazmError"
        }
        if ([string]::IsNullOrWhiteSpace("$nazmOutput$nazmError")) {
            throw 'Bundled Nazm version probe returned no identification.'
        }
        $nazmProbe.Dispose()
    }

    if ($Compiler -and $Nazm) {
        $smokeRoot = Join-Path ([IO.Path]::GetTempPath()) (
            $smokeDirectoryPrefix + [Guid]::NewGuid().ToString('N'))
        [void][IO.Directory]::CreateDirectory($smokeRoot)
        $hadBaaHome = Test-Path Env:BAA_HOME
        $previousBaaHome = $env:BAA_HOME
        $hadBaaNazm = Test-Path Env:BAA_NAZM
        $previousBaaNazm = $env:BAA_NAZM
        try {
            $env:BAA_HOME = Split-Path -Parent $resolvedCompiler
            $env:BAA_NAZM = $resolvedNazm
            $sourcePath = Join-Path $smokeRoot ($smokeFileStem + '.baa')
            $objectPath = Join-Path $smokeRoot ($smokeFileStem + '.o')
            [IO.File]::WriteAllText(
                $sourcePath,
                "${integerKeyword} ${entryPoint}() {`n    ${returnKeyword} ${arabicZero}.`n}`n",
                [Text.UTF8Encoding]::new($false))
            $compileInfo = [System.Diagnostics.ProcessStartInfo]::new()
            $compileInfo.FileName = $resolvedCompiler
            $compileInfo.Arguments = "-c `"$sourcePath`" -o `"$objectPath`""
            $compileInfo.WorkingDirectory = $smokeRoot
            $compileInfo.UseShellExecute = $false
            $compileInfo.CreateNoWindow = $true
            $compileInfo.RedirectStandardOutput = $true
            $compileInfo.RedirectStandardError = $true
            $compile = [System.Diagnostics.Process]::new()
            $compile.StartInfo = $compileInfo
            if (!$compile.Start()) { throw 'Bundled Baa object smoke test could not start.' }
            $compileOutput = $compile.StandardOutput.ReadToEnd()
            $compileError = $compile.StandardError.ReadToEnd()
            $compile.WaitForExit()
            if ($compile.ExitCode -ne 0) {
                throw "Bundled Baa/Nazm object smoke failed with exit code $($compile.ExitCode):`n$compileOutput`n$compileError"
            }
            if (!(Test-Path -LiteralPath $objectPath -PathType Leaf) -or
                (Get-Item -LiteralPath $objectPath).Length -le 0) {
                throw 'Bundled Baa/Nazm object smoke produced no object file.'
            }
            $compile.Dispose()
        } finally {
            if ($hadBaaHome) { $env:BAA_HOME = $previousBaaHome }
            else { Remove-Item Env:BAA_HOME -ErrorAction SilentlyContinue }
            if ($hadBaaNazm) { $env:BAA_NAZM = $previousBaaNazm }
            else { Remove-Item Env:BAA_NAZM -ErrorAction SilentlyContinue }
            if (Test-Path -LiteralPath $smokeRoot) {
                Remove-Item -LiteralPath $smokeRoot -Recurse -Force
            }
        }
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

if ($LanguageServer -and $Compiler -and $Nazm) {
    Write-Host 'Qalam, Baa-LSP, Baa, and Nazm passed the isolated runtime and object-generation checks.' -ForegroundColor Green
} elseif ($LanguageServer) {
    Write-Host 'Qalam and its bundled language server passed the isolated runtime check.' -ForegroundColor Green
} else {
    Write-Host 'Qalam started without external Qt or MinGW runtime paths.' -ForegroundColor Green
}
