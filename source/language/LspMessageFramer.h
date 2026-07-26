#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

class LspMessageFramer
{
public:
    static constexpr qsizetype MaximumHeaderBytes = 8 * 1024;
    static constexpr qsizetype MaximumContentBytes = 16 * 1024 * 1024;

    QList<QByteArray> appendData(const QByteArray &data, QString *errorMessage = nullptr);
    void clear();

    static QByteArray frame(const QByteArray &jsonBody);

private:
    QByteArray m_buffer;
};
