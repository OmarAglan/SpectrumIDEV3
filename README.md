# Qalam IDE - محرر قلم

<p align="center">
  <img src="qalam/resources/branding/icons/qalam-256.png" width="160" alt="شعار قلم">
</p>

[![Qt](https://img.shields.io/badge/Qt-6.x-41CD52?logo=qt&logoColor=white)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/23)
[![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS-2ea44f)](#build--run)

**English:** Qalam IDE is the fast, right-to-left friendly, Qt-powered IDE dedicated to **Baa (باء)**.
**العربية:** محرر **قلم** هو بيئة التطوير المخصصة للغة **باء**، سريعة ومبنية بـ Qt وتدعم اتجاه الكتابة من اليمين إلى اليسار.

---

## Why Qalam?

**English:** “Qalam” (قلم) is the symbol of writing, clarity, and craftsmanship. This project aims to bring a polished developer experience to Arabic-native programming.  
**العربية:** “قلم” يجمع بين المعنى والهوية: كتابة واضحة وتجربة تطوير احترافية للبرمجة بالعربية.

---

## Language: Baa (باء)

- **Spec:** See [`documents/LANGUAGE.md`](documents/LANGUAGE.md) (Baa Language Specification).
- **File extension (spec):** `.baa`
- **Entry point (spec):** `الرئيسية`

---

## Key Features (from the current implementation)

### Editor experience
- **Layered Baa highlighting**: the local lexer provides immediate,
  stateful multi-line coloring, then version-checked
  `textDocument/semanticTokens/full` results overlay compiler-owned types,
  keywords, modifiers, directives, comments, literals, numbers, operators,
  functions, variables, parameters, fields, and enum members.
  Stale LSP results are discarded and the local layer remains available while
  the server starts or is unavailable.
- **Theme engine** (multiple built-in code themes).
- **Arabic-first Baa-LSP completion** sourced from Baa's versioned metadata and
  cursor semantic query, including compiler builtins, parameters, visible
  locals, and explicitly included declarations, with exact UTF-16 edits,
  shadowing, stale-result cancellation, and navigable snippet tab stops.
- **Compiler-backed hover and signature help** with Arabic declarations,
  scope-correct shadowing, included prototypes, active parameters, and
  stale-result cancellation while editing.
- **Visible Baa symbol navigation** with a hierarchical outline for the current
  document and `Ctrl+T` project-symbol search. Both consume Baa-LSP results;
  Takween defines the project source closure and Baa owns every symbol identity,
  kind, detail, and location.
- **Safe compiler-owned quick fixes** through `Ctrl+.` or the editor context
  menu, with an Arabic preview and version-checked workspace edits.
- **Canonical Baa formatting** through `Shift+Alt+F`, the command palette, or
  the editor context menu. Baa owns the style, Baa-LSP returns a versioned edit,
  and Qalam applies it as one undoable operation.
- **Auto-save & crash recovery**
  - Periodic auto-save to `file.~`
  - Recovery prompt if a newer backup exists
- **Code folding (lightweight)** and **line numbers**
- **Smart editing**
  - Auto-pairing for brackets and quotes
  - Indentation helper on Enter
- **Drag & drop** open for supported files
- **Right-to-left (RTL) first**
  - RTL UI layout direction
  - RTL editor text direction & alignment

### IDE features
- **Embedded interactive console** with command history and fast flush buffering
- **Ecosystem tooling integration**: saved and unsaved diagnostics and
  hierarchical document symbols now flow through the standalone Baa-LSP server
  using Baa's versioned JSON contracts without writing shadow files. Arabic
  completion, hover, and call-signature help use that same language-service
  boundary. Project-wide definition, references, and collision-checked Arabic
  rename combine Takween's source closure with Baa's structured symbol identities.
  The explorer displays the current document's hierarchical symbol outline,
  while `Ctrl+T` searches a cached compiler-owned workspace-symbol index and
  navigates to exact UTF-16 locations, including unsaved open documents.
  Quick fixes come from Baa's structured diagnostic edits, pass through Baa-LSP
  without message parsing, and are previewed before Qalam applies them.
  Formatting comes from Baa's `format-json-v1`; Qalam does not maintain a
  competing formatter or silently apply stale edits.
  Semantic coloring combines Baa's tolerant `tokens-json-v1` with bound
  identifier roles from `semantic-index-json-v1` through Baa-LSP; Qalam keeps
  only an immediate lexical fallback and does not maintain a competing semantic
  grammar.
  Projects containing `مشروع.تكوين` can be built, run,
  tested, and cleaned through Takween from the Run menu or command palette.
  Qalam asks `takween-targets-v1` for selectable targets and consumes
  `takween-build-events-v1` for Arabic progress; `Shift+F5` cancels the active
  process. Standalone files retain the direct Baa fallback. Structured diagnostics stay
  authoritative; when none are present, Qalam classifies `compiler-cli-v1`
  exit codes without parsing human-readable messages.
- **File explorer sidebar** (QTreeView + QFileSystemModel)
- **Welcome screen**
  - Recent files list
  - New file / Open file / Open folder

---

## Project Structure

- `qalam/` — Qt application entry, main window, and resources.
- `source/` — Reusable components (editor, console, settings, menu bar, welcome window).
- `documents/` — Language specifications, user guide, and deployment notes.

---

## Build & Run

Qalam IDE is a Qt Widgets application written in C++23.

### Requirements

- Qt 6.x with the Widgets / GUI / Core modules
- CMake 3.21+
- A C++23-capable compiler
  - Windows: Qt MinGW kit is the easiest path
  - Linux: GCC 13+ recommended
  - macOS: Clang 16+ recommended

### Windows: one-command bootstrap recommended

On a fresh Windows machine, open PowerShell, Command Prompt, or Windows Terminal in the repository root and run:

```bat
build-qalam-windows.cmd
```

You can also run the PowerShell script directly:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\bootstrap-windows.ps1
```

The `.cmd` launcher uses `-ExecutionPolicy Bypass` only for the current PowerShell process, so users do not need to permanently change their Windows script policy.

The bootstrap script attempts to install the missing base tools with `winget`, installs Qt + MinGW with `aqtinstall`, builds Qalam, and creates the portable ZIP.

Output:

- Build: `build/windows-release/qalam/Qalam.exe`
- Portable package: `dist/Qalam-win64.zip`

Useful options:

```bat
REM Build only, do not create dist/Qalam-win64.zip
build-qalam-windows.cmd -NoPackage

REM Use a custom Qt install root
build-qalam-windows.cmd -QtRoot "D:\Qt"

REM Use an already installed Qt and skip downloading Qt
build-qalam-windows.cmd -SkipQtInstall -QtRoot "C:\Qt"
```

If Windows still blocks scripts after using the launcher, unblock the downloaded project files once after verifying the source:

```powershell
Get-ChildItem -Recurse -File | Unblock-File
```

### Windows: manual quick path

If Qt 6 + MinGW is already installed, run PowerShell from the repository root:

```powershell
# Optional when Qt is not installed under C:\Qt
$env:QALAM_QT_DIR = "C:\Qt\6.10.2\mingw_64"

.\scripts\build-windows.ps1 -Configuration Release
.\scripts\package-windows.ps1 -SkipBuild
```

### Windows: manual CMake path

```powershell
$env:QALAM_QT_DIR = "C:\Qt\6.10.2\mingw_64"
cmake --preset windows-release
cmake --build --preset release
```

If your Qt is exactly at `C:\Qt\6.10.2\mingw_64`, you can also use:

```powershell
cmake --preset windows-release-qt6102
cmake --build --preset release-qt6102
```

Windows tests normalize the runtime `Path` inside CTest and add the configured
Qt and MinGW directories automatically. The Windows build script also removes
duplicate `Path`/`PATH` variables before it launches CMake, MinGW, or CTest,
copies the selected MinGW runtime beside every native executable, and deploys
the matching Qt runtime beside Qalam. A normal build or test therefore does not
require manual DLL copying or terminal repair. Use `-SkipDeployAfterBuild` only
when an intentionally non-runnable intermediate application directory is
acceptable.

### Linux

On Ubuntu/Fedora/Arch-like systems, the low-hassle path is:

```bash
./scripts/bootstrap-linux.sh
```

If Qt is already installed:

```bash
./scripts/build-linux.sh Release
```

Or manually:

```bash
cmake -S . -B build/linux-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/Qt/6.x/gcc_64
cmake --build build/linux-release --parallel
```

### macOS

On macOS with Homebrew available:

```bash
./scripts/bootstrap-macos.sh
```

### Build with Qt Creator

1. Open the root `CMakeLists.txt`.
2. Select a Qt 6 kit.
3. Configure, build, and run.

### qmake fallback

`qalam/Qalam.pro` has been refreshed for the current source tree, but CMake is still the main supported build system. Use qmake only as a fallback.

---

## Deployment & Packaging

For detailed instructions on how to deploy Qalam IDE on Windows, Linux, and macOS, see [`documents/deployment.md`](documents/deployment.md).


## Keyboard Shortcuts

The following shortcuts are implemented:

| Shortcut | Action |
|---|---|
| `Ctrl+S` | Save |
| `Ctrl+F` | Find / Search bar |
| `Ctrl+G` | Go to line |
| `Ctrl+/` | Toggle comment |
| `Ctrl+D` | Duplicate line |
| `Alt+Up` | Move line up |
| `Alt+Down` | Move line down |
| `F6` | Toggle embedded console |
| `Ctrl + Mouse Wheel` | Zoom editor font in/out |
| `Ctrl+L` | Clear embedded console (focus in console) |

---

## More Documentation

- [**Baa Language Specification**](documents/LANGUAGE.md)
- [**User Guide**](documents/USER_GUIDE.md) (How to use the IDE)
- [**Internal Architecture**](documents/INTERNALS.md) (For contributors)
- [**Baa-First IDE Plan**](documents/BAA_FIRST_IDE_PLAN.md) (Active architecture and milestones)

---

## Download Qt 6

- Qt mirror list: https://download.qt.io/static/mirrorlist/
- Example installer mirror usage:
  - `NameOfQtOnlineInstaller.exe --mirror https://mirrors.ocf.berkeley.edu/qt/`


### Windows Python alias troubleshooting

If the bootstrap prints `Python was not found; run without arguments to install from the Microsoft Store`, Windows is exposing the Microsoft Store Python alias instead of a real Python install. The bootstrap script now tests Python by actually running it and ignores `WindowsApps\python.exe` aliases.

Manual recovery commands:

```powershell
winget install --id Python.Python.3.12 --exact --source winget --accept-package-agreements --accept-source-agreements
# Close and reopen PowerShell, then:
.\build-qalam-windows.cmd
```

Alternative: open Windows Settings → Apps → Advanced app settings → App execution aliases, then switch off the App Installer `python.exe` and `python3.exe` aliases.
