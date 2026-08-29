#include "QalamExplorerView.h"
#include "QalamSymbolOutlineView.h"
#include "../ui/QalamTheme.h"
#include "Constants.h"
#include <QScrollArea>
#include <QDir>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QMenu>
#include <QMouseEvent>
#include <QStyle>

namespace {

class QalamFileIconProvider final : public QFileIconProvider {
public:
    QIcon icon(const QFileInfo &info) const override
    {
        if (info.isDir()) {
            return QIcon(QStringLiteral(":/icons/resources/folder.svg"));
        }

        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("باء") or
            suffix == QStringLiteral("رأسباء") or
            suffix == QStringLiteral("baa") or
            suffix == QStringLiteral("baahd") or
            suffix == QStringLiteral("نظم")) {
            return QIcon(QStringLiteral(":/icons/resources/file-new.svg"));
        }
        return QFileIconProvider::icon(info);
    }
};

}

QalamExplorerView::QalamExplorerView(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    applyStyles();
}

void QalamExplorerView::setupUi()
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
    
    // One shared filesystem model backs one tree per workspace root.  Separate
    // trees keep unrelated folders as first-class roots without exposing their
    // common drive or filesystem ancestors.
    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setIconProvider(new QalamFileIconProvider());
    m_fileSystemModel->setRootPath("");
    m_fileSystemModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    
    m_rootsContainer = new QWidget(this);
    m_rootsContainer->setObjectName(QStringLiteral("workspaceRootsContainer"));
    m_rootsLayout = new QVBoxLayout(m_rootsContainer);
    m_rootsLayout->setContentsMargins(0, 0, 0, 0);
    m_rootsLayout->setSpacing(0);
    
    m_mainLayout->addWidget(m_folderHeader);
    m_mainLayout->addWidget(m_rootsContainer, 2);
    
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
    openFolderBtn->setIcon(QIcon(QStringLiteral(":/icons/resources/folder-open.svg")));
    openFolderBtn->setIconSize(QSize(18, 18));
    openFolderBtn->setCursor(Qt::PointingHandCursor);
    connect(openFolderBtn, &QPushButton::clicked, this, &QalamExplorerView::openFolderRequested);
    
    noFolderLayout->addWidget(noFolderLabel);
    noFolderLayout->addSpacing(10);
    noFolderLayout->addWidget(openFolderBtn);
    noFolderLayout->addStretch();
    
    m_mainLayout->addWidget(m_noFolderWidget);

    // ========== Current document outline ==========
    m_outlineHeader = createSectionHeader(
        QStringLiteral("المخطط"), true);
    m_outlineView = new QalamSymbolOutlineView(this);
    m_outlineView->setMinimumHeight(120);
    m_mainLayout->addWidget(m_outlineHeader);
    m_mainLayout->addWidget(m_outlineView, 1);
    connect(
        m_outlineView,
        &QalamSymbolOutlineView::symbolActivated,
        this,
        &QalamExplorerView::outlineSymbolActivated);
    
    // Initially show no folder state
    m_rootsContainer->hide();
    m_noFolderWidget->show();
}

QWidget* QalamExplorerView::createSectionHeader(const QString &title, bool expanded)
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

void QalamExplorerView::setRootPath(const QString &path)
{
    setRootPaths(path.isEmpty() ? QStringList{} : QStringList{path});
}

QTreeView *QalamExplorerView::createRootTree(const QString &rootPath)
{
    auto *treeView = new QTreeView(m_rootsContainer);
    treeView->setProperty("workspaceRoot", rootPath);
    treeView->setModel(m_fileSystemModel);
    treeView->setRootIndex(m_fileSystemModel->index(rootPath));
    treeView->setHeaderHidden(true);
    treeView->setAnimated(true);
    treeView->setIndentation(12);
    treeView->setExpandsOnDoubleClick(true);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    for (int column = 1; column <= 3; ++column) treeView->hideColumn(column);

    connect(treeView, &QTreeView::doubleClicked, this,
            [this](const QModelIndex &index) {
        const QString filePath = m_fileSystemModel->filePath(index);
        if (QFileInfo(filePath).isFile()) emit fileDoubleClicked(filePath);
    });
    connect(treeView, &QTreeView::customContextMenuRequested, this,
            [this, treeView, rootPath](const QPoint &position) {
        showTreeContextMenu(treeView, rootPath, position);
    });
    return treeView;
}

void QalamExplorerView::clearRootTrees()
{
    while (QLayoutItem *item = m_rootsLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_treeViews.clear();
}

void QalamExplorerView::setRootPaths(const QStringList &paths)
{
    QStringList roots;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (not info.isDir()) continue;
        const QString canonical = info.canonicalFilePath();
        const QString clean = QDir::cleanPath(
            canonical.isEmpty() ? info.absoluteFilePath() : canonical);
        if (not roots.contains(clean, Qt::CaseInsensitive)) roots.push_back(clean);
    }
    if (m_rootPaths == roots) return;
    m_rootPaths = roots;
    clearRootTrees();

    if (m_rootPaths.isEmpty()) {
        m_rootsContainer->hide();
        m_noFolderWidget->show();
        if (m_folderNameLabel) {
            m_folderNameLabel->setText(Constants::NoFolderOpenLabel.toUpper());
        }
    } else {
        m_fileSystemModel->setRootPath(QString());
        const bool showRootLabels = m_rootPaths.size() > 1;
        for (const QString &rootPath : m_rootPaths) {
            if (showRootLabels) {
                auto *label = new QLabel(QFileInfo(rootPath).fileName().toUpper(),
                                         m_rootsContainer);
                label->setObjectName(QStringLiteral("workspaceRootLabel"));
                label->setToolTip(QDir::toNativeSeparators(rootPath));
                label->setProperty("workspaceRoot", rootPath);
                m_rootsLayout->addWidget(label);
            }
            QTreeView *treeView = createRootTree(rootPath);
            m_treeViews.push_back(treeView);
            m_rootsLayout->addWidget(treeView, 1);
        }
        m_rootsContainer->setVisible(m_folderExpanded);
        m_noFolderWidget->hide();
        if (m_folderNameLabel) {
            const QString title = m_rootPaths.size() == 1
                ? QFileInfo(m_rootPaths.constFirst()).fileName().toUpper()
                : QStringLiteral("مساحة العمل (%1)").arg(m_rootPaths.size());
            m_folderNameLabel->setText(title);
        }
    }
}

void QalamExplorerView::addOpenEditor(const QString &filePath, bool modified)
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
    item->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(closeBtn, &QPushButton::clicked, this, [this, item]() {
        const QString path = item->property("filePath").toString();
        if (!path.isEmpty()) {
            emit openEditorCloseRequested(path);
        }
    });
    connect(item, &QWidget::customContextMenuRequested, this,
            [this, item](const QPoint &position) {
        showOpenEditorContextMenu(item, position);
    });

    m_openEditorsLayout->addWidget(item);
}

void QalamExplorerView::removeOpenEditor(const QString &filePath)
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

void QalamExplorerView::updateOpenEditor(const QString &filePath, bool modified)
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

void QalamExplorerView::clearOpenEditors()
{
    QLayoutItem *item;
    while ((item = m_openEditorsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void QalamExplorerView::setOutlineSymbols(
    const QString &filePath,
    const QVector<BaaDocumentSymbol> &symbols)
{
    if (m_outlineView) m_outlineView->setSymbols(filePath, symbols);
}

void QalamExplorerView::clearOutlineSymbols()
{
    if (m_outlineView) m_outlineView->clearSymbols();
}

void QalamExplorerView::showTreeContextMenu(
    QTreeView *treeView,
    const QString &rootPath,
    const QPoint &position)
{
    if (not treeView or rootPath.isEmpty()) return;

    const QModelIndex index = treeView->indexAt(position);
    if (index.isValid()) treeView->setCurrentIndex(index);

    const QString selectedPath = index.isValid()
        ? m_fileSystemModel->filePath(index)
        : rootPath;
    const QFileInfo selectedInfo(selectedPath);
    const QString directoryPath = selectedInfo.isDir()
        ? selectedInfo.absoluteFilePath()
        : selectedInfo.absolutePath();

    QMenu menu(this);
    menu.setLayoutDirection(Qt::RightToLeft);
    QAction *newFileAction = menu.addAction(
        QIcon(QStringLiteral(":/icons/resources/file-new.svg")),
        QStringLiteral("ملف باء جديد"));
    QAction *newFolderAction = menu.addAction(
        QIcon(QStringLiteral(":/icons/resources/folder.svg")),
        QStringLiteral("مجلد جديد"));

    QAction *renameAction{};
    QAction *deleteAction{};
    if (index.isValid()) {
        menu.addSeparator();
        renameAction = menu.addAction(QStringLiteral("إعادة التسمية"));
        deleteAction = menu.addAction(
            style()->standardIcon(QStyle::SP_TrashIcon),
            QStringLiteral("حذف"));
    }
    menu.addSeparator();
    QAction *removeRootAction = menu.addAction(
        QStringLiteral("إزالة المجلد من مساحة العمل"));

    QAction *chosen = menu.exec(treeView->viewport()->mapToGlobal(position));
    if (chosen == newFileAction) {
        emit createFileRequested(directoryPath);
    } else if (chosen == newFolderAction) {
        emit createFolderRequested(directoryPath);
    } else if (chosen == renameAction) {
        emit renameEntryRequested(selectedInfo.absoluteFilePath());
    } else if (chosen == deleteAction) {
        emit deleteEntryRequested(selectedInfo.absoluteFilePath());
    } else if (chosen == removeRootAction) {
        emit removeRootRequested(rootPath);
    }
}

void QalamExplorerView::showOpenEditorContextMenu(
    QWidget *item,
    const QPoint &position)
{
    if (not item) return;
    const QString path = item->property("filePath").toString();
    if (path.isEmpty()) return;

    QMenu menu(this);
    menu.setLayoutDirection(Qt::RightToLeft);
    QAction *closeAction = menu.addAction(QStringLiteral("إغلاق"));
    QAction *closeOthersAction = menu.addAction(QStringLiteral("إغلاق البقية"));
    QAction *closeAllAction = menu.addAction(QStringLiteral("إغلاق الكل"));

    QAction *chosen = menu.exec(item->mapToGlobal(position));
    if (chosen == closeAction) {
        emit openEditorCloseRequested(path);
    } else if (chosen == closeOthersAction) {
        emit closeOtherEditorsRequested(path);
    } else if (chosen == closeAllAction) {
        emit closeAllEditorsRequested();
    }
}

bool QalamExplorerView::eventFilter(QObject *watched, QEvent *event)
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
            m_rootsContainer->setVisible(
                m_folderExpanded && not m_rootPaths.isEmpty());
            m_noFolderWidget->setVisible(
                m_folderExpanded && m_rootPaths.isEmpty());
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

void QalamExplorerView::applyStyles()
{
    using namespace Constants;
    
    // Use centralized theme with additional explorer-specific styles
    QString styles = QString(R"(
        QalamExplorerView {
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

        #workspaceRootLabel {
            color: %4;
            background-color: %2;
            border-top: 1px solid %17;
            padding: 5px 10px;
            font-size: %5px;
            font-weight: 600;
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
    .arg(Colors::ScrollbarThumbHover)            // %16
    .arg(Colors::BorderSubtle);                  // %17
    
    setStyleSheet(styles);
}
