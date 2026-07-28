#include "TExplorerView.h"
#include "TSymbolOutlineView.h"
#include "../ui/QalamTheme.h"
#include "Constants.h"
#include <QScrollArea>
#include <QFileInfo>
#include <QMouseEvent>

TExplorerView::TExplorerView(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    applyStyles();
}

void TExplorerView::setupUi()
{
    using namespace Constants;
    
    setLayoutDirection(Qt::RightToLeft);  // RTL for Arabic
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // ========== Open Editors Section ==========
    m_openEditorsHeader = createSectionHeader(OpenEditorsLabel, true);
    
    m_openEditorsContent = new QWidget();
    m_openEditorsLayout = new QVBoxLayout(m_openEditorsContent);
    m_openEditorsLayout->setContentsMargins(0, 0, 0, 0);
    m_openEditorsLayout->setSpacing(0);
    
    m_mainLayout->addWidget(m_openEditorsHeader);
    m_mainLayout->addWidget(m_openEditorsContent);
    
    // ========== Folder Section ==========
    m_folderHeader = createSectionHeader(NoFolderOpenLabel, true);
    m_folderNameLabel = m_folderHeader->findChild<QLabel*>("sectionTitle");
    
    // Tree view for folder contents
    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setRootPath("");
    m_fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    
    m_treeView = new QTreeView();
    m_treeView->setModel(m_fileSystemModel);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(12);
    m_treeView->setExpandsOnDoubleClick(true);
    
    // Hide size, type, date columns
    m_treeView->hideColumn(1);
    m_treeView->hideColumn(2);
    m_treeView->hideColumn(3);
    
    // Connect double-click
    connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        QString filePath = m_fileSystemModel->filePath(index);
        QFileInfo info(filePath);
        if (info.isFile()) {
            emit fileDoubleClicked(filePath);
        }
    });
    
    m_mainLayout->addWidget(m_folderHeader);
    m_mainLayout->addWidget(m_treeView, 2);
    
    // ========== No Folder Open State ==========
    m_noFolderWidget = new QWidget();
    QVBoxLayout *noFolderLayout = new QVBoxLayout(m_noFolderWidget);
    noFolderLayout->setContentsMargins(15, 20, 15, 20);
    noFolderLayout->setAlignment(Qt::AlignTop);
    
    QLabel *noFolderLabel = new QLabel(NoFolderOpenLabel);
    noFolderLabel->setObjectName("noFolderLabel");
    noFolderLabel->setWordWrap(true);
    noFolderLabel->setAlignment(Qt::AlignCenter);
    
    QPushButton *openFolderBtn = new QPushButton("فتح مجلد");
    openFolderBtn->setObjectName("openFolderBtn");
    openFolderBtn->setCursor(Qt::PointingHandCursor);
    connect(openFolderBtn, &QPushButton::clicked, this, &TExplorerView::openFolderRequested);
    
    noFolderLayout->addWidget(noFolderLabel);
    noFolderLayout->addSpacing(10);
    noFolderLayout->addWidget(openFolderBtn);
    noFolderLayout->addStretch();
    
    m_mainLayout->addWidget(m_noFolderWidget);

    // ========== Current document outline ==========
    m_outlineHeader = createSectionHeader(
        QStringLiteral("المخطط"), true);
    m_outlineView = new TSymbolOutlineView(this);
    m_outlineView->setMinimumHeight(120);
    m_mainLayout->addWidget(m_outlineHeader);
    m_mainLayout->addWidget(m_outlineView, 1);
    connect(
        m_outlineView,
        &TSymbolOutlineView::symbolActivated,
        this,
        &TExplorerView::outlineSymbolActivated);
    
    // Initially show no folder state
    m_treeView->hide();
    m_noFolderWidget->show();
}

QWidget* TExplorerView::createSectionHeader(const QString &title, bool expanded)
{
    using namespace Constants;
    
    QWidget *header = new QWidget();
    header->setObjectName("sectionHeader");
    header->setFixedHeight(Layout::SidebarSectionHeaderHeight);
    header->setCursor(Qt::PointingHandCursor);
    
    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(5, 0, 10, 0);
    layout->setSpacing(4);
    
    // Expand/collapse arrow
    QLabel *arrow = new QLabel(expanded ? "▾" : "▸");
    arrow->setObjectName("sectionArrow");
    arrow->setFixedWidth(12);
    
    // Section title
    QLabel *titleLabel = new QLabel(title.toUpper());
    titleLabel->setObjectName("sectionTitle");
    
    layout->addWidget(arrow);
    layout->addWidget(titleLabel);
    layout->addStretch();
    
    // Toggle on click
    header->setProperty("expanded", expanded);
    header->installEventFilter(this);
    
    return header;
}

void TExplorerView::setRootPath(const QString &path)
{
    m_rootPath = path;
    
    if (path.isEmpty()) {
        m_treeView->hide();
        m_noFolderWidget->show();
        if (m_folderNameLabel) {
            m_folderNameLabel->setText(Constants::NoFolderOpenLabel.toUpper());
        }
    } else {
        QFileInfo info(path);
        m_fileSystemModel->setRootPath(path);
        m_treeView->setRootIndex(m_fileSystemModel->index(path));
        m_treeView->show();
        m_noFolderWidget->hide();
        
        if (m_folderNameLabel) {
            m_folderNameLabel->setText(info.fileName().toUpper());
        }
    }
}

void TExplorerView::addOpenEditor(const QString &filePath, bool modified)
{
    QFileInfo info(filePath);
    
    QWidget *item = new QWidget();
    item->setObjectName("openEditorItem");
    item->setProperty("filePath", filePath);
    item->setCursor(Qt::PointingHandCursor);
    
    QHBoxLayout *layout = new QHBoxLayout(item);
    layout->setContentsMargins(20, 4, 8, 4);
    layout->setSpacing(6);
    
    // File icon
    QLabel *icon = new QLabel();
    icon->setPixmap(QIcon(":/icons/resources/file-new.svg").pixmap(14, 14));
    icon->setFixedSize(14, 14);
    
    // File name
    QString displayName = info.fileName();
    if (modified) displayName += " •";
    
    QLabel *nameLabel = new QLabel(displayName);
    nameLabel->setObjectName("openEditorName");
    
    // Close button
    QPushButton *closeBtn = new QPushButton();
    closeBtn->setObjectName("openEditorClose");
    closeBtn->setIcon(QIcon(":/icons/resources/close.svg"));
    closeBtn->setIconSize(QSize(10, 10));
    closeBtn->setFixedSize(16, 16);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->hide();  // Show on hover
    
    layout->addWidget(icon);
    layout->addWidget(nameLabel, 1);
    layout->addWidget(closeBtn);
    
    // Click to switch to file
    item->installEventFilter(this);

    connect(closeBtn, &QPushButton::clicked, this, [this, item]() {
        const QString path = item->property("filePath").toString();
        if (!path.isEmpty()) {
            emit openEditorCloseRequested(path);
        }
    });

    m_openEditorsLayout->addWidget(item);
}

void TExplorerView::removeOpenEditor(const QString &filePath)
{
    for (int i = 0; i < m_openEditorsLayout->count(); ++i) {
        QWidget *item = m_openEditorsLayout->itemAt(i)->widget();
        if (item && item->property("filePath").toString() == filePath) {
            m_openEditorsLayout->removeWidget(item);
            item->deleteLater();
            break;
        }
    }
}

void TExplorerView::updateOpenEditor(const QString &filePath, bool modified)
{
    for (int i = 0; i < m_openEditorsLayout->count(); ++i) {
        QWidget *item = m_openEditorsLayout->itemAt(i)->widget();
        if (item && item->property("filePath").toString() == filePath) {
            QLabel *nameLabel = item->findChild<QLabel*>("openEditorName");
            if (nameLabel) {
                QFileInfo info(filePath);
                QString displayName = info.fileName();
                if (modified) displayName += " •";
                nameLabel->setText(displayName);
            }
            break;
        }
    }
}

void TExplorerView::clearOpenEditors()
{
    QLayoutItem *item;
    while ((item = m_openEditorsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void TExplorerView::setOutlineSymbols(
    const QString &filePath,
    const QVector<BaaDocumentSymbol> &symbols)
{
    if (m_outlineView) m_outlineView->setSymbols(filePath, symbols);
}

void TExplorerView::clearOutlineSymbols()
{
    if (m_outlineView) m_outlineView->clearSymbols();
}

bool TExplorerView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        if (auto *widget = qobject_cast<QWidget*>(watched)) {
            if (!widget->property("filePath").toString().isEmpty()) {
                if (auto *button = widget->findChild<QPushButton*>("openEditorClose")) {
                    button->show();
                }
            }
        }
    } else if (event->type() == QEvent::Leave) {
        if (auto *widget = qobject_cast<QWidget*>(watched)) {
            if (auto *button = widget->findChild<QPushButton*>("openEditorClose")) {
                button->hide();
            }
        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        if (watched == m_openEditorsHeader) {
            m_openEditorsExpanded = !m_openEditorsExpanded;
            m_openEditorsContent->setVisible(m_openEditorsExpanded);
            if (auto *arrow = m_openEditorsHeader->findChild<QLabel*>("sectionArrow")) {
                arrow->setText(m_openEditorsExpanded ? "▾" : "▸");
            }
            return true;
        }

        if (watched == m_folderHeader) {
            m_folderExpanded = !m_folderExpanded;
            m_treeView->setVisible(m_folderExpanded && !m_rootPath.isEmpty());
            m_noFolderWidget->setVisible(m_folderExpanded && m_rootPath.isEmpty());
            if (auto *arrow = m_folderHeader->findChild<QLabel*>("sectionArrow")) {
                arrow->setText(m_folderExpanded ? "▾" : "▸");
            }
            return true;
        }

        if (watched == m_outlineHeader) {
            m_outlineExpanded = !m_outlineExpanded;
            m_outlineView->setVisible(m_outlineExpanded);
            if (auto *arrow =
                    m_outlineHeader->findChild<QLabel*>("sectionArrow")) {
                arrow->setText(m_outlineExpanded ? "▾" : "▸");
            }
            return true;
        }

        if (auto *widget = qobject_cast<QWidget*>(watched)) {
            const QString path = widget->property("filePath").toString();
            if (!path.isEmpty() && QFileInfo(path).exists()) {
                emit openEditorClicked(path);
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void TExplorerView::applyStyles()
{
    using namespace Constants;
    
    // Use centralized theme with additional explorer-specific styles
    QString styles = QString(R"(
        TExplorerView {
            background-color: %1;
        }
        
        /* Section headers */
        #sectionHeader {
            background-color: %2;
            border: none;
        }
        
        #sectionHeader:hover {
            background-color: %3;
        }
        
        #sectionTitle {
            color: %4;
            font-size: %5px;
            font-weight: bold;
            font-family: 'Segoe UI', 'Tajawal', sans-serif;
        }
        
        #sectionArrow {
            color: %4;
            font-size: 10px;
        }
        
        /* Tree view */
        QTreeView {
            background-color: %1;
            border: none;
            color: %6;
            font-size: %7px;
            outline: none;
        }
        
        QTreeView::item {
            padding: 3px 0px;
            border-radius: 0px;
        }
        
        QTreeView::item:hover {
            background-color: %3;
        }
        
        QTreeView::item:selected {
            background-color: %8;
        }
        
        QTreeView::branch {
            background: transparent;
        }
        
        /* Open editor items */
        #openEditorItem {
            background-color: transparent;
        }
        
        #openEditorItem:hover {
            background-color: %3;
        }
        
        #openEditorName {
            color: %6;
            font-size: %7px;
        }
        
        #openEditorClose {
            background: transparent;
            border: none;
            border-radius: 3px;
        }
        
        #openEditorClose:hover {
            background-color: %9;
        }
        
        /* No folder state */
        #noFolderLabel {
            color: %10;
            font-size: %7px;
        }
        
        #openFolderBtn {
            background-color: %11;
            color: %12;
            border: none;
            border-radius: 3px;
            padding: 8px 16px;
            font-size: %7px;
        }
        
        #openFolderBtn:hover {
            background-color: %13;
        }
        
        /* Scrollbar */
        QScrollBar:vertical {
            background: transparent;
            width: %14px;
            margin: 0;
        }
        
        QScrollBar::handle:vertical {
            background: %15;
            border-radius: 4px;
            min-height: 30px;
        }
        
        QScrollBar::handle:vertical:hover {
            background: %16;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )")
    .arg(Colors::SidebarBackground)              // %1
    .arg(Colors::SidebarHeaderBackground)        // %2
    .arg(Colors::ListHoverBackground)            // %3
    .arg(Colors::TextSecondary)                  // %4
    .arg(Fonts::SectionHeaderSize)               // %5
    .arg(Colors::TextSecondary)                  // %6
    .arg(Fonts::TreeViewSize)                    // %7
    .arg(Colors::ListActiveBackground)           // %8
    .arg(Colors::ButtonHover)                    // %9
    .arg(Colors::TextMuted)                      // %10
    .arg(Colors::Accent)                         // %11
    .arg(Colors::TextPrimary)                    // %12
    .arg(Colors::AccentHover)                    // %13
    .arg(Layout::ScrollbarWidth)                 // %14
    .arg(Colors::ScrollbarThumb)                 // %15
    .arg(Colors::ScrollbarThumbHover);           // %16
    
    setStyleSheet(styles);
}
