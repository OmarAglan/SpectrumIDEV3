#pragma once

#include <QString>

struct BaaFoldingRange
{
    int startLine{-1};
    int startCharacter{-1};
    int endLine{-1};
    int endCharacter{-1};
    QString kind;

    bool isValid() const
    {
        return startLine >= 0 and startCharacter >= 0 and
               endLine > startLine and endCharacter >= 0;
    }
};
