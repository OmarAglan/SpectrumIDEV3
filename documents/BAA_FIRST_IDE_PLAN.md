# Qalam Baa-First IDE Plan

**Decision:** Qalam is the dedicated IDE for Baa.
**Status:** Active
**Started:** 2026-07-22
**Product language:** Arabic-first UI and Baa-first workflows.
**Platform gate:** Windows and Linux.

## Product Goal

A Baa programmer should be able to open a file or Takween project, understand
the program, make a safe change, build it, run it, and diagnose a failure
without leaving Qalam or interpreting compiler text manually.

Qalam is intentionally Baa-first. Its internal service boundaries may remain
extensible, but general multi-language support is not part of this milestone.

## Ownership

```text
Qalam   = interactive editing and IDE presentation
Baa-LSP = LSP transport, document state, position conversion, and semantic requests
Baa     = syntax, semantics, symbols, types, and diagnostic truth
Takween = project graph and build/run/test/clean truth
Nazm    = assembly and object-production truth
```

Qalam must not implement a second Baa parser or semantic analyzer. Local editor
logic may provide immediate lexical coloring, bracket handling, and snippets,
but authoritative language answers come from versioned Baa contracts.

**Transport decision:** the permanent editor boundary is LSP over `stdio`
through the standalone `Baa-LSP` project. The temporary direct compiler bridge
proved the unsaved-source contract and has now been removed after the LSP client
reached diagnostic parity. See
[BAA_LSP_INTEGRATION_AR.md](BAA_LSP_INTEGRATION_AR.md).

## Current Baseline

Qalam already has:

- an RTL-first Qt editor with lexical highlighting, Baa-LSP completion,
  server-owned snippets, folding, bracket insertion, and autosave;
- structured `diagnostics-json-v1` parsing, inline diagnostic decoration, a
  Problems panel, and diagnostic navigation;
- structured Takween targets and build-event consumption;
- basic workspace indexing, definition lookup, references search, command
  palette, and project file discovery;
- focused Qt Test coverage and Windows/Linux CI.

The first LSP slice now provides:

- diagnostics for unsaved Baa editor contents through versioned LSP documents;
- a dedicated `BaaLanguageClient`, independent from `BuildManager`;
- stale-result rejection by document version;
- graceful `initialize`, `shutdown`, and `exit` lifecycle handling;
- Arabic and space-containing path coverage in both client and server tests;
- compiler-owned hierarchical document symbols with UTF-16 ranges, per-version
  caching, cancellation, and authoritative same-document F12 locations;
- Arabic-triggered completion from `completion-data-json-v1`, merged with
  version-matched document-global symbols and applied through exact text edits;
- compiler-backed hover and signature help from `semantic-query-json-v1`, with
  scope-correct declarations, included prototypes, active parameters, and
  incomplete-call behavior;

The next language-intelligence gaps are:

- scope-aware local/include completion, workspace definitions, references,
  rename, and a visible outline are not available;
- `--dump-symbols=json` is implemented and consumed; a structured token stream
  remains a future compiler contract.

## Target Architecture

### 1. Immediate editor layer

Runs synchronously and never waits for a compiler process:

- text input, caret, selection, undo, and bidi behavior;
- lexical highlighting and bracket matching;
- bracket matching, indentation, and rendering of the last accepted language result;
- indentation and formatting application;
- visible cached results from the last accepted compiler analysis.

### 2. Baa intelligence layer

A dedicated `BaaLanguageClient` and per-document session own:

- `Content-Length` framing, JSON-RPC requests, and server lifecycle;
- one monotonically increasing LSP document version per document;
- cancellation or rejection of obsolete requests;
- `didOpen`, full-text `didChange`, `didSave`, and `didClose` synchronization;
- capability-driven completion, hover, symbols, navigation, and diagnostics;
- rejection of results whose document revision is no longer current;
- conversion of LSP results into Qalam's UI models.

`Baa-LSP` owns compiler invocation, unsaved UTF-8 delivery, structured contract
validation, compiler exit-code classification, and UTF-8 byte to LSP UTF-16
position conversion. Qalam never parses Baa compiler output after migration.

### 3. Takween project layer

`BuildManager` remains responsible for:

- discovering the owning Takween project and target index;
- build, run, test, clean, cancellation, output, and artifacts;
- consuming structured Takween events;
- providing active target/include/dependency context to Baa analysis.

Language analysis and project execution must remain independently cancellable.

## Step-by-Step Delivery Plan

### Step 1 — Contract and architecture baseline

- Preserve this ownership decision in Qalam documentation.
- Inventory implemented versus documented Baa tooling modes.
- Define request identity, document revision, logical path, UTF-8 source,
  output schema, and cancellation behavior.
- Define performance and correctness acceptance criteria.

Acceptance:

- No planned Qalam feature requires parsing human-readable compiler text when
  a structured contract is available.
- Every unimplemented compiler surface is marked as a dependency, not treated
  as current functionality.

### Step 2 — Unsaved-buffer diagnostics vertical slice

- Add a Baa check-only input contract that reads UTF-8 source from stdin while
  retaining the real logical source path for includes and diagnostics.
- Restrict the input mode to safe, non-codegen analysis.
- Add Windows/Linux compiler contract tests for clean, syntax-error,
  semantic-error, Arabic-path, include, empty, and invalid UTF-8 inputs.
- Prove the contract through a temporary direct compiler bridge, then remove it
  after LSP diagnostic parity. **Complete.**
- Debounce edits, supersede obsolete requests, and write no source snapshots to
  the project tree.
- Apply only results matching the current document revision.

Acceptance:

- An unsaved error appears after the debounce interval and disappears after it
  is fixed, without saving the file.
- Older process results can never replace a newer document result.
- Relative includes resolve as if the real source file had been analyzed.
- Arabic and space-containing paths pass on Windows and Linux.

### Step 3 — LSP migration and diagnostics experience

- Add the generic Qalam LSP framer, JSON-RPC connection, server process, and
  document session. **Complete for the diagnostics slice.**
- Connect the standalone Baa-LSP server and remove direct live compiler
  invocation after parity. **Complete.**
- Preserve complete start/end ranges in editor decorations.
- Add error and warning gutter markers and overview navigation.
- Keep Problems, status counts, tooltips, and editor ranges synchronized.
- Navigate to the exact range and expose compiler hints and stable codes.
- Separate project-build diagnostics from live document diagnostics.

Acceptance:

- One diagnostic has one stable identity across every Qalam presentation.
- Closing, renaming, or editing a document cannot leave stale markers behind.

### Step 4 — Compiler-backed symbols and navigation

- Implement and test Baa's versioned symbol output. **Complete for
  `symbols-json-v1`.**
- Add document and workspace symbol indexes sourced from the compiler.
  **Document cache and Takween-aware navigation index complete; a user-facing
  workspace-symbol search remains pending.**
- Implement exact go-to-definition and references. **Complete across the
  Takween project source closure, including headers, shadowed locals,
  cancellation, stale-version rejection, F12 navigation, and a structured
  references list.**
- Add scope-aware, collision-checked rename with an edit preview. **Complete
  with compiler identities, conservative collision refusal, Arabic-only names,
  versioned edits, and atomic closed-file saves.**
- Add diagnostic code actions. **Complete for compiler-owned safe insertion
  fixes, with exact UTF-16 conversion, stale-version refusal, an Arabic preview,
  and no message parsing.**
- Keep the workspace text index for explicit text search and quick-open only;
  semantic navigation never falls back to regex or equal-looking text.

Acceptance:

- Definitions and references respect scope, shadowing, includes, and Arabic
  normalized identity.
- Rename never edits comments or unrelated equal-looking names by accident.

### Step 5 — Completion and hover

- [x] Define and consume versioned Baa static completion metadata.
- [x] Merge document-global compiler symbols with snippets without editor-owned aliases.
- [x] Consume cursor-visible parameters, locals, compiler builtins, and
  explicitly included declarations from Baa without an editor-side scope table.
- [x] Trigger and filter naturally while typing Arabic letters.
- [x] Display compiler-owned symbol kind, type, signature, source, and Arabic documentation.
- [x] Add parameter help for calls and included function prototypes.

Acceptance:

- Suggestions are valid at the cursor's syntactic and semantic position.
- Completion remains responsive while analysis is running or unavailable.

### Step 6 — Arabic editing correctness

- Centralize UTF-8 byte offset to Qt UTF-16 position conversion.
- Test RTL caret movement, selection, deletion, punctuation, digits, strings,
  comments, and mixed unavoidable technical text.
- Use NFC for compiler identity; provide search-friendly normalization without
  silently changing source identifiers.
- Add matching-bracket highlighting and Baa-aware formatting/indentation.
  **Canonical document formatting is complete through Baa-owned
  `format-json-v1`, a versioned Baa-LSP full-document edit, `Shift+Alt+F`, and
  one undoable Qalam edit; richer on-type indentation remains additive.**

Acceptance:

- Diagnostic, hover, rename, and completion ranges remain exact for Arabic
  identifiers before and after non-BMP characters.
- Keyboard-only editing is predictable on Windows and Linux.

### Step 7 — Takween-native project experience

- Add persistent active target/profile selection.
- Present dependencies, source roots, targets, tests, artifacts, and cache
  state from structured Takween data.
- Provide build/run/test/clean controls with progress and actionable failures.
- Pass the selected target and project context to Baa analysis.

Acceptance:

- Qalam never reconstructs or guesses the project graph from source files.
- Editor analysis and build results use the same target and dependency context.

### Step 8 — Essential IDE completeness

- Finish replace and project search.
- Add explorer create, rename, delete, and new-folder operations.
- Make shortcuts, compiler/Takween locations, fonts, themes, and analysis delay
  configurable.
- Surface autosave and recovery failures.
- Complete keyboard navigation, accessibility labels, and high-contrast states.

Acceptance:

- A Baa project can be created and maintained without routine filesystem work
  outside Qalam.

### Step 9 — Admission and release

- Run unit, integration, headless GUI, Unicode-path, large-file, rapid-edit,
  cancellation, determinism, and packaging gates.
- Verify the same Baa/Takween contracts on Windows and Linux.
- Update user, internal, tooling, compatibility, and release documentation.

Acceptance:

- The complete Baa-first workflow has reproducible cross-platform evidence.
- No known correctness issue is hidden by a fallback parser or text scraping.

## Initial Performance Budgets

- Local typing/highlighting work: no visible input stall.
- Analysis debounce: 250 ms by default and configurable later.
- Stale-result decision: constant time using document identity and revision.
- Small-file diagnostic refresh: target under 750 ms after debounce on the
  supported development machines.
- Rapid editing: at most one active compiler analysis per document and no
  unbounded request queue.

Budgets will be measured before being promoted to release gates.

## Deferred Beyond This Milestone

- General multi-language support.
- A public extension marketplace.
- A full debugger until Baa's debug/source mapping contract is ready.
- The ecosystem work recorded in the umbrella deferred-goal document.
