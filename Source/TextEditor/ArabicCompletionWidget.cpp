#include "ArabicCompletionWidget.h"
#include "../LspClient/SpectrumLspClient.h"

#include <QApplication>
#include <QKeyEvent>
#include <QTextCursor>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QJsonDocument>
#include <QDebug>
#include <QFontDatabase>
#include <QScreen>
#include <algorithm>

// ArabicCompletionItem implementation
ArabicCompletionItem::ArabicCompletionItem(const QJsonObject& json) {
    label = json.value("label").toString();
    arabicName = json.value("arabicName").toString();
    englishName = json.value("englishName").toString();
    kind = json.value("kind").toInt();
    
    arabicDescription = json.value("arabicDescription").toString();
    arabicDetailedDesc = json.value("arabicDetailedDesc").toString();
    usageExample = json.value("usageExample").toString();
    arabicExample = json.value("arabicExample").toString();
    
    parameters = json.value("parameters").toArray();
    returnType = json.value("returnType").toString();
    arabicReturnDesc = json.value("arabicReturnDesc").toString();
    
    priority = json.value("priority").toInt(50);
    
    // Convert JSON arrays to QStringList
    QJsonArray contextsArray = json.value("contexts").toArray();
    for (const auto& value : contextsArray) {
        contexts << value.toString();
    }
    
    QJsonArray tagsArray = json.value("tags").toArray();
    for (const auto& value : tagsArray) {
        tags << value.toString();
    }
    
    category = json.value("category").toString();
    insertText = json.value("insertText").toString();
    filterText = json.value("filterText").toString();
    sortText = json.value("sortText").toString();
    
    // Set defaults if not provided
    if (insertText.isEmpty()) insertText = label;
    if (filterText.isEmpty()) filterText = label;
    if (sortText.isEmpty()) sortText = label;
}

QString ArabicCompletionItem::getDisplayText() const {
    return arabicName.isEmpty() ? label : arabicName;
}

QString ArabicCompletionItem::getDetailText() const {
    QString detail = arabicDescription;
    if (!returnType.isEmpty()) {
        detail += " → " + returnType;
    }
    return detail;
}

QString ArabicCompletionItem::getTypeText() const {
    switch (kind) {
        case 3: return "دالة";      // Function
        case 14: return "كلمة مفتاحية"; // Keyword
        case 6: return "متغير";      // Variable
        case 7: return "فئة";        // Class
        case 15: return "قالب";      // Snippet
        case 21: return "ثابت";      // Constant
        default: return "عنصر";      // Generic item
    }
}

bool ArabicCompletionItem::isApplicableInContext(const QString& context) const {
    if (contexts.isEmpty()) return true;
    return contexts.contains(context);
}

// ArabicCompletionWidget implementation
ArabicCompletionWidget::ArabicCompletionWidget(QWidget* parent)
    : QWidget(parent)
    , m_maxVisibleItems(10)
    , m_richDescriptionsEnabled(true) {
    
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    setupFonts();
    setupUI();
    setupStyling();
    setupConnections();
    setupRTLSupport();
    
    // Start hidden
    hide();
}

ArabicCompletionWidget::~ArabicCompletionWidget() = default;

void ArabicCompletionWidget::setupFonts() {
    // Arabic font for completion items
    m_arabicFont = QFont("Noto Sans Arabic", 12);
    if (!QFontDatabase::families().contains("Noto Sans Arabic")) {
        m_arabicFont = QFont("Tahoma", 12);
    }
    m_arabicFont.setWeight(QFont::Normal);
    
    // Code font for examples
    m_codeFont = QFont("Consolas", 10);
    if (!QFontDatabase::families().contains("Consolas")) {
        m_codeFont = QFont("Courier New", 10);
    }
    
    // Type font for kind labels
    m_typeFont = QFont(m_arabicFont);
    m_typeFont.setPointSize(9);
    m_typeFont.setWeight(QFont::Bold);
}

void ArabicCompletionWidget::setupUI() {
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(0);

    // Main splitter
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_mainSplitter);

    // Left panel - completion list
    m_listPanel = new QWidget();
    m_listPanel->setMinimumWidth(280);
    m_listPanel->setMaximumWidth(350);

    QVBoxLayout* listLayout = new QVBoxLayout(m_listPanel);
    listLayout->setContentsMargins(6, 6, 6, 6);
    listLayout->setSpacing(6);
    
    // Completion list
    m_listWidget = new QListWidget();
    m_listWidget->setFont(m_arabicFont);
    m_listWidget->setAlternatingRowColors(true);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listLayout->addWidget(m_listWidget);
    
    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setFont(QFont(m_arabicFont.family(), 9));
    m_statusLabel->setStyleSheet("color: #888; padding: 4px;");
    listLayout->addWidget(m_statusLabel);
    
    m_mainSplitter->addWidget(m_listPanel);
    
    // Right panel - details
    m_detailsPanel = new QWidget();
    m_detailsPanel->setMinimumWidth(320);
    m_detailsPanel->setMaximumWidth(450);

    QVBoxLayout* detailsLayout = new QVBoxLayout(m_detailsPanel);
    detailsLayout->setContentsMargins(10, 10, 10, 10);
    detailsLayout->setSpacing(10);
    
    // Title and type
    QHBoxLayout* titleLayout = new QHBoxLayout();
    
    m_titleLabel = new QLabel();
    m_titleLabel->setFont(QFont(m_arabicFont.family(), 14, QFont::Bold));
    m_titleLabel->setStyleSheet("color: #2196F3;");
    titleLayout->addWidget(m_titleLabel);
    
    titleLayout->addStretch();
    
    m_typeLabel = new QLabel();
    m_typeLabel->setFont(m_typeFont);
    m_typeLabel->setStyleSheet("background: #4CAF50; color: white; padding: 2px 8px; border-radius: 10px;");
    titleLayout->addWidget(m_typeLabel);
    
    detailsLayout->addLayout(titleLayout);
    
    // Description
    QLabel* descLabel = new QLabel("الوصف:");
    descLabel->setFont(QFont(m_arabicFont.family(), 11, QFont::Bold));
    descLabel->setStyleSheet("color: #4CAF50; margin-bottom: 4px;");
    detailsLayout->addWidget(descLabel);

    m_descriptionText = new QTextEdit();
    m_descriptionText->setFont(m_arabicFont);
    m_descriptionText->setMaximumHeight(80);
    m_descriptionText->setMinimumHeight(60);
    m_descriptionText->setReadOnly(true);
    detailsLayout->addWidget(m_descriptionText);

    // Example
    QLabel* exampleLabel = new QLabel("مثال:");
    exampleLabel->setFont(QFont(m_arabicFont.family(), 11, QFont::Bold));
    exampleLabel->setStyleSheet("color: #FF9800; margin-bottom: 4px;");
    detailsLayout->addWidget(exampleLabel);

    m_exampleText = new QTextEdit();
    m_exampleText->setFont(m_codeFont);
    m_exampleText->setMaximumHeight(100);
    m_exampleText->setMinimumHeight(70);
    m_exampleText->setReadOnly(true);
    detailsLayout->addWidget(m_exampleText);
    
    // Insert example button
    m_insertExampleButton = new QPushButton("إدراج المثال");
    m_insertExampleButton->setFont(m_arabicFont);
    detailsLayout->addWidget(m_insertExampleButton);
    
    detailsLayout->addStretch();
    
    m_mainSplitter->addWidget(m_detailsPanel);
    
    // Set splitter proportions (list panel smaller, details panel larger)
    m_mainSplitter->setSizes({280, 380});

    // Set optimal size
    setMinimumSize(600, 280);
    resize(660, 320);
}

void ArabicCompletionWidget::setupStyling() {
    setStyleSheet(R"(
        QWidget#ArabicCompletionWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #2d3142, stop:1 #1a1d29);
            border: 2px solid #4a90e2;
            border-radius: 12px;
        }
        
        QListWidget {
            background: #252836;
            border: 1px solid #3a3d4a;
            border-radius: 8px;
            color: #ffffff;
            selection-background-color: #4a90e2;
            selection-color: #ffffff;
            outline: none;
        }
        
        QListWidget::item {
            padding: 8px 12px;
            border-bottom: 1px solid #3a3d4a;
            min-height: 32px;
        }
        
        QListWidget::item:hover {
            background: rgba(74, 144, 226, 0.3);
        }
        
        QListWidget::item:selected {
            background: #4a90e2;
            color: #ffffff;
        }
        
        QTextEdit {
            background: #252836;
            border: 1px solid #3a3d4a;
            border-radius: 8px;
            padding: 8px;
            color: #e0e0e0;
        }
        
        QPushButton {
            background: #4a90e2;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 6px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background: #357abd;
        }
        
        QPushButton:pressed {
            background: #2968a3;
        }
    )");
}

void ArabicCompletionWidget::setupConnections() {
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &ArabicCompletionWidget::onItemSelectionChanged);
    
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &ArabicCompletionWidget::onItemDoubleClicked);
    
    connect(m_insertExampleButton, &QPushButton::clicked,
            this, &ArabicCompletionWidget::onInsertExampleClicked);
}

void ArabicCompletionWidget::setupRTLSupport() {
    // Enable RTL support for Arabic text
    m_listWidget->setLayoutDirection(Qt::RightToLeft);
    m_descriptionText->setLayoutDirection(Qt::RightToLeft);
    m_titleLabel->setLayoutDirection(Qt::RightToLeft);
    
    // Set text direction for Arabic content
    m_descriptionText->document()->setDefaultTextOption(QTextOption(Qt::AlignRight));
}

void ArabicCompletionWidget::showCompletions(const QList<ArabicCompletionItem>& items, const QString& filter) {
    m_allItems = items;
    m_currentFilter = filter;

    filterItems(filter);
    populateList();

    if (!m_filteredItems.isEmpty()) {
        m_listWidget->setCurrentRow(0);
        updateDetailsPanel(m_filteredItems.first());

        // Update status
        m_statusLabel->setText(QString("%1 من %2 عنصر")
                              .arg(m_filteredItems.size())
                              .arg(m_allItems.size()));

        show();
        m_listWidget->setFocus();
    } else {
        hide();
    }
}

void ArabicCompletionWidget::updateFilter(const QString& filter) {
    if (m_currentFilter == filter) return;

    m_currentFilter = filter;
    filterItems(filter);
    populateList();

    if (!m_filteredItems.isEmpty()) {
        m_listWidget->setCurrentRow(0);
        updateDetailsPanel(m_filteredItems.first());

        m_statusLabel->setText(QString("%1 من %2 عنصر")
                              .arg(m_filteredItems.size())
                              .arg(m_allItems.size()));
    } else {
        clearDetailsPanel();
        m_statusLabel->setText("لا توجد نتائج");
    }
}

void ArabicCompletionWidget::hide() {
    QWidget::hide();
    clearDetailsPanel();
}

void ArabicCompletionWidget::selectNext() {
    int currentRow = m_listWidget->currentRow();
    int nextRow = (currentRow + 1) % m_listWidget->count();
    m_listWidget->setCurrentRow(nextRow);
}

void ArabicCompletionWidget::selectPrevious() {
    int currentRow = m_listWidget->currentRow();
    int prevRow = (currentRow - 1 + m_listWidget->count()) % m_listWidget->count();
    m_listWidget->setCurrentRow(prevRow);
}

void ArabicCompletionWidget::selectFirst() {
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void ArabicCompletionWidget::selectLast() {
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(m_listWidget->count() - 1);
    }
}

ArabicCompletionItem ArabicCompletionWidget::getSelectedItem() const {
    int currentRow = m_listWidget->currentRow();
    if (currentRow >= 0 && currentRow < m_filteredItems.size()) {
        return m_filteredItems[currentRow];
    }
    return ArabicCompletionItem();
}

bool ArabicCompletionWidget::hasSelection() const {
    return m_listWidget->currentRow() >= 0;
}

void ArabicCompletionWidget::setMaxVisibleItems(int count) {
    m_maxVisibleItems = count;
    // Update list widget height
    int itemHeight = 40; // Approximate item height
    int maxHeight = m_maxVisibleItems * itemHeight + 20; // Add some padding
    m_listWidget->setMaximumHeight(maxHeight);
}

void ArabicCompletionWidget::setMinimumWidth(int width) {
    QWidget::setMinimumWidth(width);
}

void ArabicCompletionWidget::enableRichDescriptions(bool enabled) {
    m_richDescriptionsEnabled = enabled;
    m_detailsPanel->setVisible(enabled);
}

void ArabicCompletionWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            emit cancelled();
            hide();
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (hasSelection()) {
                emit itemActivated(getSelectedItem());
            }
            break;

        case Qt::Key_Up:
            selectPrevious();
            break;

        case Qt::Key_Down:
            selectNext();
            break;

        case Qt::Key_Home:
            selectFirst();
            break;

        case Qt::Key_End:
            selectLast();
            break;

        default:
            // Pass other keys to parent
            QWidget::keyPressEvent(event);
            break;
    }
}

void ArabicCompletionWidget::focusOutEvent(QFocusEvent* event) {
    // Hide completion when focus is lost
    QWidget::focusOutEvent(event);
    hide();
}

bool ArabicCompletionWidget::eventFilter(QObject* obj, QEvent* event) {
    // Handle events from other widgets if needed
    return QWidget::eventFilter(obj, event);
}

void ArabicCompletionWidget::onItemSelectionChanged() {
    if (hasSelection()) {
        ArabicCompletionItem item = getSelectedItem();
        updateDetailsPanel(item);
        emit itemSelected(item);
    }
}

void ArabicCompletionWidget::onItemDoubleClicked(QListWidgetItem* listItem) {
    Q_UNUSED(listItem)
    if (hasSelection()) {
        emit itemActivated(getSelectedItem());
    }
}

void ArabicCompletionWidget::onInsertExampleClicked() {
    if (hasSelection()) {
        ArabicCompletionItem item = getSelectedItem();

        // Create a special signal to indicate example insertion
        emit exampleInsertRequested(item);
    }
}

void ArabicCompletionWidget::populateList() {
    m_listWidget->clear();

    for (const auto& item : m_filteredItems) {
        ArabicCompletionListItem* listItem = new ArabicCompletionListItem(item, m_listWidget);
        updateItemDisplay(listItem, item);
    }

    // Limit visible items
    if (m_listWidget->count() > m_maxVisibleItems) {
        int itemHeight = m_listWidget->sizeHintForRow(0);
        m_listWidget->setMaximumHeight(m_maxVisibleItems * itemHeight + 10);
    }
}

void ArabicCompletionWidget::filterItems(const QString& filter) {
    m_filteredItems.clear();

    if (filter.isEmpty()) {
        m_filteredItems = m_allItems;
    } else {
        // Score and filter items
        QList<QPair<int, ArabicCompletionItem>> scoredItems;

        for (const auto& item : m_allItems) {
            int score = calculateMatchScore(filter, item.getDisplayText());
            if (score > 0) {
                scoredItems.append({score, item});
            }
        }

        // Sort by score (descending) and priority
        std::sort(scoredItems.begin(), scoredItems.end(),
                 [](const QPair<int, ArabicCompletionItem>& a, const QPair<int, ArabicCompletionItem>& b) {
                     if (a.first != b.first) return a.first > b.first; // Higher score first
                     return a.second.priority > b.second.priority;     // Higher priority first
                 });

        // Extract filtered items
        for (const auto& pair : scoredItems) {
            m_filteredItems.append(pair.second);
        }
    }
}

void ArabicCompletionWidget::updateItemDisplay(QListWidgetItem* listItem, const ArabicCompletionItem& item) {
    // Set main text
    listItem->setText(item.getDisplayText());
    listItem->setFont(m_arabicFont);

    // Set tooltip with detailed information
    QString tooltip = QString("<div dir='rtl'><b>%1</b><br/>%2</div>")
                     .arg(item.getDisplayText())
                     .arg(item.arabicDescription);
    listItem->setToolTip(tooltip);

    // Set icon based on kind
    QColor kindColor = getKindColor(item.kind);
    QPixmap pixmap(16, 16);
    pixmap.fill(kindColor);
    listItem->setIcon(QIcon(pixmap));
}

void ArabicCompletionWidget::updateDetailsPanel(const ArabicCompletionItem& item) {
    if (!m_richDescriptionsEnabled) return;

    // Update title and type
    m_titleLabel->setText(item.getDisplayText());
    m_typeLabel->setText(item.getTypeText());

    // Update type label color based on kind
    QColor kindColor = getKindColor(item.kind);
    m_typeLabel->setStyleSheet(QString("background: %1; color: white; padding: 2px 8px; border-radius: 10px;")
                              .arg(kindColor.name()));

    // Update description
    QString description = item.arabicDetailedDesc.isEmpty() ? item.arabicDescription : item.arabicDetailedDesc;
    m_descriptionText->setHtml(formatArabicDescription(description));

    // Update example
    QString example = item.arabicExample.isEmpty() ? item.usageExample : item.arabicExample;
    m_exampleText->setPlainText(formatCodeExample(example));

    // Show/hide insert button based on example availability
    m_insertExampleButton->setVisible(!example.isEmpty());
}

void ArabicCompletionWidget::clearDetailsPanel() {
    m_titleLabel->clear();
    m_typeLabel->clear();
    m_descriptionText->clear();
    m_exampleText->clear();
    m_insertExampleButton->setVisible(false);
}

QString ArabicCompletionWidget::formatArabicDescription(const QString& desc) {
    return QString("<div dir='rtl' style='font-family: %1; font-size: 12px; line-height: 1.4;'>%2</div>")
           .arg(m_arabicFont.family())
           .arg(desc);
}

QString ArabicCompletionWidget::formatCodeExample(const QString& example) {
    // Remove any existing formatting and return clean code
    return example;
}

QString ArabicCompletionWidget::getKindText(int kind) {
    switch (kind) {
        case 3: return "دالة";      // Function
        case 14: return "كلمة مفتاحية"; // Keyword
        case 6: return "متغير";      // Variable
        case 7: return "فئة";        // Class
        case 15: return "قالب";      // Snippet
        case 21: return "ثابت";      // Constant
        default: return "عنصر";      // Generic item
    }
}

QColor ArabicCompletionWidget::getKindColor(int kind) {
    switch (kind) {
        case 3: return QColor("#4CAF50");  // Function - Green
        case 14: return QColor("#FF9800"); // Keyword - Orange
        case 6: return QColor("#2196F3");  // Variable - Blue
        case 7: return QColor("#9C27B0");  // Class - Purple
        case 15: return QColor("#F44336"); // Snippet - Red
        case 21: return QColor("#607D8B"); // Constant - Blue Grey
        default: return QColor("#757575"); // Generic - Grey
    }
}

int ArabicCompletionWidget::calculateMatchScore(const QString& filter, const QString& text) {
    if (filter.isEmpty()) return 100;
    if (text.isEmpty()) return 0;

    QString lowerFilter = filter.toLower();
    QString lowerText = text.toLower();

    // Exact match gets highest score
    if (lowerText == lowerFilter) return 100;

    // Starts with gets high score
    if (lowerText.startsWith(lowerFilter)) return 90;

    // Contains gets medium score
    if (lowerText.contains(lowerFilter)) return 70;

    // Fuzzy match gets lower score
    int fuzzyScore = 0;
    int filterIndex = 0;
    for (int i = 0; i < lowerText.length() && filterIndex < lowerFilter.length(); ++i) {
        if (lowerText[i] == lowerFilter[filterIndex]) {
            fuzzyScore += 10;
            filterIndex++;
        }
    }

    // Return fuzzy score only if all filter characters were found
    return (filterIndex == lowerFilter.length()) ? fuzzyScore : 0;
}

QList<int> ArabicCompletionWidget::getHighlightPositions(const QString& filter, const QString& text) {
    QList<int> positions;
    if (filter.isEmpty() || text.isEmpty()) return positions;

    QString lowerFilter = filter.toLower();
    QString lowerText = text.toLower();

    int filterIndex = 0;
    for (int i = 0; i < lowerText.length() && filterIndex < lowerFilter.length(); ++i) {
        if (lowerText[i] == lowerFilter[filterIndex]) {
            positions.append(i);
            filterIndex++;
        }
    }

    return positions;
}

// ArabicCompletionListItem implementation
ArabicCompletionListItem::ArabicCompletionListItem(const ArabicCompletionItem& item, QListWidget* parent)
    : QListWidgetItem(parent), m_item(item) {
    setText(item.getDisplayText());
}
