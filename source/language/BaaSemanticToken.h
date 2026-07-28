#pragma once

#include <QString>

struct BaaSemanticToken
{
    int line{-1};
    int character{-1};
    int length{};
    QString type;
    int modifiers{};

    bool isValid() const
    {
        return line >= 0 and character >= 0 and length > 0 and
               not type.isEmpty() and modifiers >= 0;
    }
};
