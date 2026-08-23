#include "QalamTitleBar.h"
#include "QalamTheme.h"
#include "Constants.h"
#include <QPainter>
#include <QStyleOption>
#include <QStyleOptionMenuItem>
#include <QSizePolicy>
#include <QGridLayout>
#include <QResizeEvent>

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
    m_commandCenterBtn->setIcon(QIcon(QStringLiteral(":/icons/resources/search.svg")));
    m_commandCenterBtn->setIconSize(QSize(14, 14));
    m_commandCenterBtn->setMinimumSize(Layout::CommandCenterMinWidth, 22);
    m_commandCenterBtn->setMaximumSize(Layout::CommandCenterMaxWidth, 22);
    m_commandCenterBtn->setSizePolicy(QSizePolicy::Expanding,
                                      QSizePolicy::Fixed);
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

    // Three equal logical zones keep the Command Center at the actual window
    // center even though the Arabic menu group on the right is much wider than
    // the logo on the left.
    auto *layout = new QGridLayout(this);
    layout->setContentsMargins(8, 0, 0, 0);
    layout->setSpacing(0);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 1);

    auto *leftZone = new QWidget(this);
    leftZone->setLayoutDirection(Qt::LeftToRight);
    auto *leftLayout = new QHBoxLayout(leftZone);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    leftLayout->addWidget(m_iconLabel);
    leftLayout->addStretch(1);
    layout->addWidget(leftZone, 0, 0);

    layout->addWidget(m_commandCenterBtn, 0, 1);

    auto *rightZone = new QWidget(this);
    rightZone->setLayoutDirection(Qt::LeftToRight);
    m_rightLayout = new QHBoxLayout(rightZone);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(0);
    m_rightLayout->addStretch(1);

    auto *controlsWidget = new QWidget(rightZone);
    auto *controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->addWidget(m_minimizeBtn);
    controlsLayout->addWidget(m_maximizeBtn);
    controlsLayout->addWidget(m_closeBtn);
    m_rightLayout->addWidget(controlsWidget);
    m_rightContentWidth = 3 * Layout::CaptionButtonWidth;
    layout->addWidget(rightZone, 0, 2);
}

void QalamTitleBar::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QalamTheme::instance().titleBarBackground());
}

void QalamTitleBar::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateCommandCenterWidth();
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
    
    menu->setLayoutDirection(Qt::RightToLeft);
    // QMenuBar::sizeHint() can collapse to roughly one item while the bar is
    // still being reparented into a custom title bar. Measure every action
    // through the active style instead, otherwise only "ملف" remains legible.
    int menuWidth = 0;
    for (QAction *action : menu->actions()) {
        QStyleOptionMenuItem option;
        option.initFrom(menu);
        option.menuItemType = QStyleOptionMenuItem::Normal;
        option.text = action->text();
        option.font = menu->font();
        option.fontMetrics = QFontMetrics(option.font);
        const QSize textSize(option.fontMetrics.horizontalAdvance(option.text),
                             option.fontMetrics.height());
        const int styledWidth = menu->style()->sizeFromContents(
            QStyle::CT_MenuBarItem, &option, textSize, menu).width();
        menuWidth += qMax(styledWidth, textSize.width() + 24);
    }
    menuWidth += 2 * menu->style()->pixelMetric(
        QStyle::PM_MenuBarHMargin, nullptr, menu);
    menuWidth = qMax(menuWidth, Constants::Layout::TitleMenuMinWidth);
    menu->setMinimumWidth(menuWidth);
    menu->setMaximumWidth(menuWidth);
    menu->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_rightContentWidth = menuWidth
        + (3 * Constants::Layout::CaptionButtonWidth);

    // The controls widget is the final item in the right zone. Insert the menu
    // immediately before it and after the flexible leading space.
    if (m_rightLayout) {
        m_rightLayout->insertWidget(qMax(0, m_rightLayout->count() - 1), menu);
    }

    menu->setFixedHeight(Constants::Layout::TitleBarHeight);
    updateCommandCenterWidth();
}

void QalamTitleBar::updateCommandCenterWidth()
{
    if (not m_commandCenterBtn) return;
    const int available = width() - (2 * m_rightContentWidth);
    const int targetWidth = qBound(Constants::Layout::CommandCenterMinWidth,
                                   available,
                                   Constants::Layout::CommandCenterMaxWidth);
    m_commandCenterBtn->setFixedWidth(targetWidth);
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
