#include "QalamSnippetManager.h"

#include <QRegularExpression>
#include <QSet>
#include <QTextBlock>

#include <algorithm>

namespace {
struct ParsedPlaceholder
{
    int order{};
    int start{};
    int length{};
};
}

QalamSnippetManager::QalamSnippetManager(QPlainTextEdit *editor) : m_editor(editor) {}

void QalamSnippetManager::insertSnippet(const QString &snippet, QTextCursor &tc)
{
    m_targets.clear();

    static const QRegularExpression placeholderPattern(
        QStringLiteral(R"(\$\{([0-9]+)(?::([^}]*))?\}|\$([0-9]+))"));
    QString plainText;
    QVector<ParsedPlaceholder> placeholders;
    int sourceOffset = 0;
    QRegularExpressionMatchIterator matches = placeholderPattern.globalMatch(snippet);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        plainText += snippet.mid(sourceOffset, match.capturedStart() - sourceOffset);
        const int order = (match.captured(1).isEmpty()
                               ? match.captured(3)
                               : match.captured(1)).toInt();
        const QString initialText = match.captured(2);
        placeholders.push_back({order,
                                static_cast<int>(plainText.length()),
                                static_cast<int>(initialText.length())});
        plainText += initialText;
        sourceOffset = match.capturedEnd();
    }
    plainText += snippet.mid(sourceOffset);

    QString baseIndentation;
    const QString lineText = tc.block().text();
    for (const QChar character : lineText) {
        if (not character.isSpace()) break;
        baseIndentation += character;
    }

    QVector<int> offsetMap(plainText.length() + 1);
    QString indentedText;
    for (int index = 0; index < plainText.length(); ++index) {
        offsetMap[index] = indentedText.length();
        indentedText += plainText.at(index);
        if (plainText.at(index) == '\n' and index + 1 < plainText.length()) {
            indentedText += baseIndentation;
        }
    }
    offsetMap[plainText.length()] = indentedText.length();

    const int insertionStart = tc.selectionStart();
    tc.insertText(indentedText);
    m_editor->setTextCursor(tc);

    std::ranges::stable_sort(placeholders,
        [](const ParsedPlaceholder &left, const ParsedPlaceholder &right) {
            if (left.order == 0) return false;
            if (right.order == 0) return true;
            return left.order < right.order;
        });
    QSet<int> seenOrders;
    for (const ParsedPlaceholder &placeholder : placeholders) {
        if (seenOrders.contains(placeholder.order)) continue;
        seenOrders.insert(placeholder.order);
        QTextCursor target(m_editor->document());
        const int start = insertionStart + offsetMap.at(placeholder.start);
        const int end = insertionStart + offsetMap.at(
            placeholder.start + placeholder.length);
        target.setPosition(start);
        target.setPosition(end, QTextCursor::KeepAnchor);
        m_targets.push_back(target);
    }
    if (not m_targets.isEmpty()) {
        m_editor->setTextCursor(m_targets.takeFirst());
    }
}

bool QalamSnippetManager::processSnippetNavigation()
{
    if (m_targets.isEmpty()) return false;
    m_editor->setTextCursor(m_targets.takeFirst());
    return true;
}

bool QalamSnippetManager::hasActiveSnippet() const
{
    return not m_targets.isEmpty();
}
