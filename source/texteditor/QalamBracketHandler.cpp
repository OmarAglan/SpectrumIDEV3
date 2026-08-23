#include "QalamBracketHandler.h"

namespace {
enum class QuoteScanState {
    Code,
    LineComment,
    BlockComment,
    DoubleQuote,
    SingleQuote,
    Backtick
};

QChar activeQuoteBefore(const QString &text, int cursorPosition)
{
    QuoteScanState state = QuoteScanState::Code;
    const int limit = qBound(0, cursorPosition, text.size());

    for (int index = 0; index < limit; ++index) {
        const QChar current = text.at(index);
        const QChar next = index + 1 < limit ? text.at(index + 1) : QChar{};

        if (state == QuoteScanState::LineComment) {
            if (current == QLatin1Char('\n')) state = QuoteScanState::Code;
            continue;
        }
        if (state == QuoteScanState::BlockComment) {
            if (current == QLatin1Char('*') and next == QLatin1Char('/')) {
                state = QuoteScanState::Code;
                ++index;
            }
            continue;
        }

        if (state != QuoteScanState::Code) {
            if (current == QLatin1Char('\\')) {
                if (index + 1 < limit) ++index;
                continue;
            }
            const bool closes =
                (state == QuoteScanState::DoubleQuote and current == QLatin1Char('"')) or
                (state == QuoteScanState::SingleQuote and current == QLatin1Char('\'')) or
                (state == QuoteScanState::Backtick and current == QLatin1Char('`'));
            if (closes) state = QuoteScanState::Code;
            continue;
        }

        if (current == QLatin1Char('/') and next == QLatin1Char('/')) {
            state = QuoteScanState::LineComment;
            ++index;
        } else if (current == QLatin1Char('/') and next == QLatin1Char('*')) {
            state = QuoteScanState::BlockComment;
            ++index;
        } else if (current == QLatin1Char('"')) {
            state = QuoteScanState::DoubleQuote;
        } else if (current == QLatin1Char('\'')) {
            state = QuoteScanState::SingleQuote;
        } else if (current == QLatin1Char('`')) {
            state = QuoteScanState::Backtick;
        }
    }

    if (state == QuoteScanState::DoubleQuote) return QLatin1Char('"');
    if (state == QuoteScanState::SingleQuote) return QLatin1Char('\'');
    if (state == QuoteScanState::Backtick) return QLatin1Char('`');
    return {};
}
}

QalamBracketHandler::QalamBracketHandler(QPlainTextEdit *editor) : m_editor(editor) {}

bool QalamBracketHandler::handleAutoPairing(QKeyEvent *e) {
    QString text = e->text();

    if (!text.isEmpty()) {
        QChar typedChar = text.at(0);

        // Handle opening brackets
        if (typedChar == '(' || typedChar == '[' || typedChar == '{') {
            QChar closingBracket;
            if (typedChar == '(') closingBracket = ')';
            else if (typedChar == '[') closingBracket = ']';
            else closingBracket = '}';

            return handleBracketCompletion(typedChar, closingBracket);
        }
        // Handle quotes
        else if (typedChar == '\'' || typedChar == '"' || typedChar == '`') {
                return handleQuoteCompletion(typedChar);
        }
        // Handle closing brackets (skip over existing ones)
        else if (typedChar == ')' || typedChar == ']' || typedChar == '}') {
            return handleBracketSkip(typedChar);
        }
    }

    return false;
}

bool QalamBracketHandler::handleBracketCompletion(QChar openingBracket, QChar closingBracket) {
    QTextCursor cursor = m_editor->textCursor();

    // Check if there's a selection
    if (cursor.hasSelection()) {
        // Wrap selection with brackets
        QString selectedText = cursor.selectedText();
        cursor.insertText(openingBracket + selectedText + closingBracket);

        // Use logical document order. Visual Left/Right reverses in an RTL
        // editor and can place the caret outside the inserted pair.
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::MoveAnchor,
                            selectedText.length() + 1);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor,
                            selectedText.length());
        m_editor->setTextCursor(cursor);
    } else {
        // Insert both brackets and place cursor between them
        cursor.insertText(QString(openingBracket) + closingBracket);
        cursor.movePosition(QTextCursor::PreviousCharacter);
        m_editor->setTextCursor(cursor);
    }

    return true;
}

bool QalamBracketHandler::handleQuoteCompletion(QChar quoteChar) {
    QTextCursor cursor = m_editor->textCursor();
    QTextDocument *doc = m_editor->document();

    // Get the character at cursor position
    int pos = cursor.position();

    // Check if there's a selection
    if (cursor.hasSelection()) {
        // Wrap selection with quotes
        QString selectedText = cursor.selectedText();
        cursor.insertText(quoteChar + selectedText + quoteChar);

        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::MoveAnchor,
                            selectedText.length() + 1);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor,
                            selectedText.length());
        m_editor->setTextCursor(cursor);
        return true;
    }

    // Check if next character is the same quote (should skip)
    if (pos < doc->characterCount() - 1) {
        QChar nextChar = doc->characterAt(pos);
        if (nextChar == quoteChar) {
            // Just move cursor over the existing quote
            cursor.movePosition(QTextCursor::NextCharacter);
            m_editor->setTextCursor(cursor);
            return true;
        }
    }

    // Inside an existing literal, the typed character is content or the one
    // closing delimiter. Insert only that character instead of creating a new
    // pair, which previously produced triple quotes at the end of Baa strings.
    if (!activeQuoteBefore(doc->toPlainText(), pos).isNull()) {
        cursor.insertText(QString(quoteChar));
        m_editor->setTextCursor(cursor);
        return true;
    }

    // Insert the quote pair
    cursor.insertText(QString(quoteChar) + quoteChar);
    cursor.movePosition(QTextCursor::PreviousCharacter);
    m_editor->setTextCursor(cursor);

    return true;
}

bool QalamBracketHandler::handleBracketSkip(QChar typedChar) {
    QTextCursor cursor = m_editor->textCursor();
    QTextDocument *doc = m_editor->document();
    int pos = cursor.position();

    // Check if the next character matches the typed closing bracket/quote
    if (pos < doc->characterCount() - 1) {
        QChar nextChar = doc->characterAt(pos);
        if (nextChar == typedChar) {
            // Just move the cursor over the existing bracket/quote
            cursor.movePosition(QTextCursor::NextCharacter);
            m_editor->setTextCursor(cursor);
            return true;
        }
    }

    return false;
}
