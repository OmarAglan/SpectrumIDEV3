#include "QalamStatusBar.h"
#include "QalamTheme.h"
#include "Constants.h"
#include <QTimer>

QalamStatusBar::QalamStatusBar(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(Constants::Layout::StatusBarHeight);
    
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 0, 8, 0);
    m_layout->setSpacing(0);
    m_layout->setDirection(QBoxLayout::RightToLeft);  // RTL-first status bar: workspace indicators start on the right.
    
    // Primary/workspace indicators on the right in RTL.
    // Problems indicator
    m_problemsBtn = createStatusItem("", "المشاكل");
    m_problemsBtn->setVisible(false);  // Hidden until there are problems
    connect(m_problemsBtn, &QPushButton::clicked, this, &QalamStatusBar::problemsClicked);
    m_layout->addWidget(m_problemsBtn);
    
    // Branch name
    m_branchBtn = createStatusItem("", "الفرع الحالي");
    m_branchBtn->setVisible(false);  // Hidden until branch is set
    connect(m_branchBtn, &QPushButton::clicked, this, &QalamStatusBar::branchClicked);
    m_layout->addWidget(m_branchBtn);
    
    // Stretch in the middle
    m_layout->addStretch();
    
    // Message label (for temporary messages)
    m_messageLabel = new QLabel(this);
    m_messageLabel->setStyleSheet(QString("color: %1; padding: 0 8px;")
                                  .arg(Constants::Colors::StatusBarForeground));
    m_messageLabel->hide();
    m_layout->addWidget(m_messageLabel);
    
    m_layout->addStretch();
    
    // Contextual editor indicators on the left in RTL.
    // Cursor position: السطر X، العمود Y
    m_cursorPosBtn = createStatusItem("السطر 1، العمود 1", "الموضع");
    connect(m_cursorPosBtn, &QPushButton::clicked, this, &QalamStatusBar::cursorPositionClicked);
    m_layout->addWidget(m_cursorPosBtn);
    
    // Indentation
    m_indentationBtn = createStatusItem("مسافات: 4", "المسافة البادئة");
    connect(m_indentationBtn, &QPushButton::clicked, this, &QalamStatusBar::indentationClicked);
    m_layout->addWidget(m_indentationBtn);
    
    // Encoding
    m_encodingBtn = createStatusItem("UTF-8", "الترميز");
    connect(m_encodingBtn, &QPushButton::clicked, this, &QalamStatusBar::encodingClicked);
    m_layout->addWidget(m_encodingBtn);
    
    // Line ending
    m_lineEndingBtn = createStatusItem("LF", "نهاية السطر");
    connect(m_lineEndingBtn, &QPushButton::clicked, this, &QalamStatusBar::lineEndingClicked);
    m_layout->addWidget(m_lineEndingBtn);
    
    // Language
    m_languageBtn = createStatusItem("Baa", "اللغة");
    connect(m_languageBtn, &QPushButton::clicked, this, &QalamStatusBar::languageClicked);
    m_layout->addWidget(m_languageBtn);
    
    // Message timer
    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);
    connect(m_messageTimer, &QTimer::timeout, this, [this]() {
        m_messageLabel->hide();
    });
    
    applyStyles();
}

QPushButton* QalamStatusBar::createStatusItem(const QString& text, const QString& tooltip)
{
    QPushButton* btn = new QPushButton(text, this);
    btn->setFlat(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setToolTip(tooltip);
    
    btn->setStyleSheet(QString(R"(
        QPushButton {
            background: transparent;
            border: none;
            color: %1;
            padding: 0 %2px;
            font-size: %3px;
        }
        QPushButton:hover {
            background: %4;
        }
    )").arg(Constants::Colors::StatusBarForeground)
      .arg(Constants::Layout::StatusBarItemPadding)
      .arg(Constants::Fonts::StatusBarSize)
      .arg(Constants::Colors::StatusBarHover));
    
    return btn;
}

void QalamStatusBar::setCursorPosition(int line, int column)
{
    m_cursorPosBtn->setText(QString("السطر %1، العمود %2").arg(line).arg(column));
}

void QalamStatusBar::setEncoding(const QString& encoding)
{
    m_encodingBtn->setText(encoding);
}

void QalamStatusBar::setLineEnding(const QString& ending)
{
    m_lineEndingBtn->setText(ending);
}

void QalamStatusBar::setLanguage(const QString& language)
{
    m_languageBtn->setText(language);
}

void QalamStatusBar::setIndentation(const QString& indentation)
{
    m_indentationBtn->setText(indentation);
}

void QalamStatusBar::setProblemsCount(int errors, int warnings)
{
    m_errorCount = errors;
    m_warningCount = warnings;
    
    if (errors == 0 && warnings == 0) {
        m_problemsBtn->setVisible(false);
        return;
    }
    
    QString text;
    if (errors > 0) {
        text = QString("✕ %1").arg(errors);
    }
    if (warnings > 0) {
        if (!text.isEmpty()) text += "  ";
        text += QString("⚠ %1").arg(warnings);
    }
    
    m_problemsBtn->setText(text);
    m_problemsBtn->setVisible(true);
}

void QalamStatusBar::setBranch(const QString& branch)
{
    if (branch.isEmpty()) {
        m_branchBtn->setVisible(false);
        return;
    }
    
    m_branchBtn->setText(QString("⎇ %1").arg(branch));  // Git branch symbol
    m_branchBtn->setVisible(true);
}

void QalamStatusBar::setFolderOpen(bool open)
{
    if (m_folderOpen == open) {
        return;
    }
    m_folderOpen = open;
    updateBackgroundColor();
}

void QalamStatusBar::showMessage(const QString& message, int timeoutMs)
{
    m_messageLabel->setText(message);
    m_messageLabel->show();
    m_messageTimer->start(timeoutMs);
}

void QalamStatusBar::updateBackgroundColor()
{
    applyStyles();
}

void QalamStatusBar::applyStyles()
{
    QString bgColor = m_folderOpen ? Constants::Colors::StatusBarBackground 
                                   : Constants::Colors::StatusBarNoFolder;
    
    setStyleSheet(QString(R"(
        QalamStatusBar {
            background-color: %1;
        }
    )").arg(bgColor));
}
