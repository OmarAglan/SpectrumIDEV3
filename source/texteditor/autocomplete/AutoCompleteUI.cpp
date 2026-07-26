#include "AutoCompleteUI.h"

#include <QScrollBar>
#include <QPainter>


// --- CompletionModel ---
CompletionModel::CompletionModel(QObject *parent) : QAbstractListModel(parent) {}

void CompletionModel::updateData(const std::vector<CompletionItem>& items) {
    beginResetModel();
    m_data.clear();
    for (const auto& item : items) {
        m_data.push_back(item);
    }
    endResetModel();
}

const CompletionItem *CompletionModel::itemAt(int row) const {
    if (row < 0 || static_cast<size_t>(row) >= m_data.size()) return nullptr;
    return &m_data[static_cast<size_t>(row)];
}

int CompletionModel::rowCount(const QModelIndex &) const {
    return static_cast<int>(m_data.size());
}

QVariant CompletionModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_data.size()) return QVariant();
    const auto &item = m_data[index.row()];

    if (role == Qt::DisplayRole) return item.label;
    if (role == Qt::EditRole) return item.completion;
    // Custom roles for the delegate
    if (role == Qt::UserRole + 1) return item.description;
    if (role == Qt::UserRole + 2) return static_cast<int>(item.type);

    return QVariant();
}

// --- TCompletionPopup Implementation ---

TCompletionPopup::TCompletionPopup(QWidget *parent) : QListView(parent), footerHeight(52) {
    // 1. Visual Properties for the Container
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    // setAttribute(Qt::WA_TranslucentBackground);

    setLayoutDirection(Qt::RightToLeft);
    // Hide horizontal scrollbar always
    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);

    // Style the list itself
    setStyleSheet(
        "QListView { "
        "   background-color: #1e202e; "
        "   border: 1px solid #4b5263; "
        // "   border-radius: 7px; "
        "   color: #abb2bf; "
        "   outline: none; "
        "}"
        "QListView::item:selected { background-color: #3e4451; }"
        );

    // 2. The Info Panel (Label)
    infoLabel = new QLabel(this);
    infoLabel->setStyleSheet(
        "QLabel { "
        "   background-color: #2c313a; "
        "   border-top: 1px solid #4793FF; "
        "   border-top-left-radius: 8px; "
        "   border-top-right-radius: 8px; "
        "   color: #9da5b4; "
        "   padding: 8px; "
        "   font-family: 'Tajawal'; "
        "}"
        );
    infoLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);
    infoLabel->setWordWrap(true);
    infoLabel->setLayoutDirection(Qt::RightToLeft); // Set RTL for label

    // Reserve space at bottom so list items don't overlap the footer
    setViewportMargins(0, 0, 0, footerHeight);
}

void TCompletionPopup::resizeEvent(QResizeEvent *event) {
    QListView::resizeEvent(event);
    // Force the label to stay at the bottom of the visible area
    QRect cr = contentsRect();
    infoLabel->setGeometry(cr.left(), cr.bottom() - footerHeight + 1, cr.width(), footerHeight);
}

void TCompletionPopup::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
    QListView::currentChanged(current, previous);
    if (!current.isValid()) { infoLabel->clear(); return; }

    QString desc = current.data(Qt::UserRole + 1).toString();
    CompletionType type = static_cast<CompletionType>(current.data(Qt::UserRole + 2).toInt());
    QString typeStr, colorStr;

    switch(type) {
    case Keyword: typeStr = "كلمة محجوزة"; colorStr = "#c678dd"; break;
    case Snippet: typeStr = "قالب"; colorStr = "#e06c75"; break;
    case Function: typeStr = "دالة"; colorStr = "#82d448"; break;
    case Variable: typeStr = "متغير"; colorStr = "#61afef"; break;
    case Type: typeStr = "نوع"; colorStr = "#56b6c2"; break;
    case Value: typeStr = "قيمة"; colorStr = "#abb2bf"; break;
    case Preprocessor: typeStr = "معالجة قبلية"; colorStr = "#d19a66"; break;
    }

    QString html = QString("<div dir='rtl'>"
                           "<span style='font-weight:bold; color:%1; font-size:14px;'>%2</span>"
                           "<br>"
                           "<span style='font-family: Tajawal; font-size:12px; color: #dcdfe4;'>%3</span>"
                           "</div>")
                       .arg(colorStr, typeStr, desc.toHtmlEscaped().replace("\n", "<br>"));
    infoLabel->setText(html);
}

// --- Modern Delegate Implementation ---
TModernCompletionDelegate::TModernCompletionDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize TModernCompletionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const {
    // Shorter rows now that description is at the bottom
    return QSize(option.rect.width(), 30);
}

void TModernCompletionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Data
    QString label = index.data(Qt::DisplayRole).toString();
    CompletionType type = static_cast<CompletionType>(index.data(Qt::UserRole + 2).toInt());

    // Colors
    QColor bgColor = (option.state & QStyle::State_Selected) ? QColor(62, 68, 81) : QColor(30, 32, 46); // Matches popup bg
    QColor iconColor;
    QString iconText;

    switch (type) {
    case Keyword: iconColor = QColor(198, 120, 221); iconText = "ك"; break;
    case Snippet: iconColor = QColor(224, 108, 117); iconText = "ق"; break;
    case Function: iconColor = QColor(130, 212, 72); iconText = "د"; break;
    case Variable: iconColor = QColor(97, 175, 239); iconText = "م"; break;
    case Type: iconColor = QColor(86, 182, 194); iconText = "ن"; break;
    case Value: iconColor = QColor(171, 178, 191); iconText = "ق"; break;
    case Preprocessor: iconColor = QColor(209, 154, 102); iconText = "مق"; break;
    }

    // Draw Background
    // Note: We don't draw rounded rect here because the container handles the main border.
    // Just fill rect.
    painter->fillRect(option.rect, bgColor);

    // Draw Icon (Right Side)
    // ---------------------
    int iconWidth = 35;
    QRect iconRect(option.rect.right() - iconWidth, option.rect.top(), iconWidth, option.rect.height());

    // Icon text
    painter->setPen(iconColor);
    painter->setFont(QFont("Tajawal", 9, QFont::Bold));
    painter->drawText(iconRect, Qt::AlignCenter, iconText);

    // Draw Label (Main Text)
    // ----------------------
    QRect textRect = option.rect.adjusted(10, 0, -iconWidth, 0);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(Qt::white);
    } else {
        painter->setPen(QColor(171, 178, 191));
    }

    QFont mainFont("Tajawal", 10);
    // if (type == Keyword) mainFont.setBold(true);
    painter->setFont(mainFont);

    // Draw text vertically centered
    painter->drawText(textRect, Qt::AlignVCenter, label);

    // Selection Highlight Bar (Left Edge)
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect.right() - 2, option.rect.top(), 2, option.rect.height(), iconColor);
    }

    painter->restore();
}
