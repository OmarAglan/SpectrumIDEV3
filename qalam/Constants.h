#pragma once
#include <QString>

namespace Constants {
    const QString OrgName = "BaaEcosystem";
    const QString AppName = "Qalam";
    const QString AppVersion = "3.6.0";
    
    // Settings Keys
    const QString SettingsKeyRecentFiles = "RecentFiles";
    const QString SettingsKeyRecentFolders = "RecentFolders";
    const QString SettingsKeyLastOpenLocation = "LastOpenLocation";
    const QString SettingsKeyFontSize = "editorFontSize";
    const QString SettingsKeyFontType = "editorFontType";
    const QString SettingsKeyTheme = "editorCodeTheme";
    const QString SettingsKeyCompilerPath = "compilerPath";
    const QString SettingsKeyTakweenPath = "takweenPath";
    const QString SettingsKeyNazmPath = "nazmPath";
    const QString SettingsKeyLanguageServerPath = "baaLspPath";
    const QString SettingsKeySidebarWidth = "sidebarWidth";
    const QString SettingsKeyPanelHeight = "panelHeight";
    const QString SettingsKeyShowWelcome = "ShowWelcomeOnStartup";
    const QString SettingsKeyCompletionUsage = "completion/usage-v1";

    // Session Keys
    const QString SessionKeyOpenFiles = "session/openFiles";
    const QString SessionKeyActiveTab = "session/activeTabIndex";
    const QString SessionKeyFolderPath = "session/folderPath";
    const QString SessionKeyFolderPaths = "session/folderPaths";
    const QString SessionKeyWindowGeometry = "session/windowGeometry";
    const QString SessionKeyDocuments = "session/documents";
    const QString SessionKeyCleanShutdown = "session/cleanShutdown";
    const QString SessionKeyEditorViews = "session/editorViews";
    const QString SessionKeyEditorGroupCount = "session/editorGroupCount";
    const QString SessionKeyEditorSplitOrientation = "session/editorSplitOrientation";
    const QString SessionKeyEditorSplitSizes = "session/editorSplitSizes";
    const QString SessionKeyActiveEditorGroup = "session/activeEditorGroup";

    // File Extensions
    const QString ProjectExtension = ".باء";
    const QString HeaderExtension = ".رأسباء";
    const QString LegacyProjectExtension = ".baa";
    const QString LegacyHeaderExtension = ".baahd";
    const QString BackupExtension = ".~";

    inline bool isBaaImplementationPath(const QString &path)
    {
        return path.endsWith(ProjectExtension, Qt::CaseInsensitive) or
               path.endsWith(LegacyProjectExtension, Qt::CaseInsensitive);
    }

    inline bool isBaaDocumentPath(const QString &path)
    {
        return isBaaImplementationPath(path) or
               path.endsWith(HeaderExtension, Qt::CaseInsensitive) or
               path.endsWith(LegacyHeaderExtension, Qt::CaseInsensitive);
    }

    // UI Strings (Arabic)
    const QString NewFileLabel = "غير معنون";
    const QString ExplorerLabel = "المستكشف";
    const QString SearchLabel = "البحث";
    const QString SourceControlLabel = "التحكم بالمصادر";
    const QString RunLabel = "تشغيل";
    const QString ExtensionsLabel = "الإضافات";
    const QString SettingsLabel = "الإعدادات";
    const QString ProblemsLabel = "المشاكل";
    const QString OutputLabel = "المخرجات";
    const QString TerminalLabel = "الطرفية";
    const QString OpenEditorsLabel = "الملفات المفتوحة";
    const QString NoFolderOpenLabel = "لم يتم فتح مجلد";

    // Defaults
    const int DefaultFontSize = 18;
    const QString DefaultFontType = "Kawkab Mono";

    // ==========================================================================
    // UI Colors - Qalam Sky (RTL-first)
    // ==========================================================================
    namespace Colors {
        // Surfaces
        constexpr const char* WindowBackground = "#10243c";
        constexpr const char* EditorBackground = "#10243c";
        constexpr const char* SidebarBackground = "#0b1a2b";
        constexpr const char* SidebarHeaderBackground = "#0d2035";
        constexpr const char* ConsoleBackground = "#0e2137";
        constexpr const char* MenuBackground = "#132a44";

        // Activity Bar
        constexpr const char* ActivityBarBackground = "#091827";
        constexpr const char* ActivityBarBorder = "#1d3b57";
        constexpr const char* IconInactive = "#9fc5df";
        constexpr const char* IconActive = "#f0f9ff";
        constexpr const char* ActivityIndicator = "#38bdf8";
        constexpr const char* ActivityBarBadge = "#f14c4c";

        // Tabs
        constexpr const char* TabBackground = "#132a44";
        constexpr const char* TabActiveBackground = "#10243c";
        constexpr const char* TabHoverBackground = "#183854";
        constexpr const char* TabBorder = "#0b1a2b";

        // Accent
        constexpr const char* Accent = "#38bdf8";
        constexpr const char* AccentHover = "#67d3fb";
        constexpr const char* AccentAlt = "#0284c7";

        // Selection
        constexpr const char* Selection = "#24577c";
        constexpr const char* SelectionHighlight = "#38bdf840";
        constexpr const char* CurrentLineHighlight = "#173653";

        // Inputs
        constexpr const char* InputBackground = "#173653";
        constexpr const char* Border = "#28506f";
        constexpr const char* BorderSubtle = "#1b3852";
        constexpr const char* BorderFocus = "#38bdf8";

        // Text
        constexpr const char* TextPrimary = "#e6f4ff";
        constexpr const char* TextSecondary = "#c7e2f4";
        constexpr const char* TextMuted = "#82a9c2";
        constexpr const char* TextDisabled = "#52758e";
        constexpr const char* ConsoleText = "#d8efff";

        // Buttons / caption buttons
        constexpr const char* ButtonHover = "#1b4260";
        constexpr const char* ButtonPressed = "#235572";
        constexpr const char* CaptionButtonHover = "#1b4260";
        constexpr const char* CaptionButtonPressed = "#235572";
        constexpr const char* CloseButtonHover = "#e81123";
        constexpr const char* CloseButtonPressed = "#c50f1f";

        // Status Bar
        constexpr const char* StatusBarBackground = "#0284c7";
        constexpr const char* StatusBarForeground = "#ffffff";
        constexpr const char* StatusBarHover = "#0ea5e9";
        constexpr const char* StatusBarNoFolder = "#0369a1";

        // Breadcrumb
        constexpr const char* BreadcrumbBackground = "#10243c";
        constexpr const char* BreadcrumbForeground = "#8fb8d2";
        constexpr const char* BreadcrumbFocusForeground = "#e6f4ff";

        // Panel / Console Area
        constexpr const char* PanelBackground = "#0e2137";
        constexpr const char* PanelBorder = "#1b3852";
        constexpr const char* PanelTabActive = "#102b47";
        constexpr const char* PanelTabInactive = "transparent";

        // Problems colors
        constexpr const char* ErrorForeground = "#f14c4c";
        constexpr const char* WarningForeground = "#cca700";
        constexpr const char* InfoForeground = "#3794ff";
        constexpr const char* SuccessForeground = "#4ec9b0";

        // Borders (semantic aliases)
        constexpr const char* BorderActive = "#38bdf8";
        constexpr const char* LineNumberBorder = "#38bdf8";

        // Scrollbar
        constexpr const char* ScrollbarBackground = "transparent";
        constexpr const char* ScrollbarThumb = "#315873";
        constexpr const char* ScrollbarThumbHover = "#42718f";

        // List / Tree
        constexpr const char* ListHoverBackground = "#173653";
        constexpr const char* ListSelectionBackground = "#1d4f73";
        constexpr const char* ListActiveBackground = "#1d4f73";
        constexpr const char* ListInactiveBackground = "#1a344c";

        // Title Bar
        constexpr const char* TitleBarBackground = "#0b1a2b";
        constexpr const char* TitleBarActiveBackground = "#0d2035";
    }

    // ==========================================================================
    // Font Sizes
    // ==========================================================================
    namespace Fonts {
        constexpr int ConsoleSize = 13;
        constexpr int UISize = 13;
        constexpr int StatusBarSize = 12;
        constexpr int TabSize = 13;
        constexpr int TreeViewSize = 13;
        constexpr int BreadcrumbSize = 12;
        constexpr int SectionHeaderSize = 11;
        constexpr int EditorMinSize = 12;
        constexpr int EditorMaxSize = 36;
    }

    // ==========================================================================
    // Layout Dimensions
    // ==========================================================================
    namespace Layout {
        // Title Bar
        constexpr int TitleBarHeight = 30;
        constexpr int CaptionButtonWidth = 46;
        constexpr int CaptionButtonHeight = 30;
        constexpr int CaptionIconSize = 14;
        constexpr int TitleMenuMinWidth = 420;
        constexpr int CommandCenterMinWidth = 200;
        constexpr int CommandCenterMaxWidth = 430;
        constexpr int CommandCenterMinWindowWidth = 840;

        // Activity Bar (VS Code-like)
        constexpr int ActivityBarWidth = 48;
        constexpr int ActivityBarIconSize = 24;
        constexpr int ActivityBarButtonSize = 48;
        constexpr int ActivityIndicatorWidth = 3;

        // Sidebar
        constexpr int SidebarDefaultWidth = 280;
        constexpr int SidebarMinWidth = 210;
        constexpr int SidebarMaxWidth = 520;
        constexpr int SidebarHeaderHeight = 32;
        constexpr int SidebarSectionHeaderHeight = 22;

        // Editor Area
        constexpr int TabBarHeight = 35;
        constexpr int BreadcrumbHeight = 22;
        constexpr int IconSize = 16;

        // Panel Area
        constexpr int PanelDefaultHeight = 210;
        constexpr int PanelMinHeight = 110;
        constexpr int PanelTabHeight = 30;
        
        // Status Bar
        constexpr int StatusBarHeight = 22;
        constexpr int StatusBarItemPadding = 8;
        
        // General
        constexpr int BorderRadius = 6;
        constexpr int SplitterWidth = 1;
        constexpr int ScrollbarWidth = 10;
        
        // Autocomplete popup
        constexpr int PopupMinWidth = 340;
        constexpr int PopupMaxWidth = 440;
    }

    // ==========================================================================
    // Timing Constants (milliseconds)
    // ==========================================================================
    namespace Timing {
        constexpr int FlushInterval = 25;
        constexpr int ProcessTerminateTimeout = 500;
        constexpr int ProcessKillTimeout = 200;
        constexpr int AutoSaveInterval = 30000;
        constexpr int SearchDebounce = 300;
        constexpr int HoverDelay = 500;
    }

    // ==========================================================================
    // Console Limits
    // ==========================================================================
    namespace Console {
        constexpr int MaxBufferLines = 10000;
        constexpr int MaxPendingLines = 5000;
    }
}
