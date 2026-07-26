#pragma once

#include <QString>

struct BaaLocation
{
    QString filePath;
    int line{-1};
    int character{-1};
    int endLine{-1};
    int endCharacter{-1};

    bool isValid() const
    {
        return not filePath.isEmpty() and line >= 0 and character >= 0 and
               endLine >= line and endCharacter >= 0;
    }
};
