#include "QalamCommandPalette.h"

#include "Constants.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScreen>
#include <QShowEvent>
#include <QVBoxLayout>

QalamCommandPalette::QalamCommandPalette(QWidget *parent)
    : QDialog(parent)
{
    setObjectName("commandPaletteDialog");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::Popup);
    setModal(true);
    setLayoutDirection(Qt::RightToLeft);
    setupUi();
    applyStyles();
}

void QalamCommandPalette::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *frame = new QFrame(this);
    frame->setObjectName("commandPaletteFrame");
    root->addWidget(frame);

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    m_input = new QLineEdit(frame);
    m_input->setObjectName("commandPaletteInput");
    m_input->setPlaceholderText("اكتب أمراً أو اسم ملف...");
    m_input->installEventFilter(this);
    layout->addWidget(m_input);

    m_list = new QListWidget(frame);
    m_list->setObjectName("commandPaletteList");
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setUniformItemSizes(false);
    m_list->installEventFilter(this);
    layout->addWidget(m_list, 1);

    setMinimumWidth(620);
    setMaximumWidth(760);
    resize(680, 420);

    connect(m_input, &QLineEdit::textChanged, this, &QalamCommandPalette::filterEntries);
    connect(m_input, &QLineEdit::returnPressed, this, &QalamCommandPalette::activateCurrentItem);
    connect(m_list, &QListWidget::itemActivated, this, &QalamCommandPalette::activateItem);
    connect(m_list, &QListWidget::itemClicked, this, &QalamCommandPalette::activateItem);
}

void QalamCommandPalette::setPlaceholderText(const QString &text)
{
    if (m_input) {
        m_input->setPlaceholderText(text);
    }
}

void QalamCommandPalette::setEmptyText(const QString &text)
{
    m_emptyText = text;
}

void QalamCommandPalette::setEntries(const QVector<Entry> &entries)
{
    m_entries = entries;
    filterEntries();
}

void QalamCommandPalette::setInitialQuery(const QString &query)
{
    if (m_input) {
        m_input->setText(query);
        m_input->selectAll();
    }
}

void QalamCommandPalette::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if (QWidget *parentWidget = qobject_cast<QWidget*>(parent())) {
        const int x = parentWidget->geometry().center().x() - width() / 2;
        const int y = parentWidget->geometry().top() + 58;
        move(qMax(parentWidget->geometry().left() + 12, x), y);
    } else if (QScreen *screen = QApplication::primaryScreen()) {
        const QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.center().x() - width() / 2, screenGeometry.top() + 80);
    }

    if (m_input) {
        m_input->setFocus(Qt::PopupFocusReason);
        m_input->selectAll();
    }
}

bool QalamCommandPalette::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            reject();
            return true;
        }
        if (watched == m_input) {
            if (keyEvent->key() == Qt::Key_Down) {
                const int next = qMin(m_list->count() - 1, m_list->currentRow() + 1);
                m_list->setCurrentRow(next);
                return true;
            }
            if (keyEvent->key() == Qt::Key_Up) {
                const int previous = qMax(0, m_list->currentRow() - 1);
                m_list->setCurrentRow(previous);
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void QalamCommandPalette::filterEntries()
{
    if (!m_list) return;

    m_list->clear();
    const QString query = m_input ? m_input->text().trimmed() : QString();

    for (const Entry &entry : m_entries) {
        if (matches(entry, query)) {
            addVisibleEntry(entry);
        }
    }

    if (m_list->count() == 0) {
        auto *item = new QListWidgetItem(m_emptyText, m_list);
        item->setFlags(Qt::NoItemFlags);
        item->setSizeHint(QSize(0, 42));
        m_list->addItem(item);
        return;
    }

    m_list->setCurrentRow(0);
}

bool QalamCommandPalette::matches(const Entry &entry, const QString &query) const
{
    if (query.isEmpty()) return true;

    const QString normalized = query.toCaseFolded();
    return entry.title.toCaseFolded().contains(normalized)
        || entry.subtitle.toCaseFolded().contains(normalized)
        || entry.shortcut.toCaseFolded().contains(normalized)
        || entry.id.toCaseFolded().contains(normalized);
}

void QalamCommandPalette::addVisibleEntry(const Entry &entry)
{
    auto *item = new QListWidgetItem(m_list);
    item->setData(Qt::UserRole, entry.id);
    item->setData(Qt::UserRole + 1, entry.payload);
    item->setSizeHint(QSize(0, 54));
    m_list->addItem(item);

    auto *rowWidget = new QWidget(m_list);
    rowWidget->setObjectName("commandPaletteRow");
    auto *row = new QHBoxLayout(rowWidget);
    row->setContentsMargins(10, 6, 10, 6);
    row->setSpacing(10);
    row->setDirection(QBoxLayout::RightToLeft);

    auto *textColumn = new QVBoxLayout();
    textColumn->setContentsMargins(0, 0, 0, 0);
    textColumn->setSpacing(2);

    auto *title = new QLabel(entry.title, rowWidget);
    title->setObjectName("commandPaletteTitle");
    auto *subtitle = new QLabel(entry.subtitle, rowWidget);
    subtitle->setObjectName("commandPaletteSubtitle");

    textColumn->addWidget(title);
    if (!entry.subtitle.isEmpty()) {
        textColumn->addWidget(subtitle);
    }

    row->addLayout(textColumn, 1);

    if (!entry.shortcut.isEmpty()) {
        auto *shortcut = new QLabel(entry.shortcut, rowWidget);
        shortcut->setObjectName("commandPaletteShortcut");
        row->addWidget(shortcut, 0, Qt::AlignVCenter);
    }

    m_list->setItemWidget(item, rowWidget);
}

void QalamCommandPalette::activateCurrentItem()
{
    activateItem(m_list ? m_list->currentItem() : nullptr);
}

void QalamCommandPalette::activateItem(QListWidgetItem *item)
{
    if (!item) return;
    const QString id = item->data(Qt::UserRole).toString();
    const QString payload = item->data(Qt::UserRole + 1).toString();
    if (id.isEmpty()) return;

    emit commandActivated(id);
    emit entryActivated(id, payload);
    accept();
}

void QalamCommandPalette::applyStyles()
{
    using namespace Constants;
    setStyleSheet(QString(R"(
        QDialog#commandPaletteDialog {
            background: transparent;
        }

        #commandPaletteFrame {
            background-color: %3;
            border: 1px solid %4;
            border-radius: 8px;
        }

        #commandPaletteInput {
            background-color: %5;
            border: 1px solid %6;
            border-radius: 4px;
            padding: 8px 10px;
            color: %1;
            font-size: 14px;
            selection-background-color: %7;
        }

        #commandPaletteList {
            background-color: %3;
            border: none;
            outline: 0;
        }

        #commandPaletteList::item {
            border-radius: 4px;
            margin: 1px 0px;
        }

        #commandPaletteList::item:hover {
            background-color: %8;
        }

        #commandPaletteList::item:selected {
            background-color: %9;
        }

        #commandPaletteRow {
            background: transparent;
        }

        #commandPaletteTitle {
            color: %1;
            font-size: 13px;
            font-weight: 600;
        }

        #commandPaletteSubtitle {
            color: %2;
            font-size: 11px;
        }

        #commandPaletteShortcut {
            color: %2;
            background-color: %5;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 2px 6px;
            font-size: 11px;
        }
    )")
    .arg(Colors::TextPrimary)
    .arg(Colors::TextMuted)
    .arg(Colors::MenuBackground)
    .arg(Colors::Border)
    .arg(Colors::InputBackground)
    .arg(Colors::BorderFocus)
    .arg(Colors::Selection)
    .arg(Colors::ListHoverBackground)
    .arg(Colors::ListSelectionBackground));
}
