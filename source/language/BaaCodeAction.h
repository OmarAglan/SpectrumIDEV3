#pragma once

#include "BaaWorkspaceEdit.h"

#include <QString>

struct BaaCodeAction
{
    QString id;
    QString title;
    QString kind;
    bool preferred{};
    BaaWorkspaceEdit edit;

    bool isValid() const
    {
        return not title.isEmpty() and kind == QStringLiteral("quickfix") and
               edit.isValid();
    }
};
