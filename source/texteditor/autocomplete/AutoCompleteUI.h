#pragma once

#include "AutoComplete.h"

#include <QAbstractListModel>
#include <QStyledItemDelegate>
#include <QListView>
#include <QLabel>

#include <vector>



// --- Custom Model ---
class CompletionModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit CompletionModel(QObject *parent = nullptr);
    void updateData(const std::vector<CompletionItem>& items);
    const CompletionItem *itemAt(int row) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    std::vector<CompletionItem> m_data{};
};

// --- Rich Popup View (The "Container" for List + Footer) ---
class TCompletionPopup : public QListView {
    Q_OBJECT
public:
    explicit TCompletionPopup(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

private:
    QLabel *infoLabel{};
    int footerHeight{};
};

// --- Modern Delegate ---
class TModernCompletionDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit TModernCompletionDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
