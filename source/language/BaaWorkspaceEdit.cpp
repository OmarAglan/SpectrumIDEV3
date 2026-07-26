#include "BaaWorkspaceEdit.h"

#include <algorithm>

int baaUtf16TextOffset(const QString &text, int line, int character)
{
    if (line < 0 or character < 0) return -1;
    int lineStart = 0;
    for (int currentLine = 0; currentLine < line; ++currentLine) {
        const int newline = text.indexOf(u'\n', lineStart);
        if (newline < 0) return -1;
        lineStart = newline + 1;
    }
    int lineEnd = text.indexOf(u'\n', lineStart);
    if (lineEnd < 0) lineEnd = text.size();
    if (character > lineEnd - lineStart) return -1;
    return lineStart + character;
}

bool applyBaaTextEdits(const QString &originalText,
                       const QVector<BaaTextEdit> &edits,
                       QString *updatedText,
                       QVector<BaaTextEdit> *descendingEdits,
                       QString *error)
{
    if (error) error->clear();
    if (not updatedText) {
        if (error) *error = QStringLiteral("لم يحدد ناتج تعديل النص.");
        return false;
    }
    if (edits.isEmpty()) {
        if (error) *error = QStringLiteral("لا توجد تعديلات لتطبيقها.");
        return false;
    }

    QVector<BaaTextEdit> ordered = edits;
    for (const BaaTextEdit &edit : ordered) {
        if (not edit.isValid()) {
            if (error) *error = QStringLiteral("تحتوي الخطة على نطاق غير صالح.");
            return false;
        }
    }
    std::ranges::sort(
        ordered,
        [&](const BaaTextEdit &left, const BaaTextEdit &right) {
            return baaUtf16TextOffset(
                       originalText, left.line, left.character) >
                   baaUtf16TextOffset(
                       originalText, right.line, right.character);
        });

    QString result = originalText;
    int nextBoundary = originalText.size();
    for (const BaaTextEdit &edit : ordered) {
        const int start = baaUtf16TextOffset(
            originalText, edit.line, edit.character);
        const int end = baaUtf16TextOffset(
            originalText, edit.endLine, edit.endCharacter);
        if (start < 0 or end < start or end > nextBoundary) {
            if (error) {
                *error = QStringLiteral(
                    "تحتوي خطة إعادة التسمية على نطاق متداخل أو خارج الملف.");
            }
            return false;
        }
        result.replace(start, end - start, edit.newText);
        nextBoundary = start;
    }

    *updatedText = std::move(result);
    if (descendingEdits) *descendingEdits = std::move(ordered);
    return true;
}
