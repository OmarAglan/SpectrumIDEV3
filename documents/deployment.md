# Deployment Guide

## Windows execution policy note

If PowerShell reports that `scripts\bootstrap-windows.ps1` is not digitally signed, use the repository-root launcher instead:

```bat
build-qalam-windows.cmd
```

Or run the script with a process-only policy bypass:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\bootstrap-windows.ps1
```

For projects extracted from a downloaded ZIP, Windows may mark the files as internet-downloaded. After reviewing the files and trusting the source, you can unblock them once:

```powershell
Get-ChildItem -Recurse -File | Unblock-File
```


This document explains how to build and package Qalam IDE on Windows, Linux, and macOS.

## Prerequisites

- Qt 6.x with Widgets / GUI / Core / SVG installed
- CMake 3.21+
- C++23 compiler
  - Windows: Qt MinGW kit recommended
  - Linux: GCC 13+ recommended
  - macOS: Clang 16+ recommended

---

## Windows: Build and Package

### Release installer (recommended)

The standalone Windows release contains only the Qalam application, its exact
Qt/MinGW runtime, and the internal Baa-LSP executable. It deliberately does not
contain Baa, Takween, Nazm, GCC, or LD, and it never changes `PATH`.

With Qalam and Baa-LSP already built:

```powershell
.\scripts\build_installer.ps1 `
  -BuildDir build\windows-release `
  -BaaLspExecutable ..\Baa-LSP\build\windows-release\baa-lsp.exe
```

Outputs:

- `dist/installer/qalam-setup-3.5.0-x64.exe`
- `dist/installer/qalam-setup-3.5.0-x64.exe.sha256`

The default installation is per-machine under `Program Files\Qalam`.
`/CURRENTUSER` selects an unelevated per-user installation. Validate the actual
installer, native Qt window, internal language server, unchanged `PATH`, and
uninstall cleanup with:

```powershell
.\scripts\test_installer.ps1
```

See [INSTALLER.md](INSTALLER.md) for the release contract.

### Developer bootstrap and portable archive

From the repository root:

```powershell
.\scripts\bootstrap-windows.ps1
```

The bootstrap script prepares a development checkout and can produce a portable
archive. The archive path remains for compatibility and integration testing; it
is not the independently owned release installer.

1. Checks for Python and CMake.
2. Uses `winget` to install missing base tools when possible.
3. Installs `aqtinstall` with Python.
4. Downloads Qt 6 + MinGW into `C:\Qt` by default.
5. Selects the MinGW toolchain installed for that Qt kit; an unrelated `g++.exe`
   already present on `PATH` is not accepted as the kit compiler.
6. Builds Qalam.
7. Builds Baa-LSP from a sibling checkout (or consumes an explicit executable).
8. Unless `-SkipCompiler` is supplied, builds Baa and its runtime from a sibling
   checkout (or consumes an explicit compiler), then copies the matching stdlib.
9. Unless `-SkipCompiler` is supplied, builds the pinned Nazm assembler.
10. For a combined developer archive, copies the complete relocatable MinGW-w64 kit under `baa/gcc/`, including
    its licenses, so Baa never selects an arbitrary linker from `PATH`.
11. Runs isolated compile-link-run checks with external Qt, MinGW, and MSYS2
    paths removed, then creates the portable ZIP.

Outputs:

- Executable: `build/windows-release/qalam/Qalam.exe`
- Portable folder: `dist/Qalam-win64/`
- ZIP package: `dist/Qalam-win64.zip`

Useful options:

```powershell
.\scripts\bootstrap-windows.ps1 -NoPackage
.\scripts\bootstrap-windows.ps1 -QtRoot "D:\Qt"
.\scripts\bootstrap-windows.ps1 -SkipWinget
.\scripts\bootstrap-windows.ps1 -SkipQtInstall -QtRoot "C:\Qt"
.\scripts\bootstrap-windows.ps1 -BuildDir "build\fresh-package"
.\scripts\bootstrap-windows.ps1 -BaaLspSourceDir "..\Baa-LSP"
.\scripts\bootstrap-windows.ps1 -BaaLspExecutable "D:\tools\baa-lsp.exe"
.\scripts\bootstrap-windows.ps1 -BaaSourceDir "..\Baa"
.\scripts\bootstrap-windows.ps1 -BaaCompilerExecutable "D:\tools\baa.exe" -BaaSourceDir "..\Baa"
.\scripts\bootstrap-windows.ps1 -NazmSourceDir "..\Nazm"
```

`package-windows.ps1` runs `windeployqt.exe` and copies the exact MinGW runtime
recorded in `CMakeCache.txt`. By default it can still construct the historical
combined developer archive. `build_installer.ps1` always passes
`-SkipCompiler`, rejects externally owned tool executables in its payload, and
therefore enforces the standalone release boundary. `-SkipLanguageServer` is
valid only for an intentional UI-only development package.

To verify a built or packaged executable with no Qt or MinGW directories on
`PATH`:

```powershell
.\scripts\test-windows-runtime.ps1 `
  -Executable .\dist\Qalam-win64\Qalam.exe `
  -LanguageServer .\dist\Qalam-win64\baa-lsp\baa-lsp.exe `
  -Compiler .\dist\Qalam-win64\baa\baa.exe `
  -Nazm .\dist\Qalam-win64\baa\نظم.exe
```

The verifier removes external Qt and MinGW directories from `PATH`, starts the
real Windows platform plugin, and requires a titled native Qalam window with a
usable geometry. It then verifies Baa-LSP and the bundled Baa → Nazm → GCC/LD
compile-link-run path. Qt's offscreen backend is not accepted as evidence that
the desktop application can launch.

### Option B: manual PowerShell scripts

Use this when Qt 6 with the MinGW kit is already installed:

```powershell
# Optional when Qt is not installed under C:\Qt
$env:QALAM_QT_DIR = "C:\Qt\6.10.2\mingw_64"

.\scripts\build-windows.ps1 -Configuration Release
.\scripts\package-windows.ps1 -SkipBuild `
  -BaaLspExecutable "..\Baa-LSP\build\windows-release\baa-lsp.exe" `
  -BaaCompilerExecutable "..\Baa\build\windows-release\baa.exe" `
  -BaaSourceDir "..\Baa" `
  -NazmExecutable "..\Nazm\build\windows-release\نظم.exe" `
  -GccRoot "C:\Qt\Tools\mingw1310_64"
```

### Option C: CMake presets

When Qt is not in the default `C:\Qt\6.10.2\mingw_64` location:

```powershell
$env:QALAM_QT_DIR = "C:\Qt\6.10.2\mingw_64"
cmake --preset windows-release
cmake --build --preset release
.\scripts\package-windows.ps1 -SkipBuild
```

When Qt is exactly in `C:\Qt\6.10.2\mingw_64` and MinGW is exactly in `C:\Qt\Tools\mingw1310_64`:

```powershell
cmake --preset windows-release-qt6102
cmake --build --preset release-qt6102
.\scripts\package-windows.ps1 -SkipBuild
```

### Runtime deployment during development

Windows builds deploy the matching Qt and MinGW runtime beside Qalam by
default. This makes the development executable independent of other compiler
toolchains on `PATH`. To skip Qt deployment for an intentionally non-runnable
intermediate build:

```powershell
.\scripts\build-windows.ps1 -Configuration Release -SkipDeployAfterBuild
```

### Tool discovery on Windows

Qalam resolves Baa, Takween, and Nazm independently in this order:

1. The explicit executable saved under **Settings → الأدوات**.
2. `QALAM_BAA_PATH`, `QALAM_TAKWEEN_PATH`, or `QALAM_NAZM_PATH`.
3. The installed command on `PATH`.
4. A legacy beside-Qalam portable layout.

The standalone installers for Baa, Takween, and Nazm own their commands and
`PATH` entries. Qalam consumes those installations but does not copy, upgrade,
or uninstall them. The internal Baa-LSP remains private to Qalam.

For legacy portable developer archives, place the compiler here:

```text
Qalam-win64/
  Qalam.exe
  baa/
    baa.exe
    نظم.exe
    libbaa_runtime.a
    stdlib/
    gcc/
      bin/
      lib/
      libexec/
      licenses/
      x86_64-w64-mingw32/
```

`baa/gcc/BAA-TOOLCHAIN-MANIFEST.txt` records the target, GCC version, SHA-256
identity, and admitted direct-Unicode path mode of the packaged linker driver.
The portable acceptance gate removes all external toolchain paths, compiles a
source from an Arabic-named temporary directory, links it with this tree, and
executes the result. A package that merely produces an object file is not
considered portable.

---

## Windows Application Icon

The Windows executable icon is configured through `qalam/resources/Qalam.rc` and included by CMake. The `.qrc` file still embeds icons for use inside the running Qt application, but the `.rc` file is what gives `Qalam.exe` its Explorer/taskbar icon.

---

## Linux

### One-command bootstrap

```bash
./scripts/bootstrap-linux.sh
```

The Linux bootstrap installs common compiler/CMake/Python packages through `apt`, `dnf`, or `pacman` when available, installs Qt through `aqtinstall`, and builds `build/linux-release/qalam/Qalam`.

### Manual distro-package dependencies

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-svg-dev libxcb-cursor0 libxcb-cursor-dev
```

### Manual build

```bash
./scripts/build-linux.sh Release
```

Or manually:

```bash
cmake -S . -B build/linux-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/Qt/6.x/gcc_64
cmake --build build/linux-release --parallel
```

### Runtime compiler layout

Place the Baa compiler beside the executable:

```text
build/linux-release/qalam/
  Qalam
  baa/
    baa
    نظم
    libbaa_runtime.a
    stdlib/
```

Make it executable:

```bash
chmod +x build/linux-release/qalam/baa/baa
```

---

## macOS

### One-command bootstrap

```bash
./scripts/bootstrap-macos.sh
```

The macOS bootstrap expects Homebrew and Xcode Command Line Tools, installs CMake/Python/aqtinstall, downloads Qt, and builds `build/macos-release/qalam/Qalam.app`.

### Manual build

```bash
export QALAM_QT_DIR=/Users/$USER/Qt/6.x/macos
cmake --preset macos-release
cmake --build --preset macos-release
macdeployqt build/macos-release/qalam/Qalam.app
```

Runtime compiler layout inside or beside the app should be finalized when macOS packaging is ready.

---

## qmake Fallback

`qalam/Qalam.pro` has been refreshed and includes the current source tree, Windows native libraries, and the Windows icon. However, CMake remains the primary supported build system.

```powershell
cd qalam
qmake Qalam.pro
mingw32-make -j
```

---

## Continuous Integration

The repository includes `.github/workflows/build.yml` for Windows and Linux.
The Windows job builds the pinned internal Baa-LSP, creates the standalone
Qalam installer, verifies its checksum, performs a silent per-user lifecycle,
launches a real native Qalam window, checks Baa-LSP, proves `PATH` is unchanged,
and uploads the installer plus its checksum. It does not check out or package
Baa, Takween, or Nazm. The Linux archive remains a developer integration
artifact until a native Linux installer contract is defined.

---

## Known Issues / Next Work

1. **Windows terminal:** The embedded terminal starts `cmd.exe` using UTF-8 code
   page setup; a future terminal layer should support PowerShell and improve
   process-session behavior.
2. **CI:** Windows and Linux builds are present; macOS release packaging remains
   future work.
3. **Portable archive:** The combined developer archive is retained for
   compatibility and cross-project integration tests. New Windows users should
   install the independently versioned ecosystem tools or the offline Developer
   Kit bootstrapper.

---

*[← Back to User Guide](USER_GUIDE.md) | [→ Compiler Internals](INTERNALS.md)*

## Windows Python alias issue

On some Windows machines, `python.exe` exists only as a Microsoft Store/App Installer alias. This makes `Get-Command python.exe` succeed even though Python is not actually usable. `scripts/bootstrap-windows.ps1` now validates Python by running a small `python -c` command, skips `WindowsApps\python.exe`, and fails immediately if `pip` or `aqtinstall` cannot run.

Recovery command:

```powershell
winget install --id Python.Python.3.12 --exact --source winget --accept-package-agreements --accept-source-agreements
```

After installing, close and reopen PowerShell so the updated PATH is loaded, then run `build-qalam-windows.cmd`.
