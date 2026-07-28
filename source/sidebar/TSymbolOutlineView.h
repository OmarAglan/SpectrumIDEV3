#pragma once

#include "BaaDocumentSymbol.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

class TSymbolOutlineView : public QWidget
{
    Q_OBJECT

public:
    explicit TSymbolOutlineView(QWidget *parent = nullptr);

    void setSymbols(const QString &filePath,
                    const QVector<BaaDocumentSymbol> &symbols);
    void clearSymbols();
    void setFilterText(const QString &text);
    QTreeWidget *treeWidget() const { return m_tree; }

signals:
    void symbolActivated(const QString &filePath, int line, int column);

private:
    void appendSymbol(QTreeWidgetItem *parent,
                      const BaaDocumentSymbol &symbol);
    bool filterItem(QTreeWidgetItem *item, const QString &query);
    void applyFilter();
    void activateItem(QTreeWidgetItem *item);

    QString m_filePath;
    QLineEdit *m_filter{};
    QTreeWidget *m_tree{};
    QLabel *m_emptyLabel{};
};
