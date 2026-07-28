#pragma once

#include <QString>

struct BaaWorkspaceSymbol
{
    QString name;
    QString detail;
    QString containerName;
    QString filePath;
    int kind{};
    int line{};
    int character{};
    int endLine{};
    int endCharacter{};

    bool isValid() const
    {
        return not name.isEmpty() and not filePath.isEmpty() and
               line >= 0 and character >= 0;
    }
};
