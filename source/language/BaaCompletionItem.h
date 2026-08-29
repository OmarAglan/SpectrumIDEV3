#pragma once

#include <QMetaType>
#include <QString>

struct BaaCompletionItem
{
    QString label;
    QString detail;
    QString filterText;
    QString newText;
    QString sortText;
    QString context;
    QString stableKey;
    int kind{};
    int insertTextFormat{1};
    int startLine{};
    int startCharacter{};
    int endLine{};
    int endCharacter{};

    bool isValid() const
    {
        return not label.isEmpty() and not newText.isEmpty() and
               startLine >= 0 and startCharacter >= 0 and
               endLine >= startLine and endCharacter >= 0;
    }
};

Q_DECLARE_METATYPE(BaaCompletionItem)
