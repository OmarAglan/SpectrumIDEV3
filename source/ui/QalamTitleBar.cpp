#include "QalamTitleBar.h"
#include "QalamTheme.h"
#include "Constants.h"
#include <QPainter>
#include <QStyleOption>
#include <QStyleOptionMenuItem>
#include <QSizePolicy>
#include <QGridLayout>
#include <QMenuBar>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWindow>

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
    // Keep the layout's true minimum compact. A fixed 220-430 px child made
    // the frameless window refuse to shrink far enough for the responsive
    // breakpoint to run, which felt like a frozen resize/drag operation.
    m_commandCenterBtn->setMinimumSize(0, 22);
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

    // The layout owns only the edge content. The command centre is overlaid at
    // the physical window centre when enough symmetric free space exists.
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

    // The command centre is positioned as an overlay in resizeEvent. Keeping
    // it out of the layout prevents its preferred width from imposing a large
    // minimum width on the frameless window.

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

    // Forward presses from the otherwise passive title-bar zones to Qt's
    // native system move operation.  This keeps Windows Snap, drag-to-restore,
    // mixed-DPI monitors and repeated dragging under the operating system's
    // control instead of emulating movement with widget coordinates.
    installEventFilter(this);
    for (QWidget *child : findChildren<QWidget*>()) {
        child->installEventFilter(this);
    }
}

bool QalamTitleBar::isInteractiveTitleBarChild(const QObject *object) const
{
    for (const QObject *candidate = object;
         candidate and candidate != this;
         candidate = candidate->parent()) {
        if (qobject_cast<const QPushButton*>(candidate) or
            qobject_cast<const QMenuBar*>(candidate)) {
            return true;
        }
    }
    return false;
}

bool QalamTitleBar::eventFilter(QObject *watched, QEvent *event)
{
    if (not isInteractiveTitleBarChild(watched)) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                emit maximizeRestoreClicked();
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
#if defined(Q_OS_WIN)
                // QalamWindow returns HTCAPTION for passive title-bar areas,
                // so Windows owns moving, Snap, drag-to-restore and DPI
                // transitions.  Calling startSystemMove() as well created two
                // competing native move loops and made repeated drags appear
                // intermittently stuck.
                return QWidget::eventFilter(watched, event);
#else
                QWidget *topLevel = window();
                if (topLevel and topLevel->windowHandle() and
                    topLevel->windowHandle()->startSystemMove()) {
                    mouseEvent->accept();
                    return true;
                }
#endif
            }
        }
    }
    return QWidget::eventFilter(watched, event);
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
    menu->setMinimumWidth(0);
    menu->setMaximumWidth(menuWidth);
    menu->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_menuWidget = menu;
    m_menuPreferredWidth = menuWidth;

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
    const int controlsWidth = 3 * Constants::Layout::CaptionButtonWidth;
    int menuWidth = 0;
    if (m_menuWidget) {
        const int availableForMenu = qMax(
            150, width() - controlsWidth - Constants::Layout::IconSize - 32);
        menuWidth = qMin(m_menuPreferredWidth, availableForMenu);
        m_menuWidget->setFixedWidth(menuWidth);
    }
    m_rightContentWidth = controlsWidth + menuWidth;

    // Use the space that the layout can actually give the command centre.
    // Mirroring the right-side width on both sides made it disappear on
    // ordinary laptop-sized windows even though ample space remained.
    const int centeredAvailable = width() - (2 * m_rightContentWidth);
    const bool showCommandCenter =
        width() >= Constants::Layout::CommandCenterMinWindowWidth
        and centeredAvailable >= Constants::Layout::CommandCenterMinWidth;
    m_commandCenterBtn->setVisible(showCommandCenter);
    if (showCommandCenter) {
        int targetWidth = qMin(centeredAvailable,
                               Constants::Layout::CommandCenterMaxWidth);
        // Matching parity keeps QRect::center() exactly aligned.
        if ((targetWidth & 1) != (width() & 1)) --targetWidth;
        m_commandCenterBtn->setGeometry(
            (width() - targetWidth) / 2,
            (height() - 22) / 2,
            targetWidth,
            22);
        m_commandCenterBtn->raise();
    }
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
