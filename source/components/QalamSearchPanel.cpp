#include "QalamSearchPanel.h"

#include "Constants.h"
#include "QalamEditor.h"

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QRegularExpression>
#include <QShortcut>
#include <QShowEvent>
#include <QStyle>
#include <QTextCursor>
#include <QVBoxLayout>

namespace {

QString localizedNumber(int number)
{
    return QLocale(QLocale::Arabic, QLocale::SaudiArabia).toString(number);
}

}

QalamSearchPanel::QalamSearchPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("QalamSearchPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(78);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 5, 6, 5);
    layout->setSpacing(4);

    auto *searchRow = new QHBoxLayout();
    searchRow->setContentsMargins(0, 0, 0, 0);
    searchRow->setSpacing(5);

    m_closeButton = new QPushButton(this);
    m_closeButton->setObjectName(QStringLiteral("searchCloseButton"));
    m_closeButton->setIcon(QIcon(QStringLiteral(":/icons/resources/close.svg")));
    m_closeButton->setToolTip(QStringLiteral("إغلاق البحث"));
    m_closeButton->setFixedSize(28, 28);

    m_searchInput = new QLineEdit(this);
    m_searchInput->setObjectName(QStringLiteral("searchInput"));
    m_searchInput->setPlaceholderText(QStringLiteral("بحث في الملف الحالي"));
    m_searchInput->setClearButtonEnabled(true);

    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName(QStringLiteral("searchMatchCount"));
    m_countLabel->setAlignment(Qt::AlignCenter);
    m_countLabel->setMinimumWidth(74);
    m_countLabel->setAccessibleName(QStringLiteral("عدد نتائج البحث"));

    m_nextButton = new QPushButton(this);
    m_nextButton->setObjectName(QStringLiteral("searchNextButton"));
    m_nextButton->setIcon(QIcon(QStringLiteral(":/icons/resources/down-arrow.svg")));
    m_nextButton->setToolTip(QStringLiteral("النتيجة التالية (Enter)"));
    m_nextButton->setFixedSize(28, 28);

    m_previousButton = new QPushButton(this);
    m_previousButton->setObjectName(QStringLiteral("searchPreviousButton"));
    m_previousButton->setIcon(QIcon(QStringLiteral(":/icons/resources/up-arrow.svg")));
    m_previousButton->setToolTip(QStringLiteral("النتيجة السابقة (Shift+Enter)"));
    m_previousButton->setFixedSize(28, 28);

    m_caseCheck = new QCheckBox(QStringLiteral("Aa"), this);
    m_caseCheck->setObjectName(QStringLiteral("searchCaseSensitive"));
    m_caseCheck->setToolTip(QStringLiteral("مطابقة حالة الأحرف"));

    m_wordCheck = new QCheckBox(QStringLiteral("ab"), this);
    m_wordCheck->setObjectName(QStringLiteral("searchWholeWord"));
    m_wordCheck->setToolTip(QStringLiteral("مطابقة كلمة كاملة"));

    m_regexCheck = new QCheckBox(QStringLiteral(".*"), this);
    m_regexCheck->setObjectName(QStringLiteral("searchRegex"));
    m_regexCheck->setToolTip(QStringLiteral("استخدام تعبير نمطي"));

    searchRow->addWidget(m_closeButton);
    searchRow->addWidget(m_searchInput, 1);
    searchRow->addWidget(m_countLabel);
    searchRow->addWidget(m_nextButton);
    searchRow->addWidget(m_previousButton);
    searchRow->addWidget(m_caseCheck);
    searchRow->addWidget(m_wordCheck);
    searchRow->addWidget(m_regexCheck);

    auto *replaceRow = new QHBoxLayout();
    replaceRow->setContentsMargins(33, 0, 0, 0);
    replaceRow->setSpacing(5);

    m_replaceInput = new QLineEdit(this);
    m_replaceInput->setObjectName(QStringLiteral("replaceInput"));
    m_replaceInput->setPlaceholderText(QStringLiteral("استبدال بـ"));
    m_replaceInput->setClearButtonEnabled(true);

    m_replaceButton = new QPushButton(QStringLiteral("استبدال"), this);
    m_replaceButton->setObjectName(QStringLiteral("replaceButton"));
    m_replaceButton->setIcon(QIcon(QStringLiteral(":/icons/resources/replace.svg")));
    m_replaceButton->setIconSize(QSize(16, 16));
    m_replaceButton->setToolTip(QStringLiteral("استبدال النتيجة الحالية"));

    m_replaceAllButton = new QPushButton(QStringLiteral("استبدال الكل"), this);
    m_replaceAllButton->setObjectName(QStringLiteral("replaceAllButton"));
    m_replaceAllButton->setIcon(QIcon(QStringLiteral(":/icons/resources/replace-all.svg")));
    m_replaceAllButton->setIconSize(QSize(16, 16));
    m_replaceAllButton->setToolTip(QStringLiteral("استبدال كل النتائج في الملف"));

    replaceRow->addWidget(m_replaceInput, 1);
    replaceRow->addWidget(m_replaceButton);
    replaceRow->addWidget(m_replaceAllButton);

    layout->addLayout(searchRow);
    layout->addLayout(replaceRow);

    setStyleSheet(QStringLiteral(R"(
        QWidget#QalamSearchPanel {
            background-color: %1;
            border-top: 1px solid %2;
        }
        QLineEdit {
            border: 1px solid %2;
            background: %3;
            color: %4;
            padding: 3px 6px;
        }
        QLineEdit:focus { border-color: %5; }
        QLineEdit[invalidSearch="true"] { border-color: %6; }
        QLabel, QCheckBox { color: %4; }
        QPushButton {
            background: transparent;
            border: none;
            color: %4;
            padding: 4px 7px;
        }
        QPushButton:hover {
            background: %7;
            border-radius: 4px;
        }
        QPushButton:disabled { color: %8; }
    )")
        .arg(Constants::Colors::EditorBackground,
             Constants::Colors::Border,
             Constants::Colors::SidebarBackground,
             Constants::Colors::TextSecondary,
             Constants::Colors::BorderFocus,
             Constants::Colors::ErrorForeground,
             Constants::Colors::ListHoverBackground,
             Constants::Colors::TextDisabled));

    connect(m_searchInput, &QLineEdit::textChanged,
            this, &QalamSearchPanel::performFind);
    connect(m_searchInput, &QLineEdit::returnPressed,
            this, &QalamSearchPanel::performFindNext);
    connect(m_nextButton, &QPushButton::clicked,
            this, &QalamSearchPanel::performFindNext);
    connect(m_previousButton, &QPushButton::clicked,
            this, &QalamSearchPanel::performFindPrevious);
    connect(m_replaceButton, &QPushButton::clicked,
            this, &QalamSearchPanel::performReplace);
    connect(m_replaceAllButton, &QPushButton::clicked,
            this, &QalamSearchPanel::performReplaceAll);
    connect(m_closeButton, &QPushButton::clicked,
            this, &QalamSearchPanel::closed);
    connect(m_caseCheck, &QCheckBox::toggled,
            this, &QalamSearchPanel::performFind);
    connect(m_wordCheck, &QCheckBox::toggled,
            this, &QalamSearchPanel::performFind);
    connect(m_regexCheck, &QCheckBox::toggled,
            this, &QalamSearchPanel::performFind);

    auto *previousShortcut = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return), this);
    previousShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(previousShortcut, &QShortcut::activated,
            this, &QalamSearchPanel::performFindPrevious);

    auto *closeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(closeShortcut, &QShortcut::activated, this, &QalamSearchPanel::closed);

    updateCountLabel();
}

QString QalamSearchPanel::searchText() const
{
    return m_searchInput->text();
}

QString QalamSearchPanel::replacementText() const
{
    return m_replaceInput->text();
}

bool QalamSearchPanel::isCaseSensitive() const
{
    return m_caseCheck->isChecked();
}

bool QalamSearchPanel::isWholeWord() const
{
    return m_wordCheck->isChecked();
}

bool QalamSearchPanel::isRegex() const
{
    return m_regexCheck->isChecked();
}

int QalamSearchPanel::currentMatchNumber() const
{
    return m_currentMatchIndex >= 0 ? m_currentMatchIndex + 1 : 0;
}

void QalamSearchPanel::setEditor(QalamEditor *editor)
{
    if (m_editor == editor) {
        if (m_editor and m_searchActive) {
            refreshMatches(m_editor->textCursor().position(), false);
        }
        return;
    }

    if (m_editor) {
        disconnect(m_editor, &QPlainTextEdit::textChanged,
                   this, &QalamSearchPanel::refreshAfterEditorChange);
        m_editor->clearSearchHighlights();
    }

    m_editor = editor;
    m_matches.clear();
    m_currentMatchIndex = -1;

    if (m_editor) {
        connect(m_editor, &QPlainTextEdit::textChanged,
                this, &QalamSearchPanel::refreshAfterEditorChange,
                Qt::UniqueConnection);
        if (m_searchActive) {
            refreshMatches(m_editor->textCursor().position(), false);
        } else {
            updateCountLabel();
        }
    } else {
        updateCountLabel();
    }
}

void QalamSearchPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_searchActive = true;
    if (m_editor) refreshMatches(m_editor->textCursor().position(), false);
}

void QalamSearchPanel::hideEvent(QHideEvent *event)
{
    m_searchActive = false;
    clearHighlights();
    QWidget::hideEvent(event);
}

void QalamSearchPanel::setFocusToInput()
{
    if (m_editor) {
        const QString selected = m_editor->textCursor().selectedText();
        if (not selected.isEmpty() and not selected.contains(QChar::ParagraphSeparator) and
            selected.size() <= 256) {
            m_searchInput->setText(selected);
        }
    }
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

void QalamSearchPanel::clearHighlights()
{
    m_matches.clear();
    m_currentMatchIndex = -1;
    if (m_editor) m_editor->clearSearchHighlights();
    updateCountLabel();
}

QRegularExpression QalamSearchPanel::buildPattern() const
{
    QString pattern = isRegex()
        ? searchText()
        : QRegularExpression::escape(searchText());

    if (isWholeWord() and not pattern.isEmpty()) {
        pattern = QStringLiteral(
            "(?<![\\p{L}\\p{M}\\p{N}_])(?:%1)(?![\\p{L}\\p{M}\\p{N}_])")
                      .arg(pattern);
    }

    QRegularExpression::PatternOptions options =
        QRegularExpression::UseUnicodePropertiesOption;
    if (not isCaseSensitive()) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    return QRegularExpression(pattern, options);
}

void QalamSearchPanel::performFind()
{
    if (not m_searchActive) return;
    const int position = m_editor ? m_editor->textCursor().selectionStart() : 0;
    refreshMatches(position, true);
}

void QalamSearchPanel::performFindNext()
{
    if (not m_editor or searchText().isEmpty()) return;
    if (m_matches.isEmpty()) {
        refreshMatches(m_editor->textCursor().position(), true);
        return;
    }
    selectMatch((m_currentMatchIndex + 1) % m_matches.size());
}

void QalamSearchPanel::performFindPrevious()
{
    if (not m_editor or searchText().isEmpty()) return;
    if (m_matches.isEmpty()) {
        refreshMatches(m_editor->textCursor().position(), true);
        return;
    }
    const int previous = m_currentMatchIndex <= 0
        ? m_matches.size() - 1
        : m_currentMatchIndex - 1;
    selectMatch(previous);
}

void QalamSearchPanel::performReplace()
{
    if (not m_editor or m_currentMatchIndex < 0 or
        m_currentMatchIndex >= m_matches.size()) {
        QApplication::beep();
        return;
    }

    const Match match = m_matches.at(m_currentMatchIndex);
    m_editor->clearSearchHighlights();
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(match.start);
    cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);

    m_replacing = true;
    cursor.insertText(replacementFor(match));
    m_replacing = false;
    m_editor->setTextCursor(cursor);
    refreshMatches(cursor.position(), true);
}

void QalamSearchPanel::performReplaceAll()
{
    if (not m_editor or m_matches.isEmpty()) {
        QApplication::beep();
        return;
    }

    m_replacing = true;
    m_editor->clearSearchHighlights();
    QTextCursor editCursor(m_editor->document());
    editCursor.beginEditBlock();
    for (int index = m_matches.size() - 1; index >= 0; --index) {
        const Match &match = m_matches.at(index);
        QTextCursor cursor(m_editor->document());
        cursor.setPosition(match.start);
        cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);
        cursor.insertText(replacementFor(match));
    }
    editCursor.endEditBlock();
    m_replacing = false;

    refreshMatches(m_editor->textCursor().position(), false);
}

void QalamSearchPanel::refreshAfterEditorChange()
{
    if (m_replacing or not m_editor) return;
    refreshMatches(m_editor->textCursor().position(), false);
}

void QalamSearchPanel::refreshMatches(int preferredPosition, bool selectCurrent)
{
    m_matches.clear();
    m_currentMatchIndex = -1;

    if (not m_editor or searchText().isEmpty()) {
        m_patternValid = true;
        updatePatternState();
        publishHighlights();
        updateCountLabel();
        return;
    }

    const QRegularExpression pattern = buildPattern();
    m_patternValid = pattern.isValid();
    updatePatternState();
    if (not m_patternValid) {
        publishHighlights();
        updateCountLabel();
        return;
    }

    QRegularExpressionMatchIterator iterator =
        pattern.globalMatch(m_editor->toPlainText());
    while (iterator.hasNext()) {
        const QRegularExpressionMatch regularMatch = iterator.next();
        if (regularMatch.capturedLength() <= 0) continue;

        Match match;
        match.start = regularMatch.capturedStart();
        match.length = regularMatch.capturedLength();
        match.captures = regularMatch.capturedTexts();
        m_matches.push_back(std::move(match));
    }

    if (not m_matches.isEmpty()) {
        m_currentMatchIndex = 0;
        for (int index = 0; index < m_matches.size(); ++index) {
            if (m_matches.at(index).start >= preferredPosition) {
                m_currentMatchIndex = index;
                break;
            }
        }
    }

    publishHighlights();
    updateCountLabel();
    if (selectCurrent and m_currentMatchIndex >= 0) {
        selectMatch(m_currentMatchIndex);
    } else if (selectCurrent and m_matches.isEmpty()) {
        QApplication::beep();
    }
}

void QalamSearchPanel::selectMatch(int index)
{
    if (not m_editor or index < 0 or index >= m_matches.size()) return;

    m_currentMatchIndex = index;
    const Match &match = m_matches.at(index);
    QTextCursor cursor(m_editor->document());
    cursor.setPosition(match.start);
    cursor.setPosition(match.start + match.length, QTextCursor::KeepAnchor);
    m_editor->setTextCursor(cursor);
    m_editor->centerCursor();
    publishHighlights();
    updateCountLabel();
}

void QalamSearchPanel::publishHighlights()
{
    if (not m_editor) return;

    QVector<QPair<int, int>> ranges;
    ranges.reserve(m_matches.size());
    for (const Match &match : m_matches) {
        ranges.push_back({match.start, match.length});
    }
    m_editor->setSearchHighlights(ranges, m_currentMatchIndex);
}

void QalamSearchPanel::updateCountLabel()
{
    if (not m_patternValid) {
        m_countLabel->setText(QStringLiteral("نمط غير صالح"));
    } else if (searchText().isEmpty()) {
        m_countLabel->setText(QStringLiteral("—"));
    } else if (m_matches.isEmpty()) {
        m_countLabel->setText(QStringLiteral("لا نتائج"));
    } else {
        m_countLabel->setText(QStringLiteral("%1 من %2")
            .arg(localizedNumber(currentMatchNumber()),
                 localizedNumber(m_matches.size())));
    }

    const bool hasMatches = not m_matches.isEmpty();
    m_nextButton->setEnabled(hasMatches);
    m_previousButton->setEnabled(hasMatches);
    m_replaceButton->setEnabled(hasMatches);
    m_replaceAllButton->setEnabled(hasMatches);
}

void QalamSearchPanel::updatePatternState()
{
    m_searchInput->setProperty("invalidSearch", not m_patternValid);
    m_searchInput->setToolTip(
        m_patternValid ? QString() : QStringLiteral("التعبير النمطي غير صالح"));
    m_searchInput->style()->unpolish(m_searchInput);
    m_searchInput->style()->polish(m_searchInput);
}

QString QalamSearchPanel::replacementFor(const Match &match) const
{
    const QString replacement = replacementText();
    if (not isRegex()) return replacement;

    QString expanded;
    expanded.reserve(replacement.size());
    for (int index = 0; index < replacement.size(); ++index) {
        const QChar character = replacement.at(index);
        if ((character == QChar('\\') or character == QChar('$')) and
            index + 1 < replacement.size() and replacement.at(index + 1).isDigit()) {
            int captureNumber = 0;
            int digitIndex = index + 1;
            while (digitIndex < replacement.size() and
                   replacement.at(digitIndex).isDigit()) {
                captureNumber = captureNumber * 10 + replacement.at(digitIndex).digitValue();
                ++digitIndex;
            }
            if (captureNumber >= 0 and captureNumber < match.captures.size()) {
                expanded += match.captures.at(captureNumber);
                index = digitIndex - 1;
                continue;
            }
        }
        expanded += character;
    }
    return expanded;
}
