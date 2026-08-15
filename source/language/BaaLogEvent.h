#pragma once

#include <QJsonObject>
#include <QMetaType>
#include <QString>

struct BaaLogEvent
{
    qint64 sequence{};
    QString severity;
    QString component;
    QString event;
    QString message;
    QJsonObject data;

    int lspType() const;
    QString arabicSummary() const;
    QString formattedLine() const;
};

Q_DECLARE_METATYPE(BaaLogEvent)
