#include "QalamTheme.h"
#include "../../qalam/Constants.h"
#include <QFontDatabase>

QalamTheme& QalamTheme::instance() {
    static QalamTheme instance;
    return instance;
}

QalamTheme::QalamTheme() {}

void QalamTheme::apply(QApplication* app) {
    app->setStyleSheet(globalStyleSheet());
    app->setFont(uiFont());
}

// ==========================================================================
// Fonts
// ==========================================================================

QFont QalamTheme::uiFont() const {
    QStringList families;
    families << "Tajawal" << "Segoe UI" << "Roboto" << "sans-serif";
    
    QFont font;
    font.setFamilies(families);
    font.setPointSize(Constants::Fonts::UISize);
    return font;
}

QFont QalamTheme::codeFont() const {
    QStringList families;
    families << "Kawkab Mono" << "Consolas" << "JetBrains Mono" << "Courier New" << "monospace";
    
    QFont font;
    font.setFamilies(families);
    font.setPointSize(Constants::DefaultFontSize);
    font.setFixedPitch(true);
    return font;
}

QFont QalamTheme::consoleFont() const {
    QStringList families;
    families << "Consolas" << "JetBrains Mono" << "Courier New" << "monospace";
    
    QFont font;
    font.setFamilies(families);
    font.setPointSize(Constants::Fonts::ConsoleSize);
    font.setFixedPitch(true);
    return font;
}

// ==========================================================================
// Color Accessors
// ==========================================================================

QColor QalamTheme::background() const { return QColor(Constants::Colors::WindowBackground); }
QColor QalamTheme::foreground() const { return QColor(Constants::Colors::TextSecondary); }
QColor QalamTheme::accent() const { return QColor(Constants::Colors::Accent); }
QColor QalamTheme::border() const { return QColor(Constants::Colors::Border); }
QColor QalamTheme::sidebarBackground() const { return QColor(Constants::Colors::SidebarBackground); }
QColor QalamTheme::editorBackground() const { return QColor(Constants::Colors::EditorBackground); }
QColor QalamTheme::statusBarBackground() const { return QColor(Constants::Colors::StatusBarBackground); }
QColor QalamTheme::titleBarBackground() const { return QColor(Constants::Colors::TitleBarBackground); }

// ==========================================================================
// Shared Style Components
// ==========================================================================

QString QalamTheme::scrollbarStyles() {
    using namespace Constants;
    return QString(R"(
        QScrollBar:vertical {
            border: none;
            background: %1;
            width: %2px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: %3;
            min-height: 20px;
            border-radius: 7px;
            margin: 3px;
        }
        QScrollBar::handle:vertical:hover {
            background: %4;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            border: none;
            background: none;
            height: 0px;
        }
        
        QScrollBar:horizontal {
            border: none;
            background: %1;
            height: %2px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background: %3;
            min-width: 20px;
            border-radius: 7px;
            margin: 3px;
        }
        QScrollBar::handle:horizontal:hover {
            background: %4;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            border: none;
            background: none;
            width: 0px;
        }
    )")
    .arg(Colors::ScrollbarBackground)
    .arg(Layout::ScrollbarWidth + 4) // 14px total
    .arg(Colors::ScrollbarThumb)
    .arg(Colors::ScrollbarThumbHover);
}

QString QalamTheme::menuStyles() {
    using namespace Constants;
    return QString(R"(
        QMenu {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 4px 0px;
        }
        QMenu::item {
            padding: 6px 30px 6px 20px;
            background-color: transparent;
        }
        QMenu::item:selected {
            background-color: %3;
            color: %4;
            border-radius: 4px;
            margin: 0px 4px;
        }
        QMenu::separator {
            height: 1px;
            background: %2;
            margin: 4px 10px;
        }
        
        QMenuBar {
            background-color: transparent;
            color: %5;
            border: none;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 4px 10px;
            border-radius: 4px;
        }
        QMenuBar::item:selected, QMenuBar::item:pressed {
            background-color: #505050;
            color: #ffffff;
        }
    )")
    .arg(Colors::MenuBackground)
    .arg(Colors::Border)
    .arg(Colors::ListActiveBackground)
    .arg(Colors::TextPrimary)
    .arg(Colors::TextSecondary)
    .arg(Colors::ButtonHover)
    .arg(Colors::Accent);
}

QString QalamTheme::buttonStyles() {
    using namespace Constants;
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 2px;
            padding: 6px 14px;
        }
        QPushButton:hover {
            background-color: %3;
        }
        QPushButton:pressed {
            background-color: %4;
        }
        QPushButton:disabled {
            background-color: %5;
            color: %6;
        }
    )")
    .arg(Colors::Accent)
    .arg(Colors::TextPrimary)
    .arg(Colors::AccentHover)
    .arg(Colors::AccentAlt)
    .arg(Colors::ButtonHover)
    .arg(Colors::TextDisabled);
}

QString QalamTheme::inputStyles() {
    using namespace Constants;
    return QString(R"(
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 2px;
            padding: 4px 8px;
            color: %3;
            selection-background-color: %4;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid %5;
        }
    )")
    .arg(Colors::InputBackground)
    .arg(Colors::Border)
    .arg(Colors::TextSecondary)
    .arg(Colors::Selection)
    .arg(Colors::BorderFocus);
}

QString QalamTheme::listStyles() {
    using namespace Constants;
    return QString(R"(
        QListWidget, QTreeWidget, QTreeView, QListView {
            background-color: %1;
            border: none;
            outline: none;
            color: %2;
        }
        QListWidget::item, QTreeWidget::item, QTreeView::item, QListView::item {
            padding: 4px 8px;
        }
        QListWidget::item:hover, QTreeWidget::item:hover, QTreeView::item:hover, QListView::item:hover {
            background-color: %3;
        }
        QListWidget::item:selected, QTreeWidget::item:selected, QTreeView::item:selected, QListView::item:selected {
            background-color: %4;
            color: %5;
        }
        QTreeView::branch {
            background: transparent;
        }
        QTreeView::branch:closed:has-children {
            border-image: none;
            image: url(:/icons/resources/right-arrow.svg);
        }
        QTreeView::branch:open:has-children {
            border-image: none;
            image: url(:/icons/resources/down-arrow.svg);
        }
    )")
    .arg(Colors::SidebarBackground)
    .arg(Colors::TextSecondary)
    .arg(Colors::ListHoverBackground)
    .arg(Colors::ListActiveBackground)
    .arg(Colors::TextPrimary);
}

QString QalamTheme::tooltipStyles() {
    using namespace Constants;
    return QString(R"(
        QToolTip {
            background-color: %1;
            border: 1px solid %2;
            color: %3;
            padding: 4px 8px;
        }
    )")
    .arg(Colors::SidebarBackground)
    .arg(Colors::Border)
    .arg(Colors::TextSecondary);
}

// ==========================================================================
// Global Stylesheet
// ==========================================================================

QString QalamTheme::globalStyleSheet() const {
    using namespace Constants;
    
    QString baseStyles = QString(R"(
        QMainWindow, QDialog {
            background-color: %1;
            color: %2;
        }
        
        QWidget {
            background-color: transparent;
            color: %2;
            font-family: 'Segoe UI', 'Tajawal', sans-serif;
        }

        QPlainTextEdit, QTextEdit {
            background-color: %3;
            color: %4;
            border: none;
            selection-background-color: %5;
            selection-color: %6;
        }
        
        QSplitter::handle {
            background-color: %7;
        }
        QSplitter::handle:horizontal {
            width: %8px;
        }
        QSplitter::handle:vertical {
            height: %8px;
        }
    )")
    .arg(Colors::WindowBackground)
    .arg(Colors::TextSecondary)
    .arg(Colors::EditorBackground)
    .arg(Colors::TextPrimary)
    .arg(Colors::Selection)
    .arg(Colors::TextPrimary)
    .arg(Colors::BorderSubtle)
    .arg(Layout::SplitterWidth);
    
    return baseStyles 
         + scrollbarStyles() 
         + menuStyles() 
         + buttonStyles() 
         + inputStyles() 
         + listStyles() 
         + tooltipStyles()
         + tabBarStyleSheet();
}

// ==========================================================================
// Component-Specific Stylesheets
// ==========================================================================

QString QalamTheme::activityBarStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamActivityBar {
            background-color: %1;
            border-left: 1px solid %2;
        }
        
        QalamActivityBar QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 0px;
            padding: 0px;
            margin: 0px;
        }
        
        QalamActivityBar QPushButton:hover {
            color: %3;
        }
        
        QalamActivityBar QPushButton:checked {
            border-right: %4px solid %5;
            background-color: transparent;
        }
        
        QalamActivityBar QPushButton:checked:hover {
            background-color: transparent;
        }
    )")
    .arg(Colors::ActivityBarBackground)
    .arg(Colors::ActivityBarBorder)
    .arg(Colors::ButtonHover)
    .arg(Layout::ActivityIndicatorWidth)
    .arg(Colors::ActivityIndicator)
    .arg(Colors::ButtonPressed);
}

QString QalamTheme::sidebarStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamSidebar {
            background-color: %1;
            border-left: 1px solid %2;
        }
        
        #sidebarHeader {
            background-color: %3;
            border-bottom: 1px solid %2;
        }
        
        #sidebarHeaderTitle {
            color: %4;
            font-size: %5px;
            font-weight: 600;
            letter-spacing: 0.5px;
            font-family: 'Segoe UI', 'Tajawal', sans-serif;
        }
    )")
    .arg(Colors::SidebarBackground)
    .arg(Colors::BorderSubtle)
    .arg(Colors::SidebarHeaderBackground)
    .arg(Colors::TextSecondary)
    .arg(Fonts::SectionHeaderSize);
}

QString QalamTheme::explorerViewStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamExplorerView {
            background-color: %1;
            border: none;
        }
        
        #openEditorsSection, #folderSection {
            background-color: %1;
        }
        
        #sectionHeader {
            background-color: %1;
            border-bottom: none;
            padding: 0px;
        }
        
        #sectionToggle {
            color: %2;
            font-size: %3px;
            font-weight: 600;
            font-family: 'Segoe UI', 'Tajawal', sans-serif;
            text-align: right;
            padding: 4px 8px;
            background-color: transparent;
            border: none;
        }
        
        #sectionToggle:hover {
            background-color: %4;
        }
        
        #openEditorsTree {
            background-color: %1;
            border: none;
            padding: 0px;
        }
        
        #openEditorsTree::item {
            padding: 2px 8px;
            height: 22px;
        }
        
        #openEditorsTree::item:hover {
            background-color: %4;
        }
        
        #openEditorsTree::item:selected {
            background-color: %5;
            color: %6;
        }
        
        #fileTreeView {
            background-color: %1;
            border: none;
        }
        
        #fileTreeView::item {
            padding: 2px 4px;
            height: 22px;
        }
        
        #fileTreeView::item:hover {
            background-color: %4;
        }
        
        #fileTreeView::item:selected {
            background-color: %5;
            color: %6;
        }
        
        #noFolderWidget {
            background-color: %1;
        }
        
        #noFolderLabel {
            color: %7;
            font-size: %8px;
        }
        
        #openFolderBtn {
            background-color: #0e639c;
            color: %6;
            border: none;
            border-radius: 2px;
            padding: 6px 12px;
            font-size: %8px;
        }
        
        #openFolderBtn:hover {
            background-color: #1177bb;
        }
    )")
    .arg(Colors::SidebarBackground)      // %1
    .arg(Colors::TextSecondary)          // %2
    .arg(Fonts::SectionHeaderSize)       // %3
    .arg(Colors::ListHoverBackground)    // %4
    .arg(Colors::ListActiveBackground)   // %5
    .arg(Colors::TextPrimary)            // %6
    .arg(Colors::TextMuted)              // %7
    .arg(Fonts::UISize);                 // %8
}

QString QalamTheme::searchViewStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamSearchView {
            background-color: %1;
            border: none;
        }
        
        #searchContainer {
            background-color: %1;
            padding: 8px;
        }
        
        #searchInput, #replaceInput {
            background-color: #3c3c3c;
            border: 1px solid %2;
            border-radius: 2px;
            padding: 4px 8px;
            color: %3;
            font-size: %4px;
        }
        
        #searchInput:focus, #replaceInput:focus {
            border: 1px solid %5;
        }
        
        #searchOptionsContainer {
            background-color: transparent;
        }
        
        #optionBtn {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 2px;
            padding: 2px 6px;
            color: %6;
            font-size: 11px;
        }
        
        #optionBtn:hover {
            background-color: %7;
        }
        
        #optionBtn:checked {
            background-color: %8;
            border: 1px solid %5;
        }
        
        #resultsTree {
            background-color: %1;
            border: none;
            border-top: 1px solid %2;
        }
        
        #resultsTree::item {
            padding: 2px 8px;
        }
        
        #resultsTree::item:hover {
            background-color: %7;
        }
        
        #resultsTree::item:selected {
            background-color: %8;
            color: %9;
        }
        
        #noResultsLabel {
            color: %6;
            padding: 16px;
        }
        
        #replaceBtn, #replaceAllBtn {
            background-color: transparent;
            border: 1px solid %2;
            border-radius: 2px;
            padding: 4px 8px;
            color: %3;
            font-size: 12px;
        }
        
        #replaceBtn:hover, #replaceAllBtn:hover {
            background-color: %7;
        }
    )")
    .arg(Colors::SidebarBackground)       // %1
    .arg(Colors::Border)                  // %2
    .arg(Colors::TextSecondary)           // %3
    .arg(Fonts::UISize)                   // %4
    .arg(Colors::Accent)                  // %5
    .arg(Colors::TextMuted)               // %6
    .arg(Colors::ListHoverBackground)     // %7
    .arg(Colors::ListActiveBackground)    // %8
    .arg(Colors::TextPrimary);            // %9
}

QString QalamTheme::breadcrumbStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamBreadcrumb {
            background-color: %1;
            border-bottom: 1px solid %2;
        }
    )")
    .arg(Colors::BreadcrumbBackground)
    .arg(Colors::BorderSubtle);
}

QString QalamTheme::panelAreaStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamPanelArea {
            background-color: %1;
            border-top: 1px solid %2;
        }
        
        #panelTabBar {
            background-color: %1;
            border-bottom: 1px solid %2;
        }
        
        #panelTab {
            background-color: transparent;
            color: %3;
            border: none;
            border-bottom: 2px solid transparent;
            padding: 8px 12px;
            font-size: %4px;
            font-family: 'Segoe UI', 'Tajawal', sans-serif;
        }
        
        #panelTab:hover {
            color: %5;
        }
        
        #panelTab:checked {
            color: %5;
            border-bottom: 2px solid %6;
        }
        
        #panelContent {
            background-color: %1;
        }
        
        #problemsTree, #outputArea, #terminalWidget {
            background-color: %1;
            border: none;
            color: %7;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: %8px;
        }
    )")
    .arg(Colors::PanelBackground)         // %1
    .arg(Colors::PanelBorder)             // %2
    .arg(Colors::TextMuted)               // %3
    .arg(Fonts::TabSize)                  // %4
    .arg(Colors::TextPrimary)             // %5
    .arg(Colors::Accent)                  // %6
    .arg(Colors::ConsoleText)             // %7
    .arg(Fonts::ConsoleSize);             // %8
}

QString QalamTheme::statusBarStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamStatusBar {
            background-color: %1;
            border: none;
        }
        
        QalamStatusBar QLabel {
            color: %2;
            font-size: %3px;
            padding: 0 %4px;
        }
        
        QalamStatusBar QPushButton {
            background-color: transparent;
            color: %2;
            border: none;
            padding: 0 %4px;
            font-size: %3px;
        }
        
        QalamStatusBar QPushButton:hover {
            background-color: %5;
        }
    )")
    .arg(Colors::StatusBarBackground)
    .arg(Colors::StatusBarForeground)
    .arg(Fonts::StatusBarSize)
    .arg(Layout::StatusBarItemPadding)
    .arg(Colors::StatusBarHover);
}

QString QalamTheme::titleBarStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamTitleBar {
            background-color: %1;
        }
        
        #titleLabel {
            color: %2;
            font-size: 12px;
            font-family: 'Segoe UI', 'Tajawal', sans-serif;
        }

        #commandCenterButton {
            background-color: #2b2b2b;
            color: %2;
            border: 1px solid #3c3c3c;
            border-radius: 5px;
            padding: 1px 12px;
            font-size: 12px;
            font-family: 'Segoe UI', 'Tajawal', sans-serif;
            text-align: center;
        }

        #commandCenterButton:hover {
            background-color: #333333;
            border-color: %3;
            color: #ffffff;
        }
        
        #captionButton, #closeButton {
            background-color: transparent;
            border: none;
            border-radius: 0px;
        }
        
        #captionButton:hover {
            background-color: %3;
        }
        
        #captionButton:pressed {
            background-color: %4;
        }
        
        #closeButton:hover {
            background-color: %5;
        }
        
        #closeButton:pressed {
            background-color: %6;
        }
    )")
    .arg(Colors::TitleBarBackground)
    .arg(Colors::TextSecondary)
    .arg(Colors::CaptionButtonHover)
    .arg(Colors::CaptionButtonPressed)
    .arg(Colors::CloseButtonHover)
    .arg(Colors::CloseButtonPressed);
}

QString QalamTheme::editorStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamEditor {
            background-color: %1;
            color: %2;
            border: none;
            selection-background-color: %3;
            selection-color: %4;
        }
    )")
    .arg(Colors::EditorBackground)
    .arg(Colors::TextSecondary)
    .arg(Colors::Selection)
    .arg(Colors::TextPrimary);
}

QString QalamTheme::tabBarStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QTabWidget::pane {
            border: none;
            background: %1;
        }
        
        QTabBar {
            background-color: %2;
            border: none;
        }
        
        QTabBar::tab {
            background: %3;
            color: %4;
            padding: 7px 28px 7px 14px;
            border: none;
            border-right: 1px solid %5;
            border-top: 1px solid transparent;
            min-width: 110px;
            min-height: 20px;
        }
        
        QTabBar::tab:selected {
            background: %1;
            color: %6;
            border-top: 1px solid %7;
        }
        
        QTabBar::tab:hover:!selected {
            background: %8;
        }
        
        QTabBar::close-button {
            image: url(:/icons/resources/close.svg);
            background: transparent;
            border: none;
            padding: 2px;
            margin: 2px;
            subcontrol-position: right;
        }
        
        QTabBar::close-button:hover {
            background: %9;
            border-radius: 4px;
        }
    )")
    .arg(Colors::TabActiveBackground)     // %1
    .arg(Colors::SidebarBackground)       // %2
    .arg(Colors::TabBackground)           // %3
    .arg(Colors::TextMuted)               // %4
    .arg(Colors::TabBorder)               // %5
    .arg(Colors::TextPrimary)             // %6
    .arg(Colors::Accent)                  // %7
    .arg(Colors::TabHoverBackground)      // %8
    .arg(Colors::ButtonHover);            // %9
}

QString QalamTheme::consoleStyleSheet() {
    using namespace Constants;
    return QString(R"(
        QalamConsole {
            background-color: %1;
            color: %2;
            border: none;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: %3px;
        }
    )")
    .arg(Colors::ConsoleBackground)
    .arg(Colors::ConsoleText)
    .arg(Fonts::ConsoleSize);
}
