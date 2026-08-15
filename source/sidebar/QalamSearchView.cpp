#include "QalamSearchView.h"
#include "../ui/QalamTheme.h"
#include "Constants.h"
#include <QTimer>
#include <QHeaderView>
#include <QFileInfo>
#include <QRegularExpression>

QalamSearchView::QalamSearchView(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    applyStyles();
}

void QalamSearchView::setupUi()
{
    using namespace Constants;
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(8);
    
    // ========== Search Input Section ==========
    m_inputContainer = new QWidget();
    QVBoxLayout *inputLayout = new QVBoxLayout(m_inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(6);
    
    // Search input with icon
    QWidget *searchRow = new QWidget();
    QHBoxLayout *searchRowLayout = new QHBoxLayout(searchRow);
    searchRowLayout->setContentsMargins(0, 0, 0, 0);
    searchRowLayout->setSpacing(4);
    
    m_searchInput = new QLineEdit();
    m_searchInput->setObjectName("searchInput");
    m_searchInput->setPlaceholderText("بحث");
    m_searchInput->addAction(
        QIcon(QStringLiteral(":/icons/resources/search.svg")),
        QLineEdit::LeadingPosition);
    m_searchInput->setClearButtonEnabled(true);
    
    searchRowLayout->addWidget(m_searchInput);
    
    // Replace input (hidden by default)
    m_replaceInput = new QLineEdit();
    m_replaceInput->setObjectName("replaceInput");
    m_replaceInput->setPlaceholderText("استبدال");
    m_replaceInput->addAction(
        QIcon(QStringLiteral(":/icons/resources/replace.svg")),
        QLineEdit::LeadingPosition);
    m_replaceInput->setClearButtonEnabled(true);
    m_replaceInput->hide();
    
    inputLayout->addWidget(searchRow);
    inputLayout->addWidget(m_replaceInput);
    
    m_mainLayout->addWidget(m_inputContainer);
    
    // ========== Search Options ==========
    m_optionsContainer = new QWidget();
    QHBoxLayout *optionsLayout = new QHBoxLayout(m_optionsContainer);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(4);
    
    m_caseSensitiveBtn = new QPushButton("Aa");
    m_caseSensitiveBtn->setObjectName("optionBtn");
    m_caseSensitiveBtn->setToolTip("مطابقة حالة الأحرف");
    m_caseSensitiveBtn->setCheckable(true);
    m_caseSensitiveBtn->setFixedSize(28, 24);
    
    m_wholeWordBtn = new QPushButton("ab");
    m_wholeWordBtn->setObjectName("optionBtn");
    m_wholeWordBtn->setToolTip("كلمة كاملة فقط");
    m_wholeWordBtn->setCheckable(true);
    m_wholeWordBtn->setFixedSize(28, 24);
    
    m_regexBtn = new QPushButton(".*");
    m_regexBtn->setObjectName("optionBtn");
    m_regexBtn->setToolTip("تعبير نمطي (Regex)");
    m_regexBtn->setCheckable(true);
    m_regexBtn->setFixedSize(28, 24);
    
    m_toggleReplaceBtn = new QPushButton();
    m_toggleReplaceBtn->setIcon(QIcon(":/icons/resources/right-arrow.svg"));
    m_toggleReplaceBtn->setIconSize(QSize(14, 14));
    m_toggleReplaceBtn->setObjectName("toggleReplaceBtn");
    m_toggleReplaceBtn->setToolTip("إظهار/إخفاء الاستبدال");
    m_toggleReplaceBtn->setFixedSize(24, 24);
    
    optionsLayout->addWidget(m_toggleReplaceBtn);
    optionsLayout->addStretch();
    optionsLayout->addWidget(m_caseSensitiveBtn);
    optionsLayout->addWidget(m_wholeWordBtn);
    optionsLayout->addWidget(m_regexBtn);
    
    m_mainLayout->addWidget(m_optionsContainer);
    
    // ========== Result Summary ==========
    m_resultSummary = new QLabel("");
    m_resultSummary->setObjectName("resultSummary");
    m_resultSummary->hide();
    m_mainLayout->addWidget(m_resultSummary);
    
    // ========== Results Tree ==========
    m_resultsTree = new QTreeWidget();
    m_resultsTree->setObjectName("resultsTree");
    m_resultsTree->setHeaderHidden(true);
    m_resultsTree->setIndentation(16);
    m_resultsTree->setAnimated(true);
    m_resultsTree->setExpandsOnDoubleClick(true);
    m_resultsTree->setRootIsDecorated(true);
    
    m_mainLayout->addWidget(m_resultsTree, 1);
    
    // ========== Connections ==========
    m_searchDebounce = new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(Timing::SearchDebounce);
    
    connect(m_searchInput, &QLineEdit::textChanged, this, &QalamSearchView::onSearchTextChanged);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &QalamSearchView::onSearchTriggered);
    connect(m_searchDebounce, &QTimer::timeout, this, &QalamSearchView::onSearchTriggered);
    connect(m_resultsTree, &QTreeWidget::itemClicked, this, &QalamSearchView::onResultItemClicked);
    
    connect(m_toggleReplaceBtn, &QPushButton::clicked, this, [this]() {
        m_replaceVisible = !m_replaceVisible;
        m_replaceInput->setVisible(m_replaceVisible);
        m_toggleReplaceBtn->setIcon(QIcon(m_replaceVisible ? ":/icons/resources/down-arrow.svg" : ":/icons/resources/right-arrow.svg"));
    });
    
    connect(m_caseSensitiveBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_caseSensitive = checked;
        onSearchTriggered();
    });
    
    connect(m_wholeWordBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_wholeWord = checked;
        onSearchTriggered();
    });
    
    connect(m_regexBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_useRegex = checked;
        onSearchTriggered();
    });
}

void QalamSearchView::onSearchTextChanged()
{
    m_searchDebounce->start();
}

void QalamSearchView::onSearchTriggered()
{
    QString query = m_searchInput->text();
    if (query.trimmed().isEmpty()) {
        clearResults();
        setResultCount(0, 0);
        return;
    }
    if (query.length() < 2) {
        clearResults();
        return;
    }

    // Validate regex before emitting
    if (m_useRegex) {
        QRegularExpression re(query);
        if (not re.isValid()) {
            m_searchInput->setStyleSheet("QLineEdit#searchInput { border: 1px solid #f44747; }");
            m_resultSummary->setText("تعبير نمطي غير صالح: " + re.errorString());
            m_resultSummary->show();
            return;
        }
    }

    // Reset input border to normal
    m_searchInput->setStyleSheet("");

    emit searchRequested(query, m_caseSensitive, m_wholeWord, m_useRegex);
}

void QalamSearchView::onResultItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    
    // Only handle leaf items (actual matches, not file headers)
    if (item->childCount() > 0) return;
    
    QString filePath = item->data(0, Qt::UserRole).toString();
    int line = item->data(0, Qt::UserRole + 1).toInt();
    int col = item->data(0, Qt::UserRole + 2).toInt();
    
    if (!filePath.isEmpty()) {
        emit resultClicked(filePath, line, col);
    }
}

void QalamSearchView::setSearchPath(const QString &path)
{
    m_searchPath = path;
}

void QalamSearchView::focusSearchInput()
{
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

void QalamSearchView::clearResults()
{
    m_resultsTree->clear();
    m_resultSummary->hide();
}

void QalamSearchView::addResult(const QString &filePath, int line, int column,
                            const QString &lineText, const QString &matchText)
{
    // Find or create file item
    QTreeWidgetItem *fileItem = nullptr;
    for (int i = 0; i < m_resultsTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_resultsTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == filePath) {
            fileItem = item;
            break;
        }
    }
    
    if (!fileItem) {
        QFileInfo info(filePath);
        fileItem = new QTreeWidgetItem(m_resultsTree);
        fileItem->setText(0, info.fileName());
        fileItem->setData(0, Qt::UserRole, filePath);
        fileItem->setIcon(0, QIcon(":/icons/resources/file-new.svg"));
        fileItem->setExpanded(true);
    }
    
    // Add match item
    QTreeWidgetItem *matchItem = new QTreeWidgetItem(fileItem);
    QString displayText = QString("%1: %2").arg(line).arg(lineText.trimmed());
    matchItem->setText(0, displayText);
    matchItem->setData(0, Qt::UserRole, filePath);
    matchItem->setData(0, Qt::UserRole + 1, line);
    matchItem->setData(0, Qt::UserRole + 2, column);
    matchItem->setToolTip(0, lineText);
    
    // Update file item count
    int count = fileItem->childCount();
    fileItem->setText(0, QString("%1 (%2)").arg(QFileInfo(filePath).fileName()).arg(count));
}

void QalamSearchView::setSearching(bool searching)
{
    m_searchInput->setEnabled(!searching);
    // Could add a spinner here
}

void QalamSearchView::setResultCount(int fileCount, int matchCount)
{
    if (matchCount == 0) {
        m_resultSummary->setText("لا توجد نتائج");
    } else {
        m_resultSummary->setText(QString("%1 نتيجة في %2 ملف").arg(matchCount).arg(fileCount));
    }
    m_resultSummary->show();
}

void QalamSearchView::applyStyles()
{
    using namespace Constants;
    
    QString styles = QString(R"(
        QalamSearchView {
            background-color: %1;
        }
        
        /* Search input */
        #searchInput, #replaceInput {
            background-color: %2;
            border: 1px solid %3;
            border-radius: 3px;
            padding: 6px 8px;
            color: %4;
            font-size: %5px;
        }
        
        #searchInput:focus, #replaceInput:focus {
            border-color: %6;
        }
        
        #searchInput::placeholder, #replaceInput::placeholder {
            color: %7;
        }
        
        /* Option buttons */
        #optionBtn {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 3px;
            color: %7;
            font-size: 11px;
            font-weight: bold;
        }
        
        #optionBtn:hover {
            background-color: %8;
        }
        
        #optionBtn:checked {
            background-color: %6;
            color: %4;
            border-color: %6;
        }
        
        #toggleReplaceBtn {
            background-color: transparent;
            border: none;
            color: %7;
            font-size: 10px;
        }
        
        #toggleReplaceBtn:hover {
            color: %4;
        }
        
        /* Result summary */
        #resultSummary {
            color: %7;
            font-size: %9px;
            padding: 4px 0;
        }
        
        /* Results tree */
        #resultsTree {
            background-color: %1;
            border: none;
            color: %4;
            font-size: %5px;
            outline: none;
        }
        
        #resultsTree::item {
            padding: 3px 0;
        }
        
        #resultsTree::item:hover {
            background-color: %8;
        }
        
        #resultsTree::item:selected {
            background-color: %10;
        }
        
        #resultsTree::branch {
            background: transparent;
        }
        
        /* Scrollbar */
        QScrollBar:vertical {
            background: transparent;
            width: %11px;
        }
        
        QScrollBar::handle:vertical {
            background: %12;
            border-radius: 4px;
            min-height: 30px;
        }
        
        QScrollBar::handle:vertical:hover {
            background: %13;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )")
    .arg(Colors::SidebarBackground)          // %1
    .arg(Colors::EditorBackground)           // %2
    .arg(Colors::Border)                     // %3
    .arg(Colors::TextSecondary)              // %4
    .arg(Fonts::TreeViewSize)                // %5
    .arg(Colors::Accent)                     // %6
    .arg(Colors::TextMuted)                  // %7
    .arg(Colors::ListHoverBackground)        // %8
    .arg(Fonts::SectionHeaderSize)           // %9
    .arg(Colors::ListActiveBackground)       // %10
    .arg(Layout::ScrollbarWidth)             // %11
    .arg(Colors::ScrollbarThumb)             // %12
    .arg(Colors::ScrollbarThumbHover);       // %13
    
    setStyleSheet(styles);
}
