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

- Qt 6.x with Widgets / GUI / Core installed
- CMake 3.21+
- C++23 compiler
  - Windows: Qt MinGW kit recommended
  - Linux: GCC 13+ recommended
  - macOS: Clang 16+ recommended

---

## Windows: Build and Package

### Option A: one-command bootstrap (recommended)

From the repository root:

```powershell
.\scripts\bootstrap-windows.ps1
```

The bootstrap script does the full local setup:

1. Checks for Python and CMake.
2. Uses `winget` to install missing base tools when possible.
3. Installs `aqtinstall` with Python.
4. Downloads Qt 6 + MinGW into `C:\Qt` by default.
5. Selects the MinGW toolchain installed for that Qt kit; an unrelated `g++.exe`
   already present on `PATH` is not accepted as the kit compiler.
6. Builds Qalam.
7. Builds Baa-LSP from a sibling checkout (or consumes an explicit executable).
8. Runs the packaging and isolated-runtime checks to create a portable ZIP.

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
```

The packaging script runs `windeployqt.exe` for Qt libraries and plugins, then
copies the exact MinGW runtime recorded in the build's `CMakeCache.txt`.
It never selects compiler DLLs from the caller's `PATH`. A production package
also requires Baa-LSP and copies it to `baa-lsp/baa-lsp.exe`, the path Qalam
discovers automatically. `-SkipLanguageServer` exists only for an intentional
UI-only development package.

To verify a built or packaged executable with no Qt or MinGW directories on
`PATH`:

```powershell
.\scripts\test-windows-runtime.ps1 `
  -Executable .\dist\Qalam-win64\Qalam.exe `
  -LanguageServer .\dist\Qalam-win64\baa-lsp\baa-lsp.exe
```

### Option B: manual PowerShell scripts

Use this when Qt 6 with the MinGW kit is already installed:

```powershell
# Optional when Qt is not installed under C:\Qt
$env:QALAM_QT_DIR = "C:\Qt\6.10.2\mingw_64"

.\scripts\build-windows.ps1 -Configuration Release
.\scripts\package-windows.ps1 -SkipBuild `
  -BaaLspExecutable "..\Baa-LSP\build\windows-release\baa-lsp.exe"
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

### Baa compiler location on Windows

The IDE now resolves the Baa compiler in this order:

1. The configured compiler path from settings.
2. `baa/baa.exe` next to `Qalam.exe`.
3. `baa.exe` next to `Qalam.exe`.
4. `baa.exe` or `baa` from `PATH`.

For portable releases, place the compiler here:

```text
Qalam-win64/
  Qalam.exe
  baa/
    baa.exe
```

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
sudo apt install -y build-essential cmake qt6-base-dev qt6-base-dev-tools libxcb-cursor0 libxcb-cursor-dev
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
Both jobs build the pinned Baa-LSP revision and publish combined Qalam + Baa-LSP
artifacts. Windows uploads a runtime-complete ZIP; Linux uploads a `.tar.gz`
whose Qt libraries are provided by the target distribution.
The workflow pins Baa-LSP commit
`c72997e88ad2e7ccd75c547bd49abedd68e133aa`, including dynamic Takween
workspace folders and manifest/lock refresh.

The admitted receipt is
[run 31509433467](https://github.com/OmarAglan/Qalam-IDE/actions/runs/31509433467),
which uploaded `Qalam-win64` and `Qalam-linux-x86_64` after both jobs passed.

---

## Known Issues / Next Work

1. **Compiler settings UI:** The key already exists (`compilerPath`), but the settings window still needs a proper compiler path picker.
2. **Windows terminal:** The embedded terminal starts `cmd.exe` using UTF-8 code page setup, but a future terminal layer should support PowerShell and better ANSI color rendering.
3. **CI:** Windows and Linux GitHub Actions builds are included; macOS CI packaging can be added later.
4. **Packaging installer:** Current Windows packaging creates a portable ZIP. A proper installer can be added later with Qt Installer Framework or another installer system.
5. **Baa SDK distribution:** Baa-LSP is bundled now, but Baa, Nazm, `baalib`,
   target files, and Takween belong to the separate versioned SDK/tool
   environment. Until that track lands, users select an installed compiler and
   Takween through discovery/settings rather than receiving an unversioned copy
   inside the Qalam archive.

---

*[← Back to User Guide](USER_GUIDE.md) | [→ Compiler Internals](INTERNALS.md)*

## Windows Python alias issue

On some Windows machines, `python.exe` exists only as a Microsoft Store/App Installer alias. This makes `Get-Command python.exe` succeed even though Python is not actually usable. `scripts/bootstrap-windows.ps1` now validates Python by running a small `python -c` command, skips `WindowsApps\python.exe`, and fails immediately if `pip` or `aqtinstall` cannot run.

Recovery command:

```powershell
winget install --id Python.Python.3.12 --exact --source winget --accept-package-agreements --accept-source-agreements
```

After installing, close and reopen PowerShell so the updated PATH is loaded, then run `build-qalam-windows.cmd`.
