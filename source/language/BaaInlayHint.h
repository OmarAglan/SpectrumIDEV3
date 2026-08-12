#pragma once

#include <QString>

struct BaaInlayHint
{
    int line{-1};
    int character{-1};
    QString label;
    QString parameter;
    bool paddingRight{};
    bool complete{true};

    bool isValid() const
    {
        return line >= 0 and character >= 0 and not label.isEmpty() and
            not parameter.isEmpty();
    }
};
