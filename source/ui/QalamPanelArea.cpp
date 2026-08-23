#include "QalamPanelArea.h"
#include "QalamTheme.h"
#include "Constants.h"
#include "QalamConsole.h"
#include <QScrollArea>
#include <QTextEdit>
#include <QMouseEvent>
#include <QGroupBox>
#include <QShowEvent>
#include <QSplitter>
#include <QSizePolicy>
#include <QTimer>

QalamPanelArea::QalamPanelArea(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumHeight(Constants::Layout::PanelMinHeight);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    setupUI();
    applyStyles();
}

void QalamPanelArea::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Header bar with tabs and controls
    m_mainLayout->addWidget(createHeaderBar());
    
    // Stacked widget for tab contents
    m_stackedWidget = new QStackedWidget(this);
    m_mainLayout->addWidget(m_stackedWidget, 1);
    
    // Setup individual views
    setupProblemsView();
    setupOutputView();
    setupTerminal();
    setupDebugView();
    
    // Add to stacked widget
    m_stackedWidget->addWidget(m_problemsView);
    m_stackedWidget->addWidget(m_outputView);
    m_stackedWidget->addWidget(m_terminal);
    m_stackedWidget->addWidget(m_debugView);
    
    // Connect tab bar
    connect(m_tabBar, &QTabBar::currentChanged, this, [this](int index) {
        m_stackedWidget->setCurrentIndex(index);
        emit tabChanged(static_cast<Tab>(index));
    });
}

QWidget* QalamPanelArea::createHeaderBar()
{
    QWidget* header = new QWidget(this);
    header->setFixedHeight(Constants::Layout::PanelTabHeight);
    header->setObjectName("panelHeader");
    
    QHBoxLayout* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 4, 0);
    layout->setSpacing(0);
    layout->setDirection(QBoxLayout::RightToLeft);  // RTL
    
    // Tab bar on the right
    setupTabBar();
    layout->addWidget(m_tabBar);
    
    // Stretch in the middle
    layout->addStretch();
    
    // Control buttons on the left (in RTL, added last)
    m_maximizeBtn = new QPushButton(this);
    m_maximizeBtn->setObjectName(QStringLiteral("panelMaximizeButton"));
    m_maximizeBtn->setFixedSize(20, 20);
    m_maximizeBtn->setIcon(QIcon(QStringLiteral(":/icons/resources/maximize.svg")));
    m_maximizeBtn->setIconSize(QSize(14, 14));
    m_maximizeBtn->setCursor(Qt::PointingHandCursor);
    m_maximizeBtn->setToolTip("تكبير");
    connect(m_maximizeBtn, &QPushButton::clicked, this, &QalamPanelArea::maximizeRequested);
    layout->addWidget(m_maximizeBtn);
    
    m_closeBtn = new QPushButton(this);
    m_closeBtn->setObjectName(QStringLiteral("panelCloseButton"));
    m_closeBtn->setFixedSize(20, 20);
    m_closeBtn->setIcon(QIcon(QStringLiteral(":/icons/resources/close.svg")));
    m_closeBtn->setIconSize(QSize(14, 14));
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setToolTip("إغلاق");
    connect(m_closeBtn, &QPushButton::clicked, this, &QalamPanelArea::closeRequested);
    layout->addWidget(m_closeBtn);
    
    return header;
}

void QalamPanelArea::setupTabBar()
{
    m_tabBar = new QTabBar(this);
    m_tabBar->setLayoutDirection(Qt::RightToLeft);
    m_tabBar->setDocumentMode(true);
    m_tabBar->setExpanding(false);
    
    // Add tabs - RTL order (rightmost first)
    m_tabBar->addTab(Constants::ProblemsLabel);
    m_tabBar->addTab(Constants::OutputLabel);
    m_tabBar->addTab(Constants::TerminalLabel);
    m_tabBar->addTab("تصحيح");
    
    // Create problems badge
    m_problemsBadge = new QLabel(this);
    m_problemsBadge->setFixedSize(18, 14);
    m_problemsBadge->setAlignment(Qt::AlignCenter);
    m_problemsBadge->hide();
}

void QalamPanelArea::setupProblemsView()
{
    m_problemsView = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_problemsView);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_problemsStack = new QStackedWidget(m_problemsView);
    m_problemsStack->setObjectName(QStringLiteral("problemsStack"));

    m_problemsEmptyState = new QLabel(
        QStringLiteral("لا توجد مشاكل\nستظهر تشخيصات باء وتكوين هنا."),
        m_problemsStack);
    m_problemsEmptyState->setObjectName(QStringLiteral("problemsEmptyState"));
    m_problemsEmptyState->setAlignment(Qt::AlignCenter);
    m_problemsEmptyState->setWordWrap(true);

    // Scroll area for problems
    QScrollArea* scroll = new QScrollArea(m_problemsView);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);
    
    m_problemsContent = new QWidget(scroll);
    m_problemsLayout = new QVBoxLayout(m_problemsContent);
    m_problemsLayout->setContentsMargins(8, 4, 8, 4);
    m_problemsLayout->setSpacing(2);
    m_problemsLayout->setAlignment(Qt::AlignTop);
    
    scroll->setWidget(m_problemsContent);
    m_problemsStack->addWidget(m_problemsEmptyState);
    m_problemsStack->addWidget(scroll);
    m_problemsStack->setCurrentWidget(m_problemsEmptyState);
    layout->addWidget(m_problemsStack);
}

void QalamPanelArea::setupOutputView()
{
    m_outputView = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(m_outputView);
    layout->setContentsMargins(0, 0, 0, 0);

    m_outputStack = new QStackedWidget(m_outputView);
    m_outputStack->setObjectName(QStringLiteral("outputStack"));

    m_outputEmptyState = new QLabel(
        QStringLiteral("لا توجد رسائل\nستظهر أحداث خادم اللغة وعمليات البناء هنا."),
        m_outputStack);
    m_outputEmptyState->setObjectName(QStringLiteral("outputEmptyState"));
    m_outputEmptyState->setAlignment(Qt::AlignCenter);
    m_outputEmptyState->setWordWrap(true);

    m_outputText = new QPlainTextEdit(m_outputStack);
    m_outputText->setObjectName(QStringLiteral("languageOutput"));
    m_outputText->setReadOnly(true);
    m_outputText->setMaximumBlockCount(500);
    m_outputText->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_outputText->setLayoutDirection(Qt::RightToLeft);
    m_outputText->setFrameShape(QFrame::NoFrame);
    m_outputText->setStyleSheet(
        QString("QPlainTextEdit { padding: 8px; color: %1; background: transparent; }")
            .arg(Constants::Colors::TextSecondary));

    m_outputStack->addWidget(m_outputEmptyState);
    m_outputStack->addWidget(m_outputText);
    m_outputStack->setCurrentWidget(m_outputEmptyState);
    layout->addWidget(m_outputStack);
}

void QalamPanelArea::setupTerminal()
{
    m_terminal = new QalamConsole(this);
}


void QalamPanelArea::setupDebugView()
{
    m_debugView = new QWidget(this);
    auto *layout = new QVBoxLayout(m_debugView);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignTop);

    auto *title = new QLabel("لوحة التصحيح", m_debugView);
    title->setObjectName("debugTitle");
    title->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(title);

    auto *hint = new QLabel(
        "هذه واجهة تصحيح أولية تشبه VS Code.\n"
        "حاليًا يمكنك تشغيل البرنامج عبر F5، وستكون نقاط التوقف والمتغيرات ومكدس الاستدعاء جاهزة للربط عندما يدعم مترجم باء مصححًا حقيقيًا.",
        m_debugView);
    hint->setObjectName("debugHint");
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *buttonsRow = new QHBoxLayout();
    buttonsRow->setDirection(QBoxLayout::RightToLeft);
    buttonsRow->setSpacing(8);
    auto *runButton = new QPushButton("تشغيل F5", m_debugView);
    runButton->setObjectName("debugRunButton");
    runButton->setIcon(QIcon(QStringLiteral(":/icons/resources/run.svg")));
    runButton->setIconSize(QSize(16, 16));
    runButton->setToolTip("استخدم F5 لتشغيل ملف باء الحالي");
    buttonsRow->addWidget(runButton);
    auto *stopButton = new QPushButton("إيقاف", m_debugView);
    stopButton->setObjectName("debugStopButton");
    stopButton->setIcon(QIcon(QStringLiteral(":/icons/resources/stop.svg")));
    stopButton->setIconSize(QSize(16, 16));
    stopButton->setEnabled(false);
    buttonsRow->addWidget(stopButton);
    buttonsRow->addStretch(1);
    layout->addLayout(buttonsRow);

    auto *stackBox = new QGroupBox("مكدس الاستدعاء", m_debugView);
    auto *stackLayout = new QVBoxLayout(stackBox);
    stackLayout->addWidget(new QLabel("لا توجد جلسة تصحيح نشطة", stackBox));
    layout->addWidget(stackBox);

    auto *varsBox = new QGroupBox("المتغيرات", m_debugView);
    auto *varsLayout = new QVBoxLayout(varsBox);
    varsLayout->addWidget(new QLabel("ستظهر المتغيرات هنا عند توفر مصحح باء", varsBox));
    layout->addWidget(varsBox);
}

void QalamPanelArea::setCurrentTab(Tab tab)
{
    m_tabBar->setCurrentIndex(static_cast<int>(tab));
}

QalamPanelArea::Tab QalamPanelArea::currentTab() const
{
    return static_cast<Tab>(m_tabBar->currentIndex());
}

void QalamPanelArea::addProblem(const QString& message, const QString& file,
                            int line, int column, const QString& severity)
{
    if (m_problemsStack) m_problemsStack->setCurrentIndex(1);

    QWidget* item = new QWidget(m_problemsContent);
    QHBoxLayout* layout = new QHBoxLayout(item);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(8);
    layout->setDirection(QBoxLayout::RightToLeft);
    
    // Severity icon
    QLabel* icon = new QLabel(item);
    QString iconColor;
    QString iconText;
    if (severity == "error") {
        iconColor = Constants::Colors::ErrorForeground;
        iconText = "✕";
        m_errorCount++;
    } else if (severity == "warning") {
        iconColor = Constants::Colors::WarningForeground;
        iconText = "⚠";
        m_warningCount++;
    } else {
        iconColor = Constants::Colors::InfoForeground;
        iconText = "ℹ";
    }
    icon->setText(iconText);
    icon->setStyleSheet(QString("color: %1;").arg(iconColor));
    layout->addWidget(icon);
    
    // Message
    QLabel* msgLabel = new QLabel(message, item);
    msgLabel->setStyleSheet(QString("color: %1;").arg(Constants::Colors::TextPrimary));
    layout->addWidget(msgLabel, 1);
    
    // File location
    QString location = QString("%1:%2:%3").arg(file).arg(line).arg(column);
    QLabel* locLabel = new QLabel(location, item);
    locLabel->setStyleSheet(QString("color: %1;").arg(Constants::Colors::TextMuted));
    layout->addWidget(locLabel);
    
    // Make clickable
    item->setCursor(Qt::PointingHandCursor);
    item->setProperty("file", file);
    item->setProperty("line", line);
    item->setProperty("column", column);
    
    item->installEventFilter(this);
    
    m_problemsLayout->addWidget(item);
    updateProblemsBadge();
}

void QalamPanelArea::clearProblems()
{
    while (m_problemsLayout->count() > 0) {
        QLayoutItem* item = m_problemsLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    m_errorCount = 0;
    m_warningCount = 0;
    if (m_problemsStack and m_problemsEmptyState) {
        m_problemsStack->setCurrentWidget(m_problemsEmptyState);
    }
    updateProblemsBadge();
}

int QalamPanelArea::problemCount() const
{
    return m_errorCount + m_warningCount;
}

void QalamPanelArea::updateProblemsBadge()
{
    int count = problemCount();
    if (count > 0) {
        m_problemsBadge->setText(QString::number(count > 99 ? 99 : count));
        m_problemsBadge->setStyleSheet(QString(R"(
            QLabel {
                background: %1;
                color: white;
                border-radius: 7px;
                font-size: 10px;
                font-weight: bold;
            }
        )").arg(m_errorCount > 0 ? Constants::Colors::ErrorForeground 
                                 : Constants::Colors::WarningForeground));
        m_problemsBadge->show();
    } else {
        m_problemsBadge->hide();
    }
}

void QalamPanelArea::appendOutput(const QString& text)
{
    if (!m_outputText || text.isEmpty()) return;
    if (text == m_lastOutputEntry) return;
    if (m_outputStack) m_outputStack->setCurrentWidget(m_outputText);
    m_lastOutputEntry = text;
    QTextCursor cursor = m_outputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    m_outputText->setTextCursor(cursor);
    m_outputText->ensureCursorVisible();
}

void QalamPanelArea::clearOutput()
{
    m_lastOutputEntry.clear();
    if (m_outputText) m_outputText->clear();
    if (m_outputStack and m_outputEmptyState) {
        m_outputStack->setCurrentWidget(m_outputEmptyState);
    }
}

QString QalamPanelArea::outputText() const
{
    return m_outputText ? m_outputText->toPlainText() : QString();
}

int QalamPanelArea::outputBlockCount() const
{
    return m_outputText ? m_outputText->blockCount() : 0;
}

void QalamPanelArea::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    m_stackedWidget->setVisible(!collapsed);
    emit collapseToggled(collapsed);
}

void QalamPanelArea::setMaximizedState(bool maximized)
{
    if (m_maximized == maximized) return;
    m_maximized = maximized;
    m_maximizeBtn->setIcon(QIcon(maximized
        ? QStringLiteral(":/icons/resources/restore.svg")
        : QStringLiteral(":/icons/resources/maximize.svg")));
    m_maximizeBtn->setToolTip(maximized
        ? QStringLiteral("استعادة الحجم")
        : QStringLiteral("تكبير"));
}

void QalamPanelArea::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (m_initialHeightApplied) return;
    m_initialHeightApplied = true;

    // QSplitter recalculates its children after their show events. Apply the
    // initial VS Code-like panel height on the following event-loop turn so it
    // is not overwritten by that layout pass.
    QTimer::singleShot(0, this, [this]() {
        auto *splitter = qobject_cast<QSplitter*>(parentWidget());
        if (not isVisible() or not splitter
            or splitter->orientation() != Qt::Vertical) {
            m_initialHeightApplied = false;
            return;
        }

        const int totalHeight = splitter->height();
        if (totalHeight <= Constants::Layout::PanelMinHeight) {
            m_initialHeightApplied = false;
            return;
        }

        const int panelHeight = qBound(
            Constants::Layout::PanelMinHeight,
            Constants::Layout::PanelDefaultHeight,
            totalHeight / 2);
        splitter->setSizes({qMax(1, totalHeight - panelHeight), panelHeight});
    });
}

bool QalamPanelArea::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QWidget* widget = qobject_cast<QWidget*>(watched);
        if (widget) {
            QString file = widget->property("file").toString();
            int line = widget->property("line").toInt();
            int column = widget->property("column").toInt();
            if (!file.isEmpty()) {
                emit problemClicked(file, line, column);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void QalamPanelArea::applyStyles()
{
    setStyleSheet(QString(R"(
        QalamPanelArea {
            background-color: %1;
            border-top: 1px solid %2;
        }
        
        #panelHeader {
            background-color: %1;
            border-bottom: 1px solid %2;
        }
        
        QTabBar::tab {
            background: transparent;
            color: %3;
            padding: 8px 12px;
            border: none;
            border-bottom: 2px solid transparent;
        }
        
        QTabBar::tab:hover {
            color: %4;
        }
        
        QTabBar::tab:selected {
            color: %4;
            border-bottom: 2px solid %5;
        }
        
        QPushButton {
            background: transparent;
            border: none;
            color: %3;
            font-size: 14px;
        }
        
        QPushButton:hover {
            color: %4;
            background: %6;
        }
        
        QScrollArea {
            background: %1;
            border: none;
        }
        
        QScrollArea > QWidget > QWidget {
            background: %1;
        }

        #problemsEmptyState, #outputEmptyState {
            color: %3;
            font-size: 13px;
            padding: 24px;
        }

        #debugTitle {
            color: %4;
            font-size: 15px;
            font-weight: 600;
        }

        #debugHint {
            color: %3;
            line-height: 140%;
        }

        #debugActionButton {
            border: 1px solid %2;
            border-radius: 4px;
            padding: 5px 10px;
            background: %6;
            color: %4;
        }

        QGroupBox {
            color: %4;
            border: 1px solid %2;
            border-radius: 6px;
            margin-top: 8px;
            padding: 8px;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top right;
            padding: 0 6px;
        }
    )").arg(Constants::Colors::PanelBackground)
      .arg(Constants::Colors::PanelBorder)
      .arg(Constants::Colors::TextMuted)
      .arg(Constants::Colors::TextPrimary)
      .arg(Constants::Colors::ActivityIndicator)
      .arg(Constants::Colors::ListHoverBackground));
}
