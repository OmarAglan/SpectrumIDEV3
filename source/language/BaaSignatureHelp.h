#pragma once

#include <QMetaType>
#include <QString>
#include <QStringList>

struct BaaSignatureHelp
{
    QString label;
    QStringList parameters;
    int activeParameter{};

    bool isValid() const
    {
        return !label.trimmed().isEmpty();
    }
};

Q_DECLARE_METATYPE(BaaSignatureHelp)
