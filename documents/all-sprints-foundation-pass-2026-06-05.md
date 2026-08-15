# Qalam IDE — All Sprints Foundation Pass

Date: 2026-06-05

This pass turns the previous feature pile into a more maintainable IDE platform. It does not make a real Baa debugger magically exist yet, but it creates the internal models and extension points needed for the sprint roadmap.

## Fixed from deep review

- Removed the committed `aqtinstall.log` from the project tree.
- Synced `qalam/Qalam.pro` with the newer CMake feature files.
- Added a Qt Test harness in `tests/`.
- Replaced duplicated command-palette dialog logic with the reusable `QalamCommandPalette` component.
- Added `CommandRegistry` as the single source for command-palette command metadata.
- Added `DiagnosticParser` to remove compiler-output parsing from `Qalam.cpp`.
- Added `DiagnosticsModel` as the single diagnostics store for Problems, status bar, and editor underlines.
- Added `WorkspaceIndexer` as the single source for Quick Open, workspace search, definitions, and references.
- Added `BreakpointModel` as the first debug data model, ready for future gutter markers and DAP integration.

## Sprint A — Stabilize IDE foundations

Implemented foundation pieces:

- `source/core/CommandRegistry.*`
- `source/language/Diagnostic.h`
- `source/language/DiagnosticParser.*`
- `source/language/DiagnosticsModel.*`
- `source/workspace/WorkspaceIndexer.*`
- `source/debug/BreakpointModel.*`
- `tests/CMakeLists.txt`
- `tests/TestDiagnosticParser.cpp`
- `tests/TestWorkspaceIndexer.cpp`
- `tests/TestCommandRegistry.cpp`

## Sprint B — Baa language service v1

Implemented base primitives:

- symbol definition lookup moved behind `WorkspaceIndexer::findDefinition()`
- reference discovery base via `WorkspaceIndexer::findReferences()`
- Quick Open file list now comes from the same workspace index used by other features

Still recommended next: a real token-based Baa parser, document symbols, outline tree, and rename preview.

## Sprint C — Problems experience

Implemented architecture:

- compiler output parsing now goes through `DiagnosticParser`
- diagnostics storage now goes through `DiagnosticsModel`
- `Qalam` rebuilds Problems, status bar count, and editor diagnostics from one model

Still recommended next: grouping by file, severity filters, next/previous problem navigation, and problem-source labels in the UI.

## Sprint D — Editor power features

Prepared foundation through shared diagnostics and indexing. Existing editor underlines and hovers now depend on a cleaner diagnostics path.

Still recommended next: replace-in-file, project replace preview, minimap/overview ruler, indent guides, whitespace rendering toggle, and format-document placeholder.

## Sprint E — Explorer/workspace

Prepared foundation through `WorkspaceIndexer`. This avoids duplicating folder traversal logic across Quick Open, references, and future search/symbol tools.

Still recommended next: Explorer file operations, reveal active file, file icons, close all editors, recent workspaces.

## Sprint F — Running/tasks

Improved foundation by making compiler diagnostics independent from the runner UI.

Still recommended next: `tasks.json`-style Qalam task config, problem matcher config, run history, terminal profiles.

## Sprint G — Debugger foundation

Added `BreakpointModel`, the first debug data model.

Still recommended next: gutter breakpoint markers, breakpoints panel, debug toolbar state, launch configuration, and DAP-shaped adapter interface.

## Sprint H — Quality/release engineering

Added tests infrastructure and first tests.

Still recommended next: `.clang-format`, `.clang-tidy`, CI test execution, CPack installer/ZIP release generation, crash-safe settings/session saves.

## Validation in this environment

- JSON resources validated.
- Shell scripts passed `bash -n`.
- Added C++ files passed basic brace-balance checks.
- CMake configure was attempted and stopped only because Qt 6 is not installed in this sandbox (`Qt6Config.cmake` missing).

## Manual Windows test checklist

1. Rebuild with `build-qalam-windows.cmd`.
2. Run Qalam and open the Command Center.
3. Test `Ctrl+Shift+P` and confirm commands come from the unified palette.
4. Open a folder and test `Ctrl+P` Quick Open.
5. Trigger a compiler diagnostic and confirm Problems/status/editor underlines update together.
6. Test `F12` and `Shift+F12` in a project with simple Baa symbols.
7. Run CMake with `-DQALAM_BUILD_TESTS=ON` and run `ctest` on a machine with Qt Test available.
