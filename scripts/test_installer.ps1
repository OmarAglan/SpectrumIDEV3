param(
    [string]$Version = '3.3.0',
    [string]$InstallerPath = '',
    [int]$StartupSeconds = 2
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($InstallerPath)) {
    $InstallerPath = Join-Path $root "dist\installer\qalam-setup-$Version-x64.exe"
}
$InstallerPath = (Resolve-Path -LiteralPath $InstallerPath).Path
$checksumPath = "$InstallerPath.sha256"
if (!(Test-Path -LiteralPath $checksumPath -PathType Leaf)) {
    throw "Installer checksum is missing: $checksumPath"
}
$checksumLine = [IO.File]::ReadAllText(
    $checksumPath, [Text.Encoding]::ASCII).Trim()
if ($checksumLine -notmatch '^([0-9A-Fa-f]{64}) \*(.+)$') {
    throw 'Qalam installer checksum file has an invalid format.'
}
if ($Matches[2] -cne [IO.Path]::GetFileName($InstallerPath)) {
    throw 'Qalam installer checksum names the wrong file.'
}
$expectedHash = $Matches[1]
$actualHash = (Get-FileHash -LiteralPath $InstallerPath -Algorithm SHA256).Hash
if ($expectedHash -ne $actualHash) { throw 'Qalam installer checksum mismatch.' }

$userPathBefore = [Environment]::GetEnvironmentVariable('Path', 'User')
$machinePathBefore = [Environment]::GetEnvironmentVariable('Path', 'Machine')
$installRoot = Join-Path ([IO.Path]::GetTempPath()) (
    'Qalam installer - ' + [Guid]::NewGuid().ToString('N'))
$runtimeTemp = Join-Path ([IO.Path]::GetTempPath()) (
    'Qalam runtime - ' + [Guid]::NewGuid().ToString('N'))
$setupLog = Join-Path ([IO.Path]::GetTempPath()) (
    'qalam-setup-' + [Guid]::NewGuid().ToString('N') + '.log')
$uninstallLog = Join-Path ([IO.Path]::GetTempPath()) (
    'qalam-uninstall-' + [Guid]::NewGuid().ToString('N') + '.log')
$markerKey = 'HKCU:\Software\BaaEcosystem\Qalam'
$nazmArabicExecutableName =
    (-join [char[]](0x0646, 0x0638, 0x0645)) + '.exe'
$qalamArabicName = -join [char[]](0x0642, 0x0644, 0x0645)
$baaSourceExtension = '.' + (-join [char[]](0x0628, 0x0627, 0x0621))
$baaHeaderExtension = '.' + (-join [char[]](
    0x0631, 0x0623, 0x0633, 0x0628, 0x0627, 0x0621))
$nazmSourceExtension = '.' + (-join [char[]](0x0646, 0x0638, 0x0645))
$startMenuShortcut = Join-Path $env:APPDATA (
    'Microsoft\Windows\Start Menu\Programs\' + $qalamArabicName + '\' +
    $qalamArabicName + '.lnk')
$installed = $false

if (-not ('QalamShortcutReader' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;

[ComImport]
[Guid("00021401-0000-0000-C000-000000000046")]
public class QalamShellLinkObject
{
}

[ComImport]
[Guid("000214F9-0000-0000-C000-000000000046")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
public interface IQalamShellLinkW
{
    void GetPath([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder path,
                 int pathCapacity, IntPtr findData, uint flags);
    void GetIDList(out IntPtr itemIdList);
    void SetIDList(IntPtr itemIdList);
    void GetDescription([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder text,
                        int textCapacity);
    void SetDescription([MarshalAs(UnmanagedType.LPWStr)] string text);
    void GetWorkingDirectory(
        [Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder directory,
        int directoryCapacity);
    void SetWorkingDirectory([MarshalAs(UnmanagedType.LPWStr)] string directory);
    void GetArguments([Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder arguments,
                      int argumentCapacity);
    void SetArguments([MarshalAs(UnmanagedType.LPWStr)] string arguments);
    void GetHotkey(out short hotkey);
    void SetHotkey(short hotkey);
    void GetShowCmd(out int showCommand);
    void SetShowCmd(int showCommand);
    void GetIconLocation(
        [Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder iconPath,
        int iconPathCapacity, out int iconIndex);
    void SetIconLocation([MarshalAs(UnmanagedType.LPWStr)] string iconPath,
                         int iconIndex);
    void SetRelativePath([MarshalAs(UnmanagedType.LPWStr)] string path,
                         uint reserved);
    void Resolve(IntPtr window, uint flags);
    void SetPath([MarshalAs(UnmanagedType.LPWStr)] string path);
}

public sealed class QalamShortcutDetails
{
    public string TargetPath { get; set; }
    public string WorkingDirectory { get; set; }
}

public static class QalamShortcutReader
{
    public static QalamShortcutDetails Read(string shortcutPath)
    {
        object linkObject = new QalamShellLinkObject();
        try
        {
            var link = (IQalamShellLinkW)linkObject;
            ((IPersistFile)linkObject).Load(shortcutPath, 0);
            var target = new StringBuilder(32768);
            var workingDirectory = new StringBuilder(32768);
            link.GetPath(target, target.Capacity, IntPtr.Zero, 4);
            link.GetWorkingDirectory(workingDirectory, workingDirectory.Capacity);
            return new QalamShortcutDetails {
                TargetPath = target.ToString(),
                WorkingDirectory = workingDirectory.ToString()
            };
        }
        finally
        {
            if (Marshal.IsComObject(linkObject)) {
                Marshal.FinalReleaseComObject(linkObject);
            }
        }
    }
}
'@
}

function Wait-InstallerState {
    param([string]$Description, [scriptblock]$Condition)
    for ($attempt = 0; $attempt -lt 300; $attempt++) {
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 200
    }
    throw "Timed out waiting for $Description."
}

function Invoke-QalamInstaller {
    param([string[]]$Arguments)
    $install = Start-Process -FilePath $InstallerPath -ArgumentList $Arguments `
        -WindowStyle Hidden -Wait -PassThru
    if ($install.ExitCode -ne 0) {
        throw "Qalam installer failed with exit code $($install.ExitCode). Log: $setupLog"
    }
}

function Assert-OpenWithRegistration {
    param([string]$Extension, [string]$ProgramId)
    $key = "HKCU:\Software\Classes\$Extension\OpenWithProgids"
    if (!(Test-Path -LiteralPath $key)) {
        throw "Qalam file association key is missing: $Extension"
    }
    $properties = Get-ItemProperty -LiteralPath $key
    if ($properties.PSObject.Properties.Name -notcontains $ProgramId) {
        throw "Qalam Open With registration is missing: $Extension -> $ProgramId"
    }
}

try {
    $arguments = @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-',
        '/CURRENTUSER', "/DIR=`"$installRoot`"", "/LOG=`"$setupLog`""
    )
    Invoke-QalamInstaller $arguments
    $installed = $true

    $qalam = Join-Path $installRoot 'Qalam.exe'
    $lsp = Join-Path $installRoot 'baa-lsp\baa-lsp.exe'
    $uninstaller = Join-Path $installRoot 'unins000.exe'
    Wait-InstallerState 'Qalam installation' {
        (Test-Path -LiteralPath $qalam -PathType Leaf) -and
        (Test-Path -LiteralPath $lsp -PathType Leaf) -and
        (Test-Path -LiteralPath $uninstaller -PathType Leaf) -and
        (Test-Path -LiteralPath $startMenuShortcut -PathType Leaf) -and
        (Test-Path -LiteralPath $markerKey)
    }
    $staleUpgradeFile = Join-Path $installRoot 'platforms\removed-by-upgrade.tmp'
    [IO.File]::WriteAllText($staleUpgradeFile, 'stale')
    Invoke-QalamInstaller $arguments
    if (Test-Path -LiteralPath $staleUpgradeFile) {
        throw 'Qalam repair did not remove an obsolete Qt plugin.'
    }
    $marker = Get-ItemProperty -LiteralPath $markerKey
    if ($marker.Version -ne $Version -or
        $marker.InstallLocation -ine $installRoot) {
        throw 'Qalam installer did not record its installed version and location.'
    }
    Assert-OpenWithRegistration $baaSourceExtension 'Qalam.BaaSource'
    Assert-OpenWithRegistration $baaHeaderExtension 'Qalam.BaaHeader'
    Assert-OpenWithRegistration '.baa' 'Qalam.BaaSource'
    Assert-OpenWithRegistration '.baahd' 'Qalam.BaaHeader'
    Assert-OpenWithRegistration $nazmSourceExtension 'Qalam.NazmSource'
    foreach ($required in @(
        $qalam, $lsp,
        (Join-Path $installRoot 'Qt6Core.dll'),
        (Join-Path $installRoot 'Qt6Gui.dll'),
        (Join-Path $installRoot 'Qt6Widgets.dll'),
        (Join-Path $installRoot 'platforms\qwindows.dll')
    )) {
        if (!(Test-Path -LiteralPath $required -PathType Leaf)) {
            throw "Installed Qalam file is missing: $required"
        }
    }

    $forbiddenNames = @(
        'baa.exe', 'takween.exe', 'nazm.exe', $nazmArabicExecutableName,
        'gcc.exe', 'ld.exe'
    )
    $forbidden = Get-ChildItem -LiteralPath $installRoot -Recurse -File |
        Where-Object { $forbiddenNames -contains $_.Name.ToLowerInvariant() }
    if ($forbidden) {
        throw "Qalam installed an externally owned tool: $($forbidden.FullName -join ', ')"
    }

    if ([Environment]::GetEnvironmentVariable('Path', 'User') -ne $userPathBefore -or
        [Environment]::GetEnvironmentVariable('Path', 'Machine') -ne $machinePathBefore) {
        throw 'Qalam installer modified PATH.'
    }

    $shortcutMetadata = [QalamShortcutReader]::Read($startMenuShortcut)
    if ([string]::IsNullOrWhiteSpace($shortcutMetadata.TargetPath) -or
        [string]::IsNullOrWhiteSpace($shortcutMetadata.WorkingDirectory)) {
        throw 'Qalam Start Menu shortcut has incomplete Unicode metadata.'
    }
    $shortcutTarget = (Get-Item -LiteralPath $shortcutMetadata.TargetPath).FullName
    $installedQalam = (Get-Item -LiteralPath $qalam).FullName
    if ($shortcutTarget -ine $installedQalam) {
        throw 'Qalam Start Menu shortcut points to the wrong executable.'
    }
    $shortcutWorkingDirectory = (
        Get-Item -LiteralPath $shortcutMetadata.WorkingDirectory).FullName
    $installedRoot = (Get-Item -LiteralPath $installRoot).FullName
    if ($shortcutWorkingDirectory -ine $installedRoot) {
        throw 'Qalam Start Menu shortcut has the wrong working directory.'
    }

    $previousProcessPath = $env:PATH
    $previousTemp = $env:TEMP
    $previousTmp = $env:TMP
    [IO.Directory]::CreateDirectory($runtimeTemp) | Out-Null
    $toolOverrides = @('QALAM_BAA_PATH', 'QALAM_TAKWEEN_PATH',
                       'QALAM_NAZM_PATH')
    $previousOverrides = @{}
    try {
        $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
        $env:TEMP = $runtimeTemp
        $env:TMP = $runtimeTemp
        foreach ($name in $toolOverrides) {
            $previousOverrides[$name] = [Environment]::GetEnvironmentVariable(
                $name, 'Process')
            Remove-Item "Env:$name" -ErrorAction SilentlyContinue
        }
        & (Join-Path $PSScriptRoot 'test-windows-runtime.ps1') `
            -Executable $qalam `
            -LaunchPath $startMenuShortcut `
            -LanguageServer $lsp `
            -StartupSeconds $StartupSeconds
    }
    finally {
        $env:PATH = $previousProcessPath
        $env:TEMP = $previousTemp
        $env:TMP = $previousTmp
        foreach ($name in $toolOverrides) {
            [Environment]::SetEnvironmentVariable(
                $name, $previousOverrides[$name], 'Process')
        }
        if (Test-Path -LiteralPath $runtimeTemp) {
            $resolvedRuntimeTemp = [IO.Path]::GetFullPath($runtimeTemp)
            $resolvedTempRoot = [IO.Path]::GetFullPath(
                [IO.Path]::GetTempPath())
            if (!$resolvedRuntimeTemp.StartsWith(
                    $resolvedTempRoot,
                    [StringComparison]::OrdinalIgnoreCase)) {
                throw "Refusing to remove runtime files outside Temp: $resolvedRuntimeTemp"
            }
            Remove-Item -LiteralPath $resolvedRuntimeTemp -Recurse -Force
        }
    }

    if (!(Test-Path -LiteralPath $uninstaller -PathType Leaf)) {
        throw 'Qalam uninstaller is missing.'
    }
    $uninstall = Start-Process -FilePath $uninstaller -ArgumentList @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART',
        "/LOG=`"$uninstallLog`""
    ) -WindowStyle Hidden -Wait -PassThru
    if ($uninstall.ExitCode -ne 0) {
        throw "Qalam uninstaller failed with exit code $($uninstall.ExitCode)."
    }
    Wait-InstallerState 'Qalam uninstall cleanup' {
        !(Test-Path -LiteralPath $installRoot) -and
        !(Test-Path -LiteralPath $markerKey)
    }
    foreach ($programId in @('Qalam.BaaSource', 'Qalam.BaaHeader',
                              'Qalam.NazmSource')) {
        if (Test-Path -LiteralPath "HKCU:\Software\Classes\$programId") {
            throw "Qalam uninstaller left a file association: $programId"
        }
    }
    if ([Environment]::GetEnvironmentVariable('Path', 'User') -ne $userPathBefore -or
        [Environment]::GetEnvironmentVariable('Path', 'Machine') -ne $machinePathBefore) {
        throw 'Qalam uninstall changed PATH.'
    }
}
finally {
    if ($installed -and (Test-Path -LiteralPath $installRoot)) {
        $cleanupUninstaller = Join-Path $installRoot 'unins000.exe'
        if (Test-Path -LiteralPath $cleanupUninstaller -PathType Leaf) {
            Start-Process -FilePath $cleanupUninstaller -ArgumentList @(
                '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
            ) -WindowStyle Hidden -Wait | Out-Null
        }
    }
}

if (Test-Path -LiteralPath $markerKey) {
    throw 'Qalam uninstaller left its ownership marker.'
}
Write-Output 'Qalam installer contract passed.'
