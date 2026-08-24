param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,

    [string]$LaunchPath = '',

    [string]$LanguageServer = '',

    [string]$Compiler = '',

    [string]$Nazm = '',

    [string]$ToolchainRoot = '',

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
$resolvedLaunchPath = if ($LaunchPath) {
    (Resolve-Path -LiteralPath $LaunchPath).Path
} else {
    $resolvedExecutable
}
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
$previousTemp = $env:TEMP
$previousTmp = $env:TMP
$hostTempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$runtimeTempRoot = Join-Path $hostTempRoot (
    'Qalam runtime lock - ' + [Guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($runtimeTempRoot) | Out-Null
$hadPlatform = Test-Path Env:QT_QPA_PLATFORM
$previousPlatform = $env:QT_QPA_PLATFORM
$windowsDirectory = if ($env:SystemRoot) { $env:SystemRoot } else { 'C:\Windows' }
$env:Path = "$windowsDirectory\System32;$windowsDirectory"
$env:TEMP = $runtimeTempRoot
$env:TMP = $runtimeTempRoot
Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue
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
        $resolvedToolchain = if ($ToolchainRoot) {
            (Resolve-Path -LiteralPath $ToolchainRoot).Path
        } else {
            Join-Path (Split-Path -Parent $resolvedCompiler) 'gcc'
        }
        $bundledGcc = Join-Path $resolvedToolchain 'bin\gcc.exe'
        if (!(Test-Path -LiteralPath $bundledGcc -PathType Leaf)) {
            throw "Bundled GCC linker driver is missing: $bundledGcc"
        }
        $toolchainManifest = Join-Path $resolvedToolchain 'BAA-TOOLCHAIN-MANIFEST.txt'
        if (!(Test-Path -LiteralPath $toolchainManifest -PathType Leaf)) {
            throw "Bundled Baa toolchain manifest is missing: $toolchainManifest"
        }
        $manifestLines = [IO.File]::ReadAllLines($toolchainManifest)
        $unicodeModes = @($manifestLines | Where-Object {
            $_ -in @('unicode_paths=direct', 'unicode_paths=short-path')
        })
        if ($manifestLines -notcontains 'format=baa-portable-toolchain-v1' -or
            $unicodeModes.Count -ne 1 -or
            $manifestLines -notcontains 'pei386_runtime_relocator=retain') {
            throw 'Bundled Baa toolchain has no admitted Unicode path mode.'
        }

        $gccProbeInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $gccProbeInfo.FileName = $bundledGcc
        $gccProbeInfo.Arguments = '-dumpmachine'
        $gccProbeInfo.UseShellExecute = $false
        $gccProbeInfo.CreateNoWindow = $true
        $gccProbeInfo.RedirectStandardOutput = $true
        $gccProbeInfo.RedirectStandardError = $true
        $gccProbe = [System.Diagnostics.Process]::new()
        $gccProbe.StartInfo = $gccProbeInfo
        if (!$gccProbe.Start()) { throw 'Bundled GCC linker driver could not be started.' }
        $gccOutput = $gccProbe.StandardOutput.ReadToEnd().Trim()
        $gccError = $gccProbe.StandardError.ReadToEnd()
        $gccProbe.WaitForExit()
        if ($gccProbe.ExitCode -ne 0 -or $gccOutput -ne 'x86_64-w64-mingw32') {
            throw "Bundled GCC is not a working x86_64 MinGW toolchain: $gccError"
        }
        $gccProbe.Dispose()

        # GCC locates its relocated child programs by prefix, but those child
        # programs still resolve their runtime DLLs through PATH on Windows.
        # Prepend only this package's bin directory; the host toolchain remains
        # absent from the isolated environment.
        $env:Path = "$(Join-Path $resolvedToolchain 'bin');$env:Path"

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
            $programPath = Join-Path $smokeRoot ($smokeFileStem + '.exe')
            [IO.File]::WriteAllText(
                $sourcePath,
                "${integerKeyword} ${entryPoint}() {`n    ${returnKeyword} ${arabicZero}.`n}`n",
                [Text.UTF8Encoding]::new($false))
            $compileInfo = [System.Diagnostics.ProcessStartInfo]::new()
            $compileInfo.FileName = $resolvedCompiler
            $compileInfo.Arguments = "`"$sourcePath`" -o `"$programPath`""
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
                throw "Bundled Baa/Nazm/GCC link smoke failed with exit code $($compile.ExitCode):`n$compileOutput`n$compileError"
            }
            if (!(Test-Path -LiteralPath $programPath -PathType Leaf) -or
                (Get-Item -LiteralPath $programPath).Length -le 0) {
                throw 'Bundled Baa/Nazm/GCC link smoke produced no executable.'
            }
            $compile.Dispose()

            $programInfo = [System.Diagnostics.ProcessStartInfo]::new()
            $programInfo.FileName = $programPath
            $programInfo.WorkingDirectory = $smokeRoot
            $programInfo.UseShellExecute = $false
            $programInfo.CreateNoWindow = $true
            $programInfo.RedirectStandardOutput = $true
            $programInfo.RedirectStandardError = $true
            $program = [System.Diagnostics.Process]::new()
            $program.StartInfo = $programInfo
            if (!$program.Start()) { throw 'Linked Baa smoke program could not start.' }
            $programOutput = $program.StandardOutput.ReadToEnd()
            $programError = $program.StandardError.ReadToEnd()
            $program.WaitForExit()
            if ($program.ExitCode -ne 0) {
                throw "Linked Baa smoke program failed with exit code $($program.ExitCode):`n$programOutput`n$programError"
            }
            $program.Dispose()
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

    if (-not ('QalamRuntimeWindowProbe' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class QalamRuntimeWindowProbe
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rectangle);
}
'@
    }

    # This probe verifies a real top-level IDE window. Passing SW_HIDE makes
    # Process.MainWindowHandle host-dependent and caused clean CI runners to
    # report zero even though the Qt event loop stayed alive.
    $process = Start-Process -FilePath $resolvedLaunchPath -PassThru
    $windowStartupSeconds = [Math]::Max($StartupSeconds, 15)
    $startupDeadline = [DateTime]::UtcNow.AddSeconds($windowStartupSeconds)
    do {
        Start-Sleep -Milliseconds 100
        $process.Refresh()
    } while (!$process.HasExited -and
             $process.MainWindowHandle -eq [IntPtr]::Zero -and
             [DateTime]::UtcNow -lt $startupDeadline)

    if ($process.HasExited) {
        throw "Qalam exited during runtime startup with code $($process.ExitCode)."
    }
    if ($process.MainWindowHandle -eq [IntPtr]::Zero) {
        throw 'Qalam started but created no native Windows application window.'
    }
    if ([string]::IsNullOrWhiteSpace($process.MainWindowTitle)) {
        throw 'Qalam created a Windows application window without an accessible title.'
    }

    $windowRectangle = New-Object QalamRuntimeWindowProbe+Rect
    if (-not [QalamRuntimeWindowProbe]::GetWindowRect(
            $process.MainWindowHandle, [ref]$windowRectangle)) {
        throw 'Qalam created a window whose geometry could not be inspected.'
    }
    $windowWidth = $windowRectangle.Right - $windowRectangle.Left
    $windowHeight = $windowRectangle.Bottom - $windowRectangle.Top
    if ($windowWidth -lt 640 -or $windowHeight -lt 480) {
        throw "Qalam restored an unusable ${windowWidth}x${windowHeight} window."
    }
} finally {
    if ($process -and !$process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
    $env:Path = $previousPath
    $env:TEMP = $previousTemp
    $env:TMP = $previousTmp
    if ($hadPlatform) { $env:QT_QPA_PLATFORM = $previousPlatform }
    else { Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue }
    if (Test-Path -LiteralPath $runtimeTempRoot) {
        $resolvedRuntimeTemp = [IO.Path]::GetFullPath($runtimeTempRoot)
        if (!$resolvedRuntimeTemp.StartsWith(
                $hostTempRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove runtime files outside Temp: $resolvedRuntimeTemp"
        }
        Remove-Item -LiteralPath $resolvedRuntimeTemp -Recurse -Force
    }
}

if ($LanguageServer -and $Compiler -and $Nazm) {
    Write-Host 'Qalam, Baa-LSP, Baa, Nazm, and bundled GCC passed isolated compile-link-run checks.' -ForegroundColor Green
} elseif ($LanguageServer) {
    Write-Host 'Qalam and its bundled language server passed the isolated runtime check.' -ForegroundColor Green
} else {
    Write-Host 'Qalam started without external Qt or MinGW runtime paths.' -ForegroundColor Green
}
