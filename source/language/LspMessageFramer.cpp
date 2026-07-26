#include "LspMessageFramer.h"

namespace {
void setError(QString *target, const QString &message)
{
    if (target) *target = message;
}
}

QList<QByteArray> LspMessageFramer::appendData(const QByteArray &data, QString *errorMessage)
{
    if (errorMessage) errorMessage->clear();
    m_buffer.append(data);

    QList<QByteArray> messages;
    while (not m_buffer.isEmpty()) {
        const qsizetype headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            if (m_buffer.size() > MaximumHeaderBytes) {
                setError(errorMessage, QStringLiteral("ترويسة خادم اللغة أكبر من الحد المسموح."));
                clear();
            }
            break;
        }
        if (headerEnd > MaximumHeaderBytes) {
            setError(errorMessage, QStringLiteral("ترويسة خادم اللغة أكبر من الحد المسموح."));
            clear();
            return messages;
        }

        qint64 contentLength = -1;
        for (QByteArray line : m_buffer.left(headerEnd).split('\n')) {
            if (line.endsWith('\r')) line.chop(1);
            const qsizetype separator = line.indexOf(':');
            if (separator <= 0) {
                setError(errorMessage, QStringLiteral("ترويسة خادم اللغة مشوهة."));
                clear();
                return messages;
            }
            if (line.left(separator).trimmed().toLower() != "content-length") continue;
            if (contentLength >= 0) {
                setError(errorMessage, QStringLiteral("ترويسة Content-Length مكررة."));
                clear();
                return messages;
            }

            bool ok = false;
            contentLength = line.mid(separator + 1).trimmed().toLongLong(&ok, 10);
            if (not ok or contentLength < 0 or contentLength > MaximumContentBytes) {
                setError(errorMessage, QStringLiteral("قيمة Content-Length غير صالحة."));
                clear();
                return messages;
            }
        }
        if (contentLength < 0) {
            setError(errorMessage, QStringLiteral("ترويسة Content-Length مفقودة."));
            clear();
            return messages;
        }

        const qsizetype bodyStart = headerEnd + 4;
        const qint64 totalSize = static_cast<qint64>(bodyStart) + contentLength;
        if (totalSize > m_buffer.size()) break;
        messages.push_back(m_buffer.mid(bodyStart, static_cast<qsizetype>(contentLength)));
        m_buffer.remove(0, static_cast<qsizetype>(totalSize));
    }
    return messages;
}

void LspMessageFramer::clear()
{
    m_buffer.clear();
}

QByteArray LspMessageFramer::frame(const QByteArray &jsonBody)
{
    return QByteArray("Content-Length: ") + QByteArray::number(jsonBody.size()) +
           QByteArray("\r\n\r\n") + jsonBody;
}
