#include "AutoCompleteUI.h"

#include <QScrollBar>
#include <QPainter>
#include <QResizeEvent>


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

// --- QalamCompletionPopup Implementation ---

QalamCompletionPopup::QalamCompletionPopup(QWidget *parent) : QListView(parent), footerHeight(72) {
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
    infoLabel->setObjectName(QStringLiteral("completionInfoLabel"));
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
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setLayoutDirection(Qt::RightToLeft);

    // Reserve space at bottom so list items don't overlap the footer
    setViewportMargins(0, 0, 0, footerHeight);
}

void QalamCompletionPopup::resizeEvent(QResizeEvent *event) {
    QListView::resizeEvent(event);
    updateFooterLayout();
}

void QalamCompletionPopup::updateFooterLayout() {
    QRect cr = contentsRect();
    if (cr.width() <= 0) return;

    // Rich text and Arabic fonts need more room than the old fixed 52-pixel
    // footer. Measure the current description at the popup width so both its
    // heading and body remain visible, while keeping enough space for results.
    infoLabel->setFixedWidth(cr.width());
    const int measuredHeight = infoLabel->heightForWidth(cr.width());
    const int requiredHeight = qBound(72, measuredHeight, 120);
    if (requiredHeight != footerHeight) {
        footerHeight = requiredHeight;
        setViewportMargins(0, 0, 0, footerHeight);
        cr = contentsRect();
    }

    infoLabel->setGeometry(cr.left(), cr.bottom() - footerHeight + 1, cr.width(), footerHeight);
    setProperty("qalam.completionFooterHeight", footerHeight);
}

void QalamCompletionPopup::currentChanged(const QModelIndex &current, const QModelIndex &previous) {
    QListView::currentChanged(current, previous);
    if (!current.isValid()) {
        infoLabel->clear();
        updateFooterLayout();
        return;
    }

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
    case File: typeStr = "ملف باء"; colorStr = "#61afef"; break;
    case Folder: typeStr = "مجلد"; colorStr = "#e5c07b"; break;
    }

    QString html = QString("<div dir='rtl'>"
                           "<span style='font-weight:bold; color:%1; font-size:14px;'>%2</span>"
                           "<br>"
                           "<span style='font-family: Tajawal; font-size:12px; color: #dcdfe4;'>%3</span>"
                           "</div>")
                       .arg(colorStr, typeStr, desc.toHtmlEscaped().replace("\n", "<br>"));
    infoLabel->setText(html);
    updateFooterLayout();
}

// --- Modern Delegate Implementation ---
QalamModernCompletionDelegate::QalamModernCompletionDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize QalamModernCompletionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const {
    // Shorter rows now that description is at the bottom
    return QSize(option.rect.width(), 30);
}

void QalamModernCompletionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Data
    QString label = index.data(Qt::DisplayRole).toString();
    CompletionType type = static_cast<CompletionType>(index.data(Qt::UserRole + 2).toInt());

    // Colors
    QColor bgColor = (option.state & QStyle::State_Selected) ? QColor(62, 68, 81) : QColor(30, 32, 46); // Matches popup bg
    QColor iconColor;
    switch (type) {
    case Keyword: iconColor = QColor(198, 120, 221); break;
    case Snippet: iconColor = QColor(224, 108, 117); break;
    case Function: iconColor = QColor(130, 212, 72); break;
    case Variable: iconColor = QColor(97, 175, 239); break;
    case Type: iconColor = QColor(86, 182, 194); break;
    case Value: iconColor = QColor(171, 178, 191); break;
    case Preprocessor: iconColor = QColor(209, 154, 102); break;
    case File: iconColor = QColor(97, 175, 239); break;
    case Folder: iconColor = QColor(229, 192, 123); break;
    }

    // Draw Background
    // Note: We don't draw rounded rect here because the container handles the main border.
    // Just fill rect.
    painter->fillRect(option.rect, bgColor);

    // Draw Icon (Right Side)
    // ---------------------
    const int iconWidth = 34;
    const QRectF iconRect(option.rect.right() - 25,
                          option.rect.center().y() - 7, 14, 14);
    painter->setPen(QPen(iconColor, 1.6, Qt::SolidLine,
                         Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    switch (type) {
    case Keyword: {
        const QPointF center = iconRect.center();
        QPolygonF diamond;
        diamond << QPointF(center.x(), iconRect.top())
                << QPointF(iconRect.right(), center.y())
                << QPointF(center.x(), iconRect.bottom())
                << QPointF(iconRect.left(), center.y());
        painter->drawPolygon(diamond);
        break;
    }
    case Snippet:
        painter->drawLine(iconRect.topLeft(), iconRect.bottomLeft());
        painter->drawLine(iconRect.topLeft(),
                          iconRect.topLeft() + QPointF(4, 0));
        painter->drawLine(iconRect.bottomLeft(),
                          iconRect.bottomLeft() + QPointF(4, 0));
        painter->drawLine(iconRect.topRight(), iconRect.bottomRight());
        painter->drawLine(iconRect.topRight(),
                          iconRect.topRight() - QPointF(4, 0));
        painter->drawLine(iconRect.bottomRight(),
                          iconRect.bottomRight() - QPointF(4, 0));
        break;
    case Function:
        painter->drawEllipse(iconRect.adjusted(1, 1, -1, -1));
        painter->drawLine(iconRect.left() + 4, iconRect.center().y(),
                          iconRect.right() - 4, iconRect.center().y());
        break;
    case Variable:
        painter->drawRoundedRect(iconRect.adjusted(1, 1, -1, -1), 2, 2);
        break;
    case Type: {
        const QPointF center = iconRect.center();
        QPolygonF hexagon;
        hexagon << QPointF(center.x(), iconRect.top())
                << QPointF(iconRect.right(), iconRect.top() + 4)
                << QPointF(iconRect.right(), iconRect.bottom() - 4)
                << QPointF(center.x(), iconRect.bottom())
                << QPointF(iconRect.left(), iconRect.bottom() - 4)
                << QPointF(iconRect.left(), iconRect.top() + 4);
        painter->drawPolygon(hexagon);
        break;
    }
    case Value:
        painter->drawEllipse(iconRect.adjusted(3, 3, -3, -3));
        break;
    case Preprocessor: {
        QPolygonF triangle;
        triangle << QPointF(iconRect.center().x(), iconRect.top())
                 << iconRect.bottomRight() << iconRect.bottomLeft();
        painter->drawPolygon(triangle);
        break;
    }
    case File:
        painter->drawRect(iconRect.adjusted(2, 1, -2, -1));
        painter->drawLine(iconRect.left() + 4, iconRect.top() + 5,
                          iconRect.right() - 4, iconRect.top() + 5);
        break;
    case Folder:
        painter->drawRoundedRect(iconRect.adjusted(0, 3, 0, -1), 1.5, 1.5);
        painter->drawLine(iconRect.left() + 1, iconRect.top() + 3,
                          iconRect.center().x(), iconRect.top() + 3);
        break;
    }

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
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignRight, label);

    // Selection Highlight Bar (Left Edge)
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect.right() - 2, option.rect.top(), 2, option.rect.height(), iconColor);
    }

    painter->restore();
}
