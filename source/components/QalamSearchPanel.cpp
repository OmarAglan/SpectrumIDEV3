#include "QalamSearchPanel.h"
#include "Constants.h"
#include <QApplication>
#include <QTextDocument>
#include <QStyle>

SearchPanel::SearchPanel(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    // 1. إنشاء التخطيط الأفقي
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // 2. إنشاء المكونات
    searchInput = new QLineEdit(this);
    searchInput->setPlaceholderText("بحث...");
    searchInput->setStyleSheet(QString(
        "border: 1px solid %1; background: %2; color: %3; padding: 4px;")
        .arg(Constants::Colors::Border)
        .arg(Constants::Colors::SidebarBackground)
        .arg(Constants::Colors::TextSecondary));

    btnNext = new QPushButton(this);
    btnNext->setIcon(QIcon(":/icons/resources/down-arrow.svg"));
    btnNext->setToolTip("التالي");
    
    btnPrev = new QPushButton(this);
    btnPrev->setIcon(QIcon(":/icons/resources/up-arrow.svg"));
    btnPrev->setToolTip("السابق");

    // أزرار صغيرة
    QString btnStyle = QString(
        "QPushButton { background: transparent; border: none; padding: 4px; }"
        "QPushButton:hover { background: %1; border-radius: 4px; }")
        .arg(Constants::Colors::ListHoverBackground);
    btnNext->setStyleSheet(btnStyle);
    btnPrev->setStyleSheet(btnStyle);
    btnNext->setFixedSize(26, 26);
    btnPrev->setFixedSize(26, 26);

    // زر الإغلاق (X)
    btnClose = new QPushButton(this);
    btnClose->setIcon(QIcon(":/icons/resources/close.svg"));
    btnClose->setStyleSheet(QString(
        "QPushButton { background: transparent; border: none; color: %1; }"
        "QPushButton:hover { background: %2; border-radius: 7px; }")
        .arg(Constants::Colors::TextPrimary)
        .arg(Constants::Colors::ListActiveBackground));
    btnClose->setFixedSize(35, 35);

    checkCase = new QCheckBox("Aa", this); // Case Sensitive
    checkCase->setToolTip("مطابقة حالة الأحرف");
    checkCase->setStyleSheet(QString("color: %1;").arg(Constants::Colors::TextSecondary));

    checkWord = new QCheckBox("ab", this); // Whole Word
    checkWord->setToolTip("كلمة كاملة فقط");
    checkWord->setStyleSheet(QString("color: %1;").arg(Constants::Colors::TextSecondary));

    // 3. إضافة المكونات للتخطيط
    layout->addWidget(btnClose);
    layout->addWidget(searchInput);
    layout->addWidget(btnNext);
    layout->addWidget(btnPrev);
    layout->addWidget(checkCase);
    layout->addWidget(checkWord);
    layout->addStretch(); // فراغ في النهاية

    // 4. الخلفية واللون
    setStyleSheet(QString("background-color: %1; border-top: 1px solid %2;")
        .arg(Constants::Colors::EditorBackground)
        .arg(Constants::Colors::Border));
    setFixedHeight(45);

    // 5. ربط الإشارات — search logic is handled internally
    connect(btnNext, &QPushButton::clicked, this, &SearchPanel::performFindNext);
    connect(btnPrev, &QPushButton::clicked, this, &SearchPanel::performFindPrev);
    connect(btnClose, &QPushButton::clicked, this, &SearchPanel::closed);

    // البحث عند الضغط على Enter في مربع النص
    connect(searchInput, &QLineEdit::returnPressed, this, &SearchPanel::performFindNext);
    connect(searchInput, &QLineEdit::textChanged, this, &SearchPanel::performFind);
}

void SearchPanel::setEditor(QPlainTextEdit *editor) {
    m_editor = editor;
}

void SearchPanel::performFind() {
    if (!m_editor) return;

    QString text = getText();
    if (text.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (isCaseSensitive()) flags |= QTextDocument::FindCaseSensitively;
    if (isWholeWord()) flags |= QTextDocument::FindWholeWords;

    m_editor->moveCursor(QTextCursor::Start);
    bool found = m_editor->find(text, flags);

    if (!found) {
        QApplication::beep();
    }
}

void SearchPanel::performFindNext() {
    if (!m_editor) return;

    QString text = getText();
    if (text.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (isCaseSensitive()) flags |= QTextDocument::FindCaseSensitively;
    if (isWholeWord()) flags |= QTextDocument::FindWholeWords;

    bool found = m_editor->find(text, flags);

    if (!found) {
        // Wrap around to start
        m_editor->moveCursor(QTextCursor::Start);
        found = m_editor->find(text, flags);
        if (!found) {
            QApplication::beep();
        }
    }
}

void SearchPanel::performFindPrev() {
    if (!m_editor) return;

    QString text = getText();
    if (text.isEmpty()) return;

    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (isCaseSensitive()) flags |= QTextDocument::FindCaseSensitively;
    if (isWholeWord()) flags |= QTextDocument::FindWholeWords;

    bool found = m_editor->find(text, flags);

    if (!found) {
        // Wrap around to end
        m_editor->moveCursor(QTextCursor::End);
        found = m_editor->find(text, flags);
        if (!found) QApplication::beep();
    }
}

QString SearchPanel::getText() const { return searchInput->text(); }
bool SearchPanel::isCaseSensitive() const { return checkCase->isChecked(); }
bool SearchPanel::isWholeWord() const { return checkWord && checkWord->isChecked(); }
void SearchPanel::setFocusToInput() { searchInput->setFocus(); searchInput->selectAll(); }
