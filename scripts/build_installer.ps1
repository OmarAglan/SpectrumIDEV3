param(
    [string]$Version = '3.6.0',
    [string]$QtRoot = $env:QALAM_QT_DIR,
    [string]$BuildDir = 'build/windows-release',
    [string]$BaaLspExecutable = $env:QALAM_BAA_LSP_PATH,
    [string]$IsccPath = '',
    [string]$SignToolName = '',
    [string]$SignToolCommand = '',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($SignToolName) -ne
    [string]::IsNullOrWhiteSpace($SignToolCommand)) {
    throw 'Pass both -SignToolName and -SignToolCommand, or neither.'
}
if (![string]::IsNullOrWhiteSpace($SignToolName) -and
    $SignToolName -notmatch '^[A-Za-z0-9_-]+$') {
    throw 'SignToolName may contain only letters, digits, underscore, and hyphen.'
}
$payload = Join-Path $root 'dist\installer-payload'
$nazmArabicExecutableName =
    (-join [char[]](0x0646, 0x0638, 0x0645)) + '.exe'

if (!$SkipBuild) {
    & (Join-Path $PSScriptRoot 'build-windows.ps1') `
        -Configuration Release `
        -QtRoot $QtRoot `
        -BuildDir $BuildDir `
        -DeployAfterBuild
    if ($LASTEXITCODE -ne 0) { throw "Qalam build failed with exit code $LASTEXITCODE." }
}

if ([string]::IsNullOrWhiteSpace($BaaLspExecutable)) {
    $BaaLspExecutable = @(
        (Join-Path $root '..\Baa-LSP\build\windows-release\baa-lsp.exe'),
        (Join-Path $root '..\Baa-LSP\build\ci-admission\baa-lsp.exe'),
        (Join-Path $root 'ecosystem\Baa-LSP\build\windows-release\baa-lsp.exe')
    ) | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
}
if ([string]::IsNullOrWhiteSpace($BaaLspExecutable) -or
    !(Test-Path -LiteralPath $BaaLspExecutable -PathType Leaf)) {
    throw 'Baa-LSP was not found. Build it or pass -BaaLspExecutable.'
}

& (Join-Path $PSScriptRoot 'package-windows.ps1') `
    -QtRoot $QtRoot `
    -BuildDir $BuildDir `
    -PackageDir $payload `
    -BaaLspExecutable $BaaLspExecutable `
    -SkipCompiler `
    -SkipBuild `
    -SkipArchive
if ($LASTEXITCODE -ne 0) { throw "Qalam payload packaging failed with exit code $LASTEXITCODE." }

$forbidden = @(
    (Join-Path $payload 'baa.exe'),
    (Join-Path $payload 'baa\baa.exe'),
    (Join-Path $payload 'takween.exe'),
    (Join-Path $payload 'nazm.exe'),
    (Join-Path $payload $nazmArabicExecutableName),
    (Join-Path $payload 'gcc\bin\gcc.exe'),
    (Join-Path $payload 'ld.exe'),
    (Join-Path $payload 'baa\gcc\bin\gcc.exe'),
    (Join-Path $payload 'baa\gcc\bin\ld.exe')
)
foreach ($path in $forbidden) {
    if (Test-Path -LiteralPath $path) {
        throw "Standalone Qalam payload contains an externally owned tool: $path"
    }
}

if ([string]::IsNullOrWhiteSpace($IsccPath)) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $IsccPath = $command.Source }
}
if ([string]::IsNullOrWhiteSpace($IsccPath)) {
    $IsccPath = @(
        "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
        'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
        'C:\Program Files\Inno Setup 6\ISCC.exe'
    ) | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
}
if ([string]::IsNullOrWhiteSpace($IsccPath) -or
    !(Test-Path -LiteralPath $IsccPath -PathType Leaf)) {
    throw 'Inno Setup 6 compiler was not found. Pass -IsccPath explicitly.'
}

$isccArguments = @(
    "/DMyAppVersion=$Version",
    "/DQalamPayloadDir=$payload"
)
if (![string]::IsNullOrWhiteSpace($SignToolName)) {
    $isccArguments += "/DInstallerSignTool=$SignToolName"
    $isccArguments += "/S$SignToolName=$SignToolCommand"
}
Push-Location $root
try {
    & $IsccPath @isccArguments setup.iss
    if ($LASTEXITCODE -ne 0) { throw "ISCC failed with exit code $LASTEXITCODE." }
}
finally {
    Pop-Location
}

$installer = Join-Path $root "dist\installer\qalam-setup-$Version-x64.exe"
if (!(Test-Path -LiteralPath $installer -PathType Leaf)) {
    throw "Qalam installer was not produced at $installer"
}
$hash = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
[IO.File]::WriteAllText(
    "$installer.sha256",
    "$hash *$([IO.Path]::GetFileName($installer))`n",
    [Text.Encoding]::ASCII)
Write-Output $installer
Write-Output "$installer.sha256"
