#pragma once
#include <QString>

namespace Constants {
    const QString OrgName = "BaaEcosystem";
    const QString AppName = "Qalam";
    const QString AppVersion = "3.4.0";
    
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

    // Session Keys
    const QString SessionKeyOpenFiles = "session/openFiles";
    const QString SessionKeyActiveTab = "session/activeTabIndex";
    const QString SessionKeyFolderPath = "session/folderPath";
    const QString SessionKeyWindowGeometry = "session/windowGeometry";
    const QString SessionKeyDocuments = "session/documents";
    const QString SessionKeyCleanShutdown = "session/cleanShutdown";

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
    // UI Colors - VS Code Dark+ (RTL-first)
    // ==========================================================================
    namespace Colors {
        // Surfaces
        constexpr const char* WindowBackground = "#1f1f1f";
        constexpr const char* EditorBackground = "#1f1f1f";
        constexpr const char* SidebarBackground = "#181818";
        constexpr const char* SidebarHeaderBackground = "#181818";
        constexpr const char* ConsoleBackground = "#1f1f1f";
        constexpr const char* MenuBackground = "#252526";

        // Activity Bar
        constexpr const char* ActivityBarBackground = "#181818";
        constexpr const char* ActivityBarBorder = "#2b2b2b";
        constexpr const char* IconInactive = "#c5c5c5";
        constexpr const char* IconActive = "#ffffff";
        constexpr const char* ActivityIndicator = "#007acc";
        constexpr const char* ActivityBarBadge = "#f14c4c";

        // Tabs
        constexpr const char* TabBackground = "#2d2d2d";
        constexpr const char* TabActiveBackground = "#1f1f1f";
        constexpr const char* TabHoverBackground = "#2b2d2e";
        constexpr const char* TabBorder = "#181818";

        // Accent
        constexpr const char* Accent = "#007acc";
        constexpr const char* AccentHover = "#1f8ad2";
        constexpr const char* AccentAlt = "#006bb3";

        // Selection
        constexpr const char* Selection = "#264f78";
        constexpr const char* SelectionHighlight = "#264f7840";
        constexpr const char* CurrentLineHighlight = "#2a2d2e";

        // Inputs
        constexpr const char* InputBackground = "#3c3c3c";
        constexpr const char* Border = "#3c3c3c";
        constexpr const char* BorderSubtle = "#2a2a2a";
        constexpr const char* BorderFocus = "#007acc";

        // Text
        constexpr const char* TextPrimary = "#d4d4d4";
        constexpr const char* TextSecondary = "#cccccc";
        constexpr const char* TextMuted = "#8a8a8a";
        constexpr const char* TextDisabled = "#5a5a5a";
        constexpr const char* ConsoleText = "#d4d4d4";

        // Buttons / caption buttons
        constexpr const char* ButtonHover = "#2a2d2e";
        constexpr const char* ButtonPressed = "#3e3e3e";
        constexpr const char* CaptionButtonHover = "#2a2d2e";
        constexpr const char* CaptionButtonPressed = "#3e3e3e";
        constexpr const char* CloseButtonHover = "#e81123";
        constexpr const char* CloseButtonPressed = "#c50f1f";

        // Status Bar
        constexpr const char* StatusBarBackground = "#007acc";
        constexpr const char* StatusBarForeground = "#ffffff";
        constexpr const char* StatusBarHover = "#1f8ad2";
        constexpr const char* StatusBarNoFolder = "#68217a";

        // Breadcrumb
        constexpr const char* BreadcrumbBackground = "#1f1f1f";
        constexpr const char* BreadcrumbForeground = "#9e9e9e";
        constexpr const char* BreadcrumbFocusForeground = "#e7e7e7";

        // Panel / Console Area
        constexpr const char* PanelBackground = "#1f1f1f";
        constexpr const char* PanelBorder = "#2a2a2a";
        constexpr const char* PanelTabActive = "#1e1e1e";
        constexpr const char* PanelTabInactive = "transparent";

        // Problems colors
        constexpr const char* ErrorForeground = "#f14c4c";
        constexpr const char* WarningForeground = "#cca700";
        constexpr const char* InfoForeground = "#3794ff";
        constexpr const char* SuccessForeground = "#4ec9b0";

        // Borders (semantic aliases)
        constexpr const char* BorderActive = "#007acc";
        constexpr const char* LineNumberBorder = "#007acc";

        // Scrollbar
        constexpr const char* ScrollbarBackground = "transparent";
        constexpr const char* ScrollbarThumb = "#424242";
        constexpr const char* ScrollbarThumbHover = "#4f4f4f";

        // List / Tree
        constexpr const char* ListHoverBackground = "#2a2d2e";
        constexpr const char* ListSelectionBackground = "#094771";
        constexpr const char* ListActiveBackground = "#094771";
        constexpr const char* ListInactiveBackground = "#37373d";

        // Title Bar
        constexpr const char* TitleBarBackground = "#181818";
        constexpr const char* TitleBarActiveBackground = "#181818";
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
        constexpr int CommandCenterMinWidth = 220;
        constexpr int CommandCenterMaxWidth = 430;

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
