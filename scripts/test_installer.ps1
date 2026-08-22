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
$setupLog = Join-Path ([IO.Path]::GetTempPath()) (
    'qalam-setup-' + [Guid]::NewGuid().ToString('N') + '.log')
$uninstallLog = Join-Path ([IO.Path]::GetTempPath()) (
    'qalam-uninstall-' + [Guid]::NewGuid().ToString('N') + '.log')
$markerKey = 'HKCU:\Software\BaaEcosystem\Qalam'
$nazmArabicExecutableName =
    (-join [char[]](0x0646, 0x0638, 0x0645)) + '.exe'
$qalamArabicName = -join [char[]](0x0642, 0x0644, 0x0645)
$startMenuShortcut = Join-Path $env:APPDATA (
    'Microsoft\Windows\Start Menu\Programs\' + $qalamArabicName + '\' +
    $qalamArabicName + '.lnk')
$installed = $false

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
    Invoke-QalamInstaller $arguments
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

    $shortcutMetadata = (New-Object -ComObject WScript.Shell).CreateShortcut(
        $startMenuShortcut)
    if ($shortcutMetadata.TargetPath -ine $qalam) {
        throw 'Qalam Start Menu shortcut points to the wrong executable.'
    }
    if ($shortcutMetadata.WorkingDirectory -ine $installRoot) {
        throw 'Qalam Start Menu shortcut has the wrong working directory.'
    }

    $previousProcessPath = $env:PATH
    $toolOverrides = @('QALAM_BAA_PATH', 'QALAM_TAKWEEN_PATH',
                       'QALAM_NAZM_PATH')
    $previousOverrides = @{}
    try {
        $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
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
        foreach ($name in $toolOverrides) {
            [Environment]::SetEnvironmentVariable(
                $name, $previousOverrides[$name], 'Process')
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
