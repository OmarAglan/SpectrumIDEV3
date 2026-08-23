# Qalam IDE Internals

This document describes the internal architecture and component relationships of the Qalam IDE.

## High-Level Architecture

Qalam IDE is built using **Qt 6 (C++23)** and follows a modular design. The project is split into two main parts:

1. **`qalam_core` (Static Library):** Contains all the reusable logic and UI components (Editor, Console, Settings, etc.).
2. **`Qalam` (Application):** The main entry point that assembles the components into the final IDE.

## Directory Structure

```
/documents              # Documentation files
/qalam                  # Main application entry (main.cpp, Qalam.cpp, Qalam.h, resources.qrc)
/source                 # qalam_core static library
├── texteditor          # Custom editor components
│   ├── highlighter     # QalamLexer (state-machine), QalamSyntaxHighlighter, LanguageDefinition
│   └── autocomplete    # AutoComplete strategies and UI
├── console             # QalamConsole, ProcessWorker
├── components          # QalamFlatButton, QalamSearchPanel
├── menubar             # QalamMenuBar
├── settings            # QalamSettings
├── sidebar             # QalamExplorerView, QalamSearchView
├── workspace           # WorkspaceIndexer, ProjectSearchService
├── ui                  # QalamWindow, QalamTheme, QalamTitleBar, QalamActivityBar, QalamSidebar, QalamPanelArea, QalamStatusBar, QalamBreadcrumb
├── managers            # FileManager, BuildManager, SessionManager, LayoutManager
└── pages               # QalamWelcomePage
```

### Key Components

#### 1. Text Editor (`source/texteditor`)
The heart of Qalam is `QalamEditor`, a custom `QPlainTextEdit` subclass.
- **`QalamLexer`:** A state-based lexer using `QStringView` for zero-copy syntax highlighting.
- **`QalamSyntaxHighlighter`:** Integrates `QalamLexer` with Qt's `QSyntaxHighlighter` for real-time coloring.
- **Completion UI:** `QalamEditor` requests `textDocument/completion` after Arabic
  input, and displays only version-matched Baa-LSP results. Those results
  include Baa-owned builtins, parameters, visible locals, and explicitly
  included declarations. It does not own a copied keyword, builtin, snippet,
  or scope table.
- **Semantic tooltips:** delayed identifier hover requests and call-signature
  requests flow through Baa-LSP. `QalamEditor` renders version-matched Markdown and
  the active Arabic parameter, but owns no keyword, builtin, type, or signature
  rules.
- **`QalamBracketHandler`:** Handles bracket/quote auto-pairing, skip-over, and selection wrapping.
- **`QalamSnippetManager`:** Manages code snippets with Tab/Enter placeholder navigation.
- **`QalamAutoSave`:** Handles automatic backup to `.~` files.
- **`QalamSearchPanel`:** Owns current-document literal/regex matching,
  Unicode-aware Arabic word boundaries, wrapped navigation, replacement, and
  match counts. It publishes position/length ranges to `QalamEditor`, whose
  single decoration compositor layers search backgrounds with the current line
  and Baa diagnostic underlines. Closing the panel clears those ranges, so a
  hidden search cannot leak highlights across editor tabs.
- **SVG action resources:** Visual actions use the registered vector set under
  `qalam/resources`. `test_qalam_icons` scans source references and verifies
  that each asset exists and is registered; widget tests also prove the key
  action icons load through Qt's resource system.

#### 2. Console (`source/console`)
`QalamConsole` provides an interactive terminal.
- It owns an integrated session bar, state indicator, prompt row, history, ANSI
  rendering, bounded output, and shell lifecycle controls. Windows processes
  use `CREATE_NO_WINDOW`, keeping compiler and program streams inside the panel.
- **`ProcessWorker`:** Runs in a background `QThread` with mutex-protected buffers for the compiler or external scripts, keeping the UI responsive.
  Standalone F5 uses one worker for an explicit two-phase pipeline: Baa first
  writes a uniquely keyed executable under Qalam's application cache, then the
  worker launches that executable only after a successful compile. Both phases
  share ordered stdout/stderr capture, cancellation, and terminal stdin routing.

#### 3. Framing & Theme (`source/ui`)
- **`QalamWindow`:** Handles the frameless window implementation with native Windows snap/shadow and RTL layout.
- **`QalamTheme`:** Singleton managing CSS-based themes for consistent styling across all components.
- **`QalamTitleBar`:** Custom title bar with embedded menu and system buttons.
- **`QalamSidebar`:** Stacked widget for Explorer and Search views.
- **`QalamPanelArea`:** Bottom panel hosting Problems, Output, and Terminal tabs.
  Output is a read-only plain-text view capped at 500 blocks; it is the retained
  presentation surface for validated local Baa-LSP operational events.

#### 4. Managers (`source/managers`)
- **`FileManager`:** Handles file open/save, recent files, drag-and-drop, and file size safety checks.
- **`SessionManager`:** Saves and restores open files, active tab, folder path, and window geometry.
- **`BaaLanguageClient`:** Owns LSP framing, JSON-RPC, the
  Baa-LSP process lifecycle, document sessions, capabilities, and feature
  requests. Baa-LSP—not Qalam—owns compiler invocation and UTF-8 byte to UTF-16
  position conversion. The client caches hierarchical document symbols only
  when their response matches the current document version and cancels obsolete
  symbol requests. `QalamSymbolOutlineView` renders that version-matched hierarchy
  for the current editor. Project-symbol search requests `workspace/symbol`
  once, then lets `QalamCommandPalette` filter the returned Baa-owned index locally;
  Qalam does not scan manifests or parse source for semantic symbols. It tracks
  open-document ownership per Takween root, publishes standard dynamic
  `workspaceFolders`, and watches only `مشروع.تكوين`/`تكوين.قفل` so Baa-LSP can
  ask Takween to refresh the authoritative project plan.
  It opts into `baa-lsp-log-v1`, validates the server's local/no-telemetry
  capability and every bounded monotonic event, then emits a typed
  `BaaLogEvent`; it never parses a human log message for behavior.
  Cursor-sensitive completion flushes the newest full-text
  document change before requesting compiler-scoped symbols and exact UTF-16
  edits, rejects obsolete responses, and passes standard snippet tab stops to
  `QalamSnippetManager`. See
  `documents/BAA_LSP_INTEGRATION_AR.md`. Hover and signature requests use the
  same immediate full-text flush, cancellation, and document-version checks.
  Project navigation starts at the nearest Takween root. Prepared rename
  consumes versioned `WorkspaceEdit` results, previews their scope, applies
  open-document edits as one undo block, and writes closed files atomically.
  Quick fixes consume only Baa-owned structured diagnostic edits, request
  `textDocument/codeAction` at the cursor, preview the selected Arabic action,
  and reuse the same version-checked workspace-edit path. Document formatting
  flushes the newest text, requests `textDocument/formatting`, and applies
  Baa's one versioned full-document edit as a single undo block. Qalam does not
  own formatting rules. Folding and expand/shrink selection request
  `textDocument/foldingRange` and `textDocument/selectionRange`; Baa-LSP serves
  both from one validated `structure-json-v1` result for the current document
  version. The editor keeps local folding only while that authoritative result
  is unavailable and never derives semantic selection ranges itself. If the
  server process exits unexpectedly, the client preserves its current document
  snapshots, restarts with capped exponential backoff, and reopens every
  document after initialization. Three consecutive automatic attempts are
  allowed; the budget resets only after 30 seconds of stable service, preventing
  a broken server from becoming an unbounded process loop.

- **`BuildManager`:** Owns project execution. Build, run, test, and clean requests
  search parent directories for `مشروع.تكوين`, ask
  `takween-targets-v1` for capabilities, and invoke canonical Arabic Takween argv
  in that root. Project F5 selects an authoritative runnable target; standalone
  files retain direct Baa invocation.
- **`TakweenProtocol`:** Strictly parses `takween-targets-v1` and each
  `takween-build-events-v1` JSONL record. The process worker tails complete lines;
  the manager checks sequence, operation, terminal state, and exit-code agreement,
  then emits Arabic progress without parsing stdout/stderr.
- **`DiagnosticParser`:** Treats `diagnostics-json-v1` as the primary compiler
  contract and retains human-text patterns only as a compatibility fallback.
  The model preserves codes, categories, primary/end spans, and hints.
- **Tool completion contract:** `BuildManager` emits project operations with the raw
  process exit code and classifies `compiler-cli-v1` codes `0` through `5`.
- **`ToolchainDiscovery`:** Resolves Baa, Takween, and Nazm once by the shared
  order Settings → `QALAM_*_PATH` → system `PATH` → legacy portable fallback.
  It also builds the exact child-process environment used by Takween, Baa, and
  Baa-LSP, including the selected `BAA_NAZM`, without exposing Baa's private
  linker directory globally.
- **Nazm editor/build path:** `.نظم` documents use a local Arabic assembly
  highlighter and never enter Baa-LSP. `BuildManager` invokes Nazm with argv for
  adjacent object generation, while F5 invokes Baa on the direct `.نظم` root so
  Baa owns startup objects and final linking.
  Qalam uses structured diagnostics first and creates a code-based fallback only
  when the structured model is empty. `run` and `test` remain operation-aware
  because a successfully built program may return its own nonzero code.
- **`LayoutManager`:** Manages sidebar and panel visibility states.

#### 5. Workspace Services (`source/workspace`)

- **`WorkspaceIndexer`:** Builds the shared quick-open and textual-search file
  inventory. It rejects generated/version-control directories, symlinks,
  unsupported extensions, and files larger than 5 MiB. Root and nested
  `.gitignore` rules support comments, negation, anchored paths, `*`, `**`, and
  `?`, with later and more local rules taking precedence. Initial and refreshed
  scans run on a single cancellable worker; a root switch invalidates any stale
  result before it can reach the UI.
- The indexer watches searchable files, ignore files, and traversed directories
  for external content and inventory changes. Native watches are deduplicated
  and capped at 4,096 paths. If that bound or an operating-system limit rejects
  a path, a three-second asynchronous fallback rescan remains active. A 160 ms
  debounce coalesces editor/tool write bursts, and an in-flight scan records one
  pending refresh instead of spawning overlapping work.
- `Qalam` consumes `indexUpdated` as a cache-invalidation boundary. Visible
  project search is rescheduled, and an open Quick Open palette repopulates in
  place while preserving its current filter, including during the initial scan.
- **`ProjectSearchService`:** Runs literal or regular-expression matching on a
  dedicated single-worker pool. An atomic generation cancels stale queued or
  running requests, progress is throttled before crossing to the UI thread,
  and only the newest generation may publish results. Unicode word boundaries
  include letters, marks, numbers, and underscore, so Arabic diacritics are not
  split from their identifier. The request overlays indexed disk paths with
  current editor text and revisions.
- Successful searches are cached from the query flags, result limit, normalized
  file paths, disk size/time metadata, and overlay revision/content hash. A
  project switch, committed replacement, or live index update invalidates that
  cache. A visible search is then rescheduled, so even same-size external edits
  cannot reuse a stale metadata cache entry.
- Project replacement is a two-stage operation. The worker creates a bounded
  immutable plan from UTF-8 snapshots, expands regex captures, and preserves a
  byte-order mark. `Qalam` then asks for confirmation, verifies that every path
  remains inside the open project and every snapshot is current, atomically
  commits closed files with best-effort rollback, and applies each open-editor
  update in one undo block. Unsupported encodings or stale files produce an
  explicit Arabic failure; there is no partial silent fallback.

## Development Workflow

### Adding a new keyword

The Baa compiler owns keywords and completion metadata:

1. Add the language word to Baa's central language profile and its parser rules.
2. Add or adjust the Baa-owned completion record and contract test.
3. Baa-LSP and Qalam consume `completion-data-json-v1`; do not add an editor alias.
4. Until semantic highlighting replaces the local lexer profile, mirror the word
   in `qalam/resources/baa-language.json` for coloring only.

### Adding a new source file

1. Create the `.h` and `.cpp` files in the appropriate `source/` subdirectory.
2. Add the files to `source/CMakeLists.txt` target sources.
3. Run `cmake --preset windows-mingw` (or your platform's preset) to reconfigure.

On Windows, registered tests run through a CMake environment wrapper that
removes duplicate `Path`/`PATH` variables and supplies the configured Qt and
compiler runtime directories. Keep new tests inside `add_qalam_test()` so this
load-time protection remains automatic.

### Adding a setting

1. Add the key constant to `Constants.h` in the appropriate `Settings` namespace.
2. Add UI widget creation in `QalamSettings::createAppearancePage()` (or a new page method).
3. Wire up the load/save logic in `QalamSettings::loadSettings()` and `applySettings()`.
4. Connect the change signal in `Qalam::openSettings()` to propagate changes.

## Build System

**CMake is the only supported build system.** The `.pro` file is stale and cannot build the current codebase.

- Root `CMakeLists.txt`: Manages project configuration and subdirectories.
- `source/CMakeLists.txt`: Defines the `qalam_core` static library.
- `qalam/CMakeLists.txt`: Defines the final `Qalam` executable.
- `CMakePresets.json`: Contains build presets. Currently Windows-only; Linux/macOS presets need to be added (see Phase 7 of ROADMAP.md).

### Prerequisites

- Qt 6.x with Widgets and SVG (the Windows artifact uses 6.10.2; Linux CI also
  checks Ubuntu's base and SVG packages)
- MinGW 13.1+ (Windows) or GCC 13+ (Linux) or Clang 16+ (macOS)
- CMake 3.21+

GitHub Actions keeps the Windows portable artifact on Qt 6.10.2 with its matching
MinGW kit. The same relocatable kit is copied into the artifact under
`baa/gcc/`, where Baa's compiler driver discovers it before considering
`PATH`. Its versioned Baa toolchain manifest admits the measured direct-Unicode
path mode, avoiding dependence on optional Windows 8.3 aliases for Arabic
package locations. The Windows package gate removes external toolchain paths
and requires an Arabic-path compile, link, and successful execution, so object
generation alone cannot certify portability. The Linux compatibility job intentionally
builds against Ubuntu's Qt 6 development packages and runs the same test suite
offscreen, exercising the stated Qt 6.x source-compatibility floor without
depending on Qt download mirror metadata.

### Build Commands

```sh
# Configure (Windows)
cmake --preset windows-mingw

# Build (Debug)
cmake --build build

# Build (Release)
cmake --preset windows-release
cmake --build build --preset release
```

Output: `build/qalam/Qalam.exe`

**Note:** Reconfigure CMake when adding new `.cpp`/`.h` files to `source/CMakeLists.txt` or editing `.qrc`.

---

*For planned improvements, see [ROADMAP.md](ROADMAP.md)*
