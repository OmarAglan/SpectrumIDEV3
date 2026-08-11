<p align="center">
  <img src="qalam/resources/branding/icons/qalam-256.png" width="140" alt="Qalam IDE logo">
  &nbsp;&nbsp;&nbsp;
  <img src="qalam/resources/branding/baa-language-logo.png" width="140" alt="Baa programming language logo">
</p>

<h1 align="center">Qalam IDE · محرر قلم</h1>

<p align="center">
  A dedicated, Arabic-first development environment for the Baa programming language.
</p>

Qalam is a cross-platform Qt 6 IDE designed around Baa rather than adapted from a generic editor. It combines a right-to-left interface, compiler-accurate language intelligence, and first-class Takween project workflows in one focused desktop application.

## Ecosystem architecture

Qalam keeps each project responsible for one layer:

- **Baa** is the source of truth for syntax, semantics, diagnostics, formatting, and source structure.
- **Baa-LSP** translates Baa and Takween contracts into standard Language Server Protocol features.
- **Takween** owns project discovery, targets, builds, tests, runs, and cleaning.
- **Qalam** owns the editor and user experience; it does not duplicate compiler semantics.

Unsaved documents are analyzed in memory, results are tied to the exact document version, and stale responses are discarded.

## Highlights

- Arabic-first, right-to-left editor and interface.
- Live compiler diagnostics with safe structured quick fixes.
- Completion, hover, signature help, and snippets.
- Document outline and project-wide symbol search.
- Go to definition, references, and collision-checked Arabic rename.
- Compiler-owned semantic highlighting and canonical formatting.
- Nested folding plus semantic expand/shrink selection.
- Takween targets and structured build, run, test, clean, and cancellation workflows.
- File explorer, command palette, problems panel, embedded console, themes, autosave, and crash recovery.
- Deterministic Windows runtime deployment with no recurring manual DLL or `PATH` repair.

For the detailed feature state and remaining production work, see the [roadmap](documents/ROADMAP.md).

## Build and run

Qalam requires Qt 6, CMake 3.21 or newer, and a C++23 compiler.

### Windows

For a fresh machine, the bootstrap installs missing prerequisites, builds Qalam, runs the configured checks, and can create a portable package:

```bat
build-qalam-windows.cmd
```

When Qt and MinGW are already installed:

```powershell
.\scripts\build-windows.ps1 -Configuration Release
```

The Windows scripts select one matching Qt/MinGW toolchain and deploy its runtime beside the built executables. A normal build and test run should not require copying DLLs or editing the terminal environment manually.
Production packages also build or consume the pinned standalone Baa-LSP binary,
place it under `baa-lsp/` beside Qalam, and verify both programs with external Qt
and MinGW paths removed. The Baa SDK and Takween remain separately discoverable
tools until the versioned SDK distribution track is complete.

### Linux

```sh
./scripts/bootstrap-linux.sh
```

With Qt already installed:

```sh
./scripts/build-linux.sh Release
```

### macOS

```sh
./scripts/bootstrap-macos.sh
```

Qt Creator users can open the root `CMakeLists.txt` and select a Qt 6 kit. CMake is the supported build system; the qmake project is retained only as a fallback.

## Documentation

- [User guide](documents/USER_GUIDE.md)
- [Baa language reference](documents/LANGUAGE.md)
- [Baa-LSP integration](documents/BAA_LSP_INTEGRATION_AR.md)
- [Takween integration](documents/TAKWEEN_INTEGRATION.md)
- [Internal architecture](documents/INTERNALS.md)
- [Deployment and packaging](documents/deployment.md)
- [Branding assets](documents/BRANDING.md)
- [Roadmap](documents/ROADMAP.md)

Qalam is under active development with Baa, Baa-LSP, and Takween. Features are considered complete only after their compiler, protocol, editor, and cross-platform integration gates pass together.
