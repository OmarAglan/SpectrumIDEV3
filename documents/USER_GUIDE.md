# Qalam IDE User Guide

Welcome to **Qalam IDE**, the professional environment for Arabic-syntax programming.

## Getting Started

### Creating your first Baa file
1. Click **ملف جديد** (New File) on the Welcome Screen or in the menu.
2. Write your code using Baa syntax (see [Language Spec](LANGUAGE.md)).
3. Save the file with a `.baa` extension using `Ctrl+S`.

### Running your code
1. Open your `.baa` file.
2. Ensure you have the Baa compiler installed (default path: `baa/baa.exe` relative to the IDE).
3. Use **تشغيل** (Run) from the build menu to compile and execute your program.

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
- **Live Baa diagnostics:** Saved and unsaved `.baa`/`.baahd` contents are checked
  after a short pause. Only results matching the current document revision are shown.
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
- **Code Folding:** Collapse and expand supported Baa blocks.
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
it or drag and drop files from your system. The **المخطط** section below the
files shows the hierarchical symbols in the current Baa document. Its results
follow the current unsaved document version.

### Project Symbols (`Ctrl+T`)
Press `Ctrl+T` after the Baa language server is ready. Qalam loads the
compiler-owned workspace index once and filters it locally while you type
Arabic. The list includes the symbol detail, container, relative file, and line.
Selecting a result opens its exact location. If the language server is not
ready, Qalam reports that state instead of leaving an unfinished search open.

### Sidebar Search (`Ctrl+Shift+F`)
Search across all files in the opened folder (project-wide search).

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
- Find next: Enter or click down arrow
- Find previous: Shift+Enter or click up arrow
- Case-sensitive toggle: Click "Cc" button
- Wrap-around: Automatically wraps to beginning/end

**Note:** Replace functionality is planned for a future release (see [ROADMAP.md](ROADMAP.md)).

### Project Search (`Ctrl+Shift+F`)
Search across all files in the opened project folder.

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
- Ensure the Baa compiler is installed.
- The IDE looks in this order:
  1. Path specified in Settings
  2. `baa/baa.exe` relative to the IDE
  3. System PATH

### Auto-save files
Backup files use the `.~` suffix (e.g., `program.baa.~`). These are automatically cleaned up when you save or close the file.

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
- Semantic tokens, folding ranges, and selection ranges through Baa-LSP
- Git integration
- Debugger

---

*[→ Language Specification](LANGUAGE.md) | [→ Compiler Internals](INTERNALS.md) | [→ Development Roadmap](ROADMAP.md)*
