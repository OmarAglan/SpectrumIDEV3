#pragma once

#include <QMetaType>
#include <QString>

struct BaaHover
{
    QString contentKind;
    QString contents;
    int startLine{-1};
    int startCharacter{-1};
    int endLine{-1};
    int endCharacter{-1};

    bool isValid() const
    {
        return !contents.trimmed().isEmpty() &&
               startLine >= 0 && startCharacter >= 0 &&
               endLine >= startLine && endCharacter >= 0;
    }
};

Q_DECLARE_METATYPE(BaaHover)
