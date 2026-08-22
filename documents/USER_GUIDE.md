# Qalam IDE User Guide

Welcome to **Qalam IDE**, the professional environment for Arabic-syntax programming.

## Getting Started

### Creating your first Baa file
1. Click **ملف جديد** (New File) on the Welcome Screen or in the menu.
2. Write your code using Baa syntax (see [Language Spec](LANGUAGE.md)).
3. Save the file with the canonical `.باء` extension using `Ctrl+S`.
   Existing `.baa` projects remain supported during the compatibility period.

### Running your code
1. Open your `.باء` file (or a compatible existing `.baa` file).
2. Ensure Baa and Nazm are installed and visible in `PATH`, or select them on
   the **الأدوات** page in Settings.
3. Use **تشغيل** (Run) from the build menu to compile and execute your program.
   For a standalone `.باء` file, Qalam builds into its application cache, then
   launches the new executable with the source folder as its working directory.
   Program stdout and stderr appear after **مخرجات البرنامج** in the
   **الطرفية** tab, and the input field remains connected to the running program.

### Writing and running Nazm

Qalam treats `.نظم` as the canonical Arabic assembly language of the ecosystem:

1. Open or save a UTF-8 file with the `.نظم` extension.
2. Use `Ctrl+Shift+B` to invoke Nazm directly and create an adjacent `.obj` on
   Windows or `.o` on Linux. The Terminal names the artifact before assembly.
3. Use `F5` to ask Baa to assemble, link, and run the file with its owned linker
   pipeline. Qalam never translates the Arabic source to GAS and never silently
   falls back to another assembler.

Nazm files receive local highlighting for Arabic directives, instructions,
registers, labels, numbers, strings, and comments. Baa-LSP remains Baa-only;
Qalam does not send `.نظم` documents to it.

## Interface Overview

### The Editor
The editor supports:
- **Syntax Highlighting:** Automatic coloring for Baa keywords, types, and directives.
- **RTL Support:** Full right-to-left support for both the interface and the code.
- **Arabic completion:** Baa-owned keywords, directives, snippets, compiler
  builtins, parameters, visible locals, and explicitly included declarations
  appear through Baa-LSP while typing Arabic letters. Inner declarations
  correctly shadow outer ones, while future and sibling-block declarations do
  not appear.
- **Live Baa diagnostics:** Saved and unsaved `.باء`/`.رأسباء` contents are
  checked after a short pause; `.baa`/`.baahd` remain compatible. Only results
  matching the current document revision are shown.
- **Arabic parameter hints:** Baa-owned parameter names appear beside call
  arguments as muted overlays. They are not inserted into the file, are excluded
  when the argument already explains itself, and disappear if their document
  version becomes stale.
- **Language-server status:** validated Baa-LSP events appear as Arabic plain
  text in the **المخرجات** (Output) tab. Qalam keeps only the newest 500 text
  blocks; log content is never interpreted as HTML and no telemetry is sent.
- **Compiler-backed navigation:** `F12` opens the exact declaration selected by
  Baa's semantic analysis, including declarations in headers. `Shift+F12`
  lists scope-correct references across the Takween project in the search
  sidebar. Qalam does not guess from equal-looking identifier text.
- **Symbol outline and project symbols:** the explorer's **المخطط** section
  shows the current document hierarchy and filters it as you type. Press
  `Ctrl+T` to search Baa-owned functions, types, globals, fields, and enum
  members across the Takween source closure, then open the exact location.
- **Safe rename:** press `F2` on a Baa symbol, enter an Arabic name, and review
  the number of affected files and locations. Qalam applies only
  compiler-resolved occurrences and refuses stale or colliding edits.
- **Official formatting:** press `Shift+Alt+F` or choose **تنسيق مستند باء**
  from the editor context menu. Baa formats the current unsaved buffer using
  its canonical four-space style, and Qalam applies the versioned result as one
  undoable edit.
- **Semantic hover:** pause the pointer over an Arabic identifier to see the
  declaration and Baa-owned description. Diagnostic tooltips take priority
  when the pointer is on an error.
- **Call signatures:** type `(`, `،`, or `,` inside a call to see the function
  declaration and active parameter. Use `Ctrl+Shift+Space` to request it again.
- **Bracket Matching:** Automatic pairing of `()`, `{}`, `[]`, `''`, `""`, and ` `` `.
- **Semantic structure:** collapse nested Baa regions from the folding gutter.
  Use `Shift+Alt+Right` to expand the current selection through Baa-owned
  expression, statement, block, and construct ranges; use `Shift+Alt+Left` to
  return through the previous selections.
- **Snippets:** Choose an Arabic template such as `إذا` or `الرئيسية`; use Tab
  or Enter to move through its editable positions.
- **Auto-save:** Automatic backup every 30 seconds to `.~` files.
- **Zoom:** Use `Ctrl+Scroll` to change font size.

### The Console (`F6`)
The embedded console allows you to interact with the system and view compiler output.
- Use `Ctrl+L` to clear the console.
- Command history is available using the Up/Down keys.
- The console runs your default shell (cmd.exe on Windows, bash on Linux).

### File Explorer
The right sidebar shows your project files. You can double-click a file to open
it or drag and drop files from your system. Right-click a file, folder, or empty
area in the tree to create a Baa file, create a folder, rename the selected
entry, or delete it after confirmation. Qalam rejects unsafe or non-portable
names and never applies these operations outside the opened project folder.
Renaming a file or folder keeps affected open tabs and language analysis on the
new path. Right-click an open tab or an item under **المحررات المفتوحة** to
close it, close the other tabs, or close all tabs.

The **المخطط** section below the files shows the hierarchical symbols in the
current Baa document. Its results follow the current unsaved document version.

### Project Symbols (`Ctrl+T`)
Press `Ctrl+T` after the Baa language server is ready. Qalam loads the
compiler-owned workspace index once and filters it locally while you type
Arabic. The list includes the symbol detail, container, relative file, and line.
Selecting a result opens its exact location. If the language server is not
ready, Qalam reports that state instead of leaving an unfinished search open.

### Sidebar Search (`Ctrl+Shift+F`)
Open the project-wide search sidebar. See [Search and Replace](#search-and-replace)
for matching, progress, ignore, and safe replacement behavior.

## Customization

### Themes
You can change the editor's **syntax highlighting theme** in Settings:
- **GitHub Dark** (default)
- **GitHub Light**
- **Monokai**
- **Solarized**

Access via: **File** → **Settings** → **Editor** → **Theme**.

**Note:** The overall UI theme is currently dark-only. Multiple UI themes are planned (see [ROADMAP.md](ROADMAP.md)).

### Fonts
Adjust the editor font size and family in Settings:
- Font Size: 12–36pt
- Font Family: Select from bundled fonts or system fonts (when enabled)

### Build tools

Open **File → Settings → الأدوات** to inspect or override the executable for
Baa, Takween, and Nazm. Leave a field empty for automatic discovery. Qalam uses
the same deterministic order everywhere:

1. a saved path in Settings;
2. `QALAM_BAA_PATH`, `QALAM_TAKWEEN_PATH`, or `QALAM_NAZM_PATH`;
3. the corresponding command in system `PATH`;
4. the old portable beside-Qalam layout as a compatibility fallback.

The page reports the selected source and exact executable path. An invalid
explicit setting remains visible as an error instead of silently selecting a
different installation.

Build actions also follow the current file. Qalam keeps editing and Baa-LSP
available when external tools are missing, disables only the actions that
cannot run, and places the exact Arabic reason in the action tooltip and status
bar. A standalone `.نظم` build needs Nazm; standalone run needs Baa and Nazm;
project build, run, and test need Takween, Baa, and Nazm; project clean needs
Takween only. Saving a new file or changing a tool path refreshes these states
immediately.

---

## Keyboard Shortcuts

| Category | Shortcut | Action | Status |
|----------|----------|--------|--------|
| **File** | `Ctrl+S` | Save | ✓ Working |
| | `Ctrl+N` | New File | ✓ Working |
| | `Ctrl+O` | Open File | ✓ Working |
| | `Ctrl+Shift+S` | Save As | ✓ Working |
| | `Ctrl+W` | Close Tab | ✓ Working |
| **Edit** | `Ctrl+F` | Find / Search | ✓ Working |
| | `Ctrl+G` | Go to Line | ✓ Working |
| | `Ctrl+/` | Toggle Comment | ✓ Working |
| | `Ctrl+D` | Duplicate Line | ✓ Working |
| | `Ctrl+Space` | Trigger Autocomplete | ✓ Working |
| | `Ctrl+.` | Preview a safe Baa quick fix | ✓ Working |
| | `Shift+Alt+F` | Format the current Baa document | ✓ Working |
| **Navigation** | `Alt+Up/Down` | Move line up/down | ✓ Working |
| | `F12` | Go to a Baa declaration | ✓ Working |
| | `Shift+F12` | List compiler-resolved Baa references | ✓ Working |
| | `F2` | Safely rename a Baa symbol across the project | ✓ Working |
| | `Ctrl+T` | Search compiler-owned project symbols | ✓ Working |
| | `Ctrl+Tab` | Next Tab | ✓ Working |
| | `Ctrl+Shift+Tab` | Previous Tab | ✓ Working |
| **View** | `F6` | Toggle Console | ✓ Working |
| | `Ctrl++/-` | Zoom in/out | ✓ Working |
| | `Ctrl+0` | Reset Zoom | ✓ Working |
| | `Ctrl+Shift+F` | Project Search | ✓ Working |
| **Build** | `F5` | Run / Build | ✓ Working |

---

## Safe Quick Fixes

Place the cursor on a Baa diagnostic and press `Ctrl+.` or choose
**إصلاح سريع من باء** from the editor context menu. Qalam asks Baa-LSP for
compiler-owned structured edits, shows an Arabic preview, and applies the
selected fix only when it still matches the current document version.

Qalam does not infer fixes from human-readable error messages. A stale,
destructive, duplicate, or out-of-document edit is refused.

## Format Document

Press `Shift+Alt+F`, choose **الشفرة: تنسيق مستند باء** from the command
palette, or choose **تنسيق مستند باء** from the editor context menu.

Qalam first sends the newest unsaved text to Baa-LSP. Baa owns the canonical
rules—four-space indentation, LF line endings, safe spacing, and preservation
of literal and comment contents. Baa-LSP returns one full-document edit only
if the text changes. Qalam rejects an obsolete version and applies a current
result as one undoable editor action. Client preferences do not create a second
Baa style.

---

## Search and Replace

### Editor Search (`Ctrl+F`)
Find text within the current file:
- Find next with Enter or the down arrow; find previous with Shift+Enter or the
  up arrow. Navigation wraps at the beginning and end.
- Toggle `Aa` for case-sensitive matching, `ab` for an entire Unicode word, or
  `.*` for a regular expression. Arabic combining marks remain part of a word.
- The result counter and editor highlights show every match while distinguishing
  the current result. Search highlights compose with Baa diagnostic underlines.
- Use **استبدال** for the current result or **استبدال الكل** for the complete
  document. Replace-all is one undoable edit.
- In regular-expression mode, replacement text may use `$1` or `\1` for a
  captured group. Invalid and zero-length patterns are rejected safely.
- Escape closes the panel and removes only search highlights.

### Project Search (`Ctrl+Shift+F`)
Search across the eligible UTF-8 files in the opened project folder:

- Typing starts a background search, including one-character Arabic queries;
  changing or clearing the query cancels obsolete work. Arabic progress and
  result-limit or unreadable-file notices remain visible in the sidebar.
- If search or Quick Open is opened during the initial asynchronous scan, it
  shows an Arabic indexing state and fills itself when the current index arrives.
- Toggle the icon buttons for case-sensitive text, an entire Unicode word, or
  a regular expression. Arabic combining marks count as part of their word.
- Qalam recursively searches Baa sources, headers, project text, Markdown,
  JSON, CMake, and native source files up to 5 MiB. Generated directories and
  common root or nested `.gitignore` patterns are excluded.
- Current unsaved editor text overrides the disk copy, so results match what is
  visible in open tabs. Results are grouped by file and open at the exact line
  and column.
- Expand the replacement row, enter replacement text, and choose the
  replace-all icon. Regular expressions accept `$1` or `\1` capture references.
  Qalam first prepares an immutable plan, shows the affected location and file
  counts, and changes nothing until you confirm.
- Before writing, Qalam verifies every open document revision and every closed
  file byte snapshot. A stale or invalid UTF-8 file aborts the whole plan.
  Closed files use atomic saves with best-effort rollback; each open editor
  receives one undoable change. UTF-8 byte-order marks are preserved.
- Repeated unchanged searches use a metadata-and-editor-revision cache. Qalam
  watches the complete searchable inventory and refreshes it after external
  content edits, `.gitignore` changes, and file or directory creation, deletion,
  or rename. When a platform cannot allocate every native watch, a bounded
  background rescan keeps the inventory current without blocking the editor.

---

## Snippets

Type the trigger and press **Tab** to expand:

| Trigger | Expands To |
|---------|------------|
| `الرئيسية` | `صحيح الرئيسية() { ... }` |
| `إذا` | `إذا (شرط) { ... }` |
| `إذا_وإلا` | `إذا (شرط) { ... } وإلا { ... }` |
| `لكل` | `لكل (تهيئة؛ شرط؛ زيادة) { ... }` |
| `طالما` | `طالما (شرط) { ... }` |
| `دالة` | Baa function definition template |

After expanding, press **Tab** to jump between placeholders (e.g., function name, parameters).

---

## Troubleshooting

### "Compiler not found" error
- Install the standalone Baa, Nazm, and (for projects) Takween packages.
- Open **Settings → الأدوات**, clear stale overrides, and choose
  **إعادة فحص الأدوات**.
- Open a new terminal after installation so it receives the updated `PATH`.

### Auto-save files
Backup files use the `.~` suffix (e.g., `برنامج.باء.~`). These are automatically cleaned up when you save or close the file.

### File won't open
- Check that the file is UTF-8 encoded.
- Files larger than 50MB are blocked for safety.
- Files larger than 10MB show a warning.

---

## Development Status

Qalam IDE is actively developed. See [ROADMAP.md](ROADMAP.md) for planned features including:
- Multi-cursor editing
- Split editor
- Minimap
- Git integration
- Debugger

---

*[→ Language Specification](LANGUAGE.md) | [→ Compiler Internals](INTERNALS.md) | [→ Development Roadmap](ROADMAP.md)*
