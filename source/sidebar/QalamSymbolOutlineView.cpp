#include "QalamSymbolOutlineView.h"

#include "Constants.h"

#include <QAbstractItemView>
#include <QLabel>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {
constexpr int SymbolLineRole = Qt::UserRole;
constexpr int SymbolColumnRole = Qt::UserRole + 1;
constexpr int SymbolSearchRole = Qt::UserRole + 2;
}

QalamSymbolOutlineView::QalamSymbolOutlineView(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("symbolOutlineView");
    setLayoutDirection(Qt::RightToLeft);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 8);
    layout->setSpacing(6);

    m_filter = new QLineEdit(this);
    m_filter->setObjectName("symbolOutlineFilter");
    m_filter->setPlaceholderText(QStringLiteral("تصفية رموز المستند"));
    m_filter->setClearButtonEnabled(true);
    layout->addWidget(m_filter);

    m_tree = new QTreeWidget(this);
    m_tree->setObjectName("symbolOutlineTree");
    m_tree->setHeaderHidden(true);
    m_tree->setColumnCount(1);
    m_tree->setIndentation(14);
    m_tree->setAnimated(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_tree, 1);

    m_emptyLabel = new QLabel(
        QStringLiteral("لا توجد رموز في المستند الحالي"), this);
    m_emptyLabel->setObjectName("symbolOutlineEmpty");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    layout->addWidget(m_emptyLabel, 1);

    connect(m_filter, &QLineEdit::textChanged,
            this, &QalamSymbolOutlineView::applyFilter);
    connect(m_tree, &QTreeWidget::itemActivated,
            this, [this](QTreeWidgetItem *item) { activateItem(item); });

    setStyleSheet(QString(R"(
        #symbolOutlineView {
            background: %1;
        }
        #symbolOutlineFilter {
            background: %2;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 5px 7px;
        }
        #symbolOutlineFilter:focus {
            border-color: %5;
        }
        #symbolOutlineTree {
            background: transparent;
            color: %3;
            border: none;
            outline: none;
        }
        #symbolOutlineTree::item {
            padding: 3px 2px;
        }
        #symbolOutlineTree::item:hover {
            background: %6;
        }
        #symbolOutlineTree::item:selected {
            background: %7;
        }
        #symbolOutlineEmpty {
            color: %8;
            padding: 12px;
        }
    )")
        .arg(Constants::Colors::SidebarBackground)
        .arg(Constants::Colors::InputBackground)
        .arg(Constants::Colors::TextSecondary)
        .arg(Constants::Colors::Border)
        .arg(Constants::Colors::Accent)
        .arg(Constants::Colors::ListHoverBackground)
        .arg(Constants::Colors::ListActiveBackground)
        .arg(Constants::Colors::TextMuted));

    clearSymbols();
}

void QalamSymbolOutlineView::setSymbols(
    const QString &filePath,
    const QVector<BaaDocumentSymbol> &symbols)
{
    m_filePath = filePath;
    m_tree->clear();
    for (const BaaDocumentSymbol &symbol : symbols)
        appendSymbol(nullptr, symbol);
    applyFilter();
}

void QalamSymbolOutlineView::clearSymbols()
{
    m_filePath.clear();
    m_tree->clear();
    if (m_filter) m_filter->clear();
    m_tree->hide();
    m_emptyLabel->setText(
        QStringLiteral("لا توجد رموز في المستند الحالي"));
    m_emptyLabel->show();
}

void QalamSymbolOutlineView::setFilterText(const QString &text)
{
    m_filter->setText(text);
}

void QalamSymbolOutlineView::appendSymbol(
    QTreeWidgetItem *parent,
    const BaaDocumentSymbol &symbol)
{
    auto *item = parent
        ? new QTreeWidgetItem(parent)
        : new QTreeWidgetItem(m_tree);
    item->setText(0, symbol.name);
    item->setData(0, SymbolLineRole, symbol.line);
    item->setData(0, SymbolColumnRole, symbol.column);
    item->setData(
        0,
        SymbolSearchRole,
        (symbol.name + QLatin1Char(' ') + symbol.detail).toCaseFolded());
    const QString tooltip = symbol.detail.isEmpty()
        ? QStringLiteral("%1 — السطر %2").arg(symbol.name).arg(symbol.line)
        : QStringLiteral("%1\n%2\nالسطر %3")
              .arg(symbol.name, symbol.detail)
              .arg(symbol.line);
    item->setToolTip(0, tooltip);
    for (const BaaDocumentSymbol &child : symbol.children)
        appendSymbol(item, child);
    if (not symbol.children.isEmpty()) item->setExpanded(true);
}

bool QalamSymbolOutlineView::filterItem(
    QTreeWidgetItem *item,
    const QString &query)
{
    bool childVisible = false;
    for (int index = 0; index < item->childCount(); ++index)
        childVisible = filterItem(item->child(index), query) or childVisible;
    const bool ownMatch = query.isEmpty() or
        item->data(0, SymbolSearchRole).toString().contains(query);
    const bool visible = ownMatch or childVisible;
    item->setHidden(not visible);
    if (not query.isEmpty() and childVisible) item->setExpanded(true);
    return visible;
}

void QalamSymbolOutlineView::applyFilter()
{
    const QString query = m_filter->text().trimmed().toCaseFolded();
    bool anyVisible = false;
    for (int index = 0; index < m_tree->topLevelItemCount(); ++index)
        anyVisible = filterItem(m_tree->topLevelItem(index), query) or
                     anyVisible;

    m_tree->setVisible(anyVisible);
    m_emptyLabel->setText(
        query.isEmpty()
            ? QStringLiteral("لا توجد رموز في المستند الحالي")
            : QStringLiteral("لا توجد رموز مطابقة"));
    m_emptyLabel->setVisible(not anyVisible);
}

void QalamSymbolOutlineView::activateItem(QTreeWidgetItem *item)
{
    if (not item or m_filePath.isEmpty()) return;
    const int line = item->data(0, SymbolLineRole).toInt();
    const int column = item->data(0, SymbolColumnRole).toInt();
    if (line > 0 and column > 0)
        emit symbolActivated(m_filePath, line, column);
}
