# AGENTS.md — Qalam IDE

Guidelines for AI agents working on this codebase.

## Project Overview

Qalam IDE is a Qt 6 / C++23 IDE for the Baa Arabic-syntax programming language (لغة باء).
RTL-first design, syntax highlighting, auto-completion, embedded console, frameless window.
The repository includes focused Qt Test coverage and Windows/Linux GitHub Actions
builds. Tests are opt-in through `QALAM_BUILD_TESTS`.

## Build Commands

**Prerequisites:** Qt 6.10.2, MinGW 13.1+, CMake 3.21+

```sh
# Configure (one-time or after adding new source files / .qrc changes)
cmake --preset windows-mingw

# Build (Debug)
cmake --build build

# Build (Release)
cmake --preset windows-release
cmake --build build --preset release
```

Output: `build/qalam/Qalam.exe`. The Vulkan DLL warning during linking is harmless.
**Reconfigure CMake** when adding new `.cpp`/`.h` files to `source/CMakeLists.txt` or editing `.qrc`.

Legacy qmake (not maintained): `cd qalam && qmake Qalam.pro && mingw32-make -j`

Tests:

```sh
cmake -S . -B build-tests -DQALAM_BUILD_TESTS=ON -DCMAKE_PREFIX_PATH=<qt-root>
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

No lint or format checks are configured yet.

## Project Structure

```
qalam/                # App entry: main.cpp, Qalam.cpp/.h, Constants.h, resources.qrc
source/               # Static library (qalam_core)
├── texteditor/       # QalamEditor, QalamBracketHandler, QalamAutoSave, QalamSnippetManager
│   ├── highlighter/  # QalamLexer, QalamSyntaxHighlighter, QalamToken, QalamSyntaxDefinition, ThemeManager
│   └── autocomplete/ # AutoComplete (strategies), AutoCompleteUI
├── console/          # QalamConsole, ProcessWorker
├── components/       # QalamFlatButton, QalamSearchPanel
├── menubar/          # QalamMenuBar
├── settings/         # QalamSettings
├── sidebar/          # QalamExplorerView, QalamSearchView
├── ui/               # QalamWindow, QalamTheme, QalamTitleBar, QalamActivityBar, QalamSidebar,
│                     # QalamBreadcrumb, QalamPanelArea, QalamStatusBar
├── managers/         # FileManager, BuildManager, SessionManager, LayoutManager
└── pages/            # QalamWelcomePage
documents/            # ROADMAP.md, INTERNALS.md, USER_GUIDE.md, LANGUAGE.md
```

## Code Style

### Includes

`#pragma once` in all headers. Order: own header, project-local headers, blank line, Qt headers, STL headers.

```cpp
#include "QalamConsole.h"
#include "Constants.h"
#include "ui/QalamTheme.h"

#include <QVBoxLayout>
#include <QScrollBar>
#include <memory>
```

### Naming Conventions

| Element            | Convention               | Examples                                      |
|--------------------|--------------------------|-----------------------------------------------|
| Qalam-owned UI/classes | `Qalam` prefix + PascalCase | `QalamEditor`, `QalamConsole`, `QalamLexer`, `QalamStatusBar` |
| App/theme classes  | `Qalam` prefix           | `QalamWindow`, `QalamTheme`                   |
| Manager classes    | PascalCase (no prefix)   | `FileManager`, `SessionManager`               |
| Data/interface     | PascalCase (no prefix)   | `CompletionItem`, `ICompletionStrategy`, `SyntaxTheme` |
| Private members    | `m_` prefix              | `m_editor`, `m_fileManager`, `m_glowIntensity` |
| Public members     | camelCase, no prefix     | `filePath`, `tabWidget`                       |
| Signals            | camelCase, past tense    | `fontSizeChanged`, `buildFinished`, `closed`  |
| Slots              | camelCase, `on` or verb  | `onInputReturn`, `performFind`, `toggleConsole` |
| Constants          | Nested namespace + PascalCase | `Constants::Colors::Accent`, `Constants::Timing::FlushInterval` |
| Enums              | `enum class` preferred   | `TokenType::Keyword`, `SnippetId::Function`   |

### Member Initialization

Always use `{}` value-initialization on member declarations:

```cpp
QTabWidget *m_tabWidget{};
int m_historyIndex{};
bool m_autoscroll{true};
QString m_output{};
```

### Class Declaration Order

```cpp
class QalamExample : public QWidget {
    Q_OBJECT
public:
    explicit QalamExample(QWidget *parent = nullptr);
    void publicMethod();
public slots:
    void slotMethod();
protected:
    void paintEvent(QPaintEvent *event) override;
private slots:
    void onSomething();
signals:
    void somethingChanged();
private:
    QWidget *m_widget{};
};
```

Use `explicit` on all single-argument constructors. Use `override` on all virtual overrides.

### Formatting

- **4 spaces** indentation (no tabs)
- **Same-line braces** (K&R style): `if (cond) {`
- **Pointer/reference style**: `Type *name`, `const QString &param`
- **Logical operators**: prefer `and`, `or`, `not` over `&&`, `||`, `!`
- **No strict line length** but aim for ~100 characters
- All header files use `.h`, all source files use `.cpp` (no `.hpp`, no `.ui`)

### Signal/Slot Connections

Always use pointer-to-member syntax. Never use string-based `SIGNAL()`/`SLOT()`.

```cpp
connect(button, &QPushButton::clicked, this, &MyClass::handleClick);

// Lambda for inline logic
connect(timer, &QTimer::timeout, this, [this]() {
    if (m_editor) m_editor->update();
});

// QOverload for overloaded signals
connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MyClass::onIndexChanged);
```

### Memory Management

- **Qt parent-child**: raw `new` with parent for all QObject-derived widgets
- **`std::unique_ptr`**: non-Qt owned objects (lexer states, strategies)
- **`std::shared_ptr`**: shared config objects (`SyntaxTheme`)
- **`QPointer`**: weak refs to QObjects across threads (`BuildManager::m_worker`)
- **`deleteLater()`**: for cross-thread QObject cleanup

### Error Handling

No exceptions. Guard with early return + `QMessageBox` for user-facing errors:

```cpp
if (not file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, "خطأ", "لا يمكن فتح الملف");
    return;
}
```

Use `qWarning()` for non-fatal internal issues. Use `Q_UNUSED()` for intentionally unused parameters.

### RTL / Arabic

- App layout is `Qt::RightToLeft` globally (`main.cpp`)
- All user-facing UI strings are in Arabic
- Document text direction: `option.setTextDirection(Qt::RightToLeft)`
- Code comments and variable names in source are English; UI labels are Arabic
- Baa uses `//` for comments, `.` as statement terminator, `؛` as Arabic semicolon

## Architecture Patterns

- **Singleton**: `LanguageDefinition::instance()`, `QalamTheme::instance()`
- **Strategy**: `ICompletionStrategy` → `KeywordStrategy`, `BuiltinStrategy`, `SnippetStrategy`, `DynamicWordStrategy`
- **State Machine**: `LexerState` → `NormalState`, `StringState` (with `nextState()` transitions)
- **Observer**: Qt signals/slots throughout; `QalamMenuBar` emits, `Qalam` connects

## Common Tasks

**Adding a Baa keyword:** Edit `qalam/resources/baa-language.json` (keywords array). Fallback list is in `QalamSyntaxDefinition.cpp:loadDefaults()`.

**Adding a snippet:** Add entry in `SnippetStrategy::getSuggestions()` (`AutoComplete.cpp`) with a `SnippetId`. Add navigation in `QalamSnippetManager::setupNavigation()`.

**Adding a setting:** Add key to `Constants.h`, add UI in `QalamSettings::createAppearancePage()`, connect in `Qalam::openSettings()`.

**Adding a source file:** Add to `source/CMakeLists.txt` target sources, then reconfigure CMake.

## File Extensions

- `.baa` — Baa source files
- `.baahd` — Baa header files
- `.~` suffix — auto-save backup files (temporary)

## Dependencies

- Qt6::Widgets, Qt6::Gui, Qt6::Core
- dwmapi, user32 (Windows frameless window APIs)
