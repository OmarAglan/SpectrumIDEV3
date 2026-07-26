#pragma once

#include <QString>
#include <QVector>

struct BaaTextEdit
{
    int line{-1};
    int character{-1};
    int endLine{-1};
    int endCharacter{-1};
    QString newText;

    bool isValid() const
    {
        return line >= 0 and character >= 0 and endLine >= line and
               endCharacter >= 0;
    }
};

struct BaaDocumentEdit
{
    QString filePath;
    int version{-1};
    QVector<BaaTextEdit> edits;

    bool isValid() const
    {
        return not filePath.isEmpty() and not edits.isEmpty();
    }
};

struct BaaWorkspaceEdit
{
    QVector<BaaDocumentEdit> documents;

    bool isValid() const { return not documents.isEmpty(); }

    int editCount() const
    {
        int count = 0;
        for (const BaaDocumentEdit &document : documents)
            count += document.edits.size();
        return count;
    }
};

int baaUtf16TextOffset(const QString &text, int line, int character);

bool applyBaaTextEdits(const QString &originalText,
                       const QVector<BaaTextEdit> &edits,
                       QString *updatedText,
                       QVector<BaaTextEdit> *descendingEdits = nullptr,
                       QString *error = nullptr);
