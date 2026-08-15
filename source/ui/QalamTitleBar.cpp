#include "QalamTitleBar.h"
#include "QalamTheme.h"
#include "Constants.h"
#include <QPainter>
#include <QStyleOption>

QalamTitleBar::QalamTitleBar(QWidget *parent) : QWidget(parent) {
    setLayoutDirection(Qt::LeftToRight);
    setFixedHeight(Constants::Layout::TitleBarHeight);
    setupUi();
    setStyleSheet(QalamTheme::titleBarStyleSheet());
}

void QalamTitleBar::setupUi() {
    using namespace Constants;
    
    // Logo
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(Layout::IconSize, Layout::IconSize);
    m_iconLabel->setPixmap(QPixmap(":/icons/resources/QalamLogo.ico").scaled(
        Layout::IconSize, Layout::IconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    // Title + command center
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->hide();

    m_commandCenterBtn = new QPushButton("قلم  —  ابحث أو نفّذ أمرًا", this);
    m_commandCenterBtn->setObjectName("commandCenterButton");
    m_commandCenterBtn->setFixedSize(430, 22);
    m_commandCenterBtn->setCursor(Qt::PointingHandCursor);
    m_commandCenterBtn->setToolTip("لوحة الأوامر (Ctrl+Shift+P)");
    connect(m_commandCenterBtn, &QPushButton::clicked, this, &QalamTitleBar::commandCenterClicked);
    
    // Window Controls
    m_minimizeBtn = createCaptionButton(":/icons/resources/minimize.svg", "captionButton");
    m_maximizeBtn = createCaptionButton(":/icons/resources/maximize.svg", "captionButton");
    m_closeBtn = createCaptionButton(":/icons/resources/close.svg", "closeButton");

    connect(m_minimizeBtn, &QPushButton::clicked, this, &QalamTitleBar::minimizeClicked);
    connect(m_maximizeBtn, &QPushButton::clicked, this, &QalamTitleBar::maximizeRestoreClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QalamTitleBar::closeClicked);

    // Build layout in LTR style (like VSCode even for RTL apps)
    // Layout: [Icon][Menu]...stretch...[Title]...[Min][Max][Close]
    
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(0);
    layout->setDirection(QBoxLayout::LeftToRight);  // LTR for title bar
    
    // Left side: Logo
    layout->addWidget(m_iconLabel);
    layout->addSpacing(8);
    
    // Menu will be inserted at index 2 by addMenuBar()
    
    // Center: VS Code-like Command Center button
    layout->addStretch(1);
    layout->addWidget(m_commandCenterBtn);
    layout->addStretch(1);
    
    // Right side: Window controls (no spacing between caption buttons)
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->addWidget(m_minimizeBtn);
    controlsLayout->addWidget(m_maximizeBtn);
    controlsLayout->addWidget(m_closeBtn);
    
    layout->addLayout(controlsLayout);
}

void QalamTitleBar::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QalamTheme::instance().titleBarBackground());
}

void QalamTitleBar::setTitle(const QString &title) {
    m_titleLabel->setText(title);
    const QString cleanTitle = title.trimmed().isEmpty() ? QStringLiteral("قلم") : title;
    if (m_commandCenterBtn) {
        m_commandCenterBtn->setText(cleanTitle + QStringLiteral("  —  Ctrl+Shift+P"));
    }
}

void QalamTitleBar::setMaximizedState(bool maximized) {
    if (maximized) {
        m_maximizeBtn->setToolTip("restore");
        m_maximizeBtn->setIcon(QIcon(":/icons/resources/restore.svg"));
    } else {
        m_maximizeBtn->setToolTip("maximize");
        m_maximizeBtn->setIcon(QIcon(":/icons/resources/maximize.svg"));
    }
}

void QalamTitleBar::addMenuBar(QWidget *menu) {
    if (!menu) return;
    
    menu->setLayoutDirection(Qt::LeftToRight);

    // Insert menu after icon (index 2: Icon=0, Spacing=1, then insert at 2)
    if (QHBoxLayout *l = qobject_cast<QHBoxLayout*>(layout())) {
        l->insertWidget(2, menu);
    }

    menu->setFixedHeight(Constants::Layout::TitleBarHeight);
}

QPushButton* QalamTitleBar::createCaptionButton(const QString &iconPath, const QString &objName) {
    using namespace Constants;
    
    QPushButton *btn = new QPushButton(this);
    btn->setObjectName(objName);
    btn->setFixedSize(Layout::CaptionButtonWidth, Layout::CaptionButtonHeight);
    btn->setIcon(QIcon(iconPath));
    btn->setIconSize(QSize(Layout::CaptionIconSize, Layout::CaptionIconSize));
    
    return btn;
}
