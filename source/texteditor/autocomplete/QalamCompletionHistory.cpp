#include "QalamCompletionHistory.h"

#include "Constants.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <limits>

namespace {
constexpr int MaximumEntries = 256;

QString usageKey(const QString &context, const QString &stableKey)
{
    return context + QChar(0x001f) + stableKey;
}

QJsonObject readUsage(QSettings &settings)
{
    const QByteArray encoded = settings.value(
        Constants::SettingsKeyCompletionUsage).toByteArray();
    const QJsonDocument document = QJsonDocument::fromJson(encoded);
    return document.isObject() ? document.object() : QJsonObject{};
}

void writeUsage(QSettings &settings, const QJsonObject &usage)
{
    settings.setValue(Constants::SettingsKeyCompletionUsage,
                      QJsonDocument(usage).toJson(QJsonDocument::Compact));
    settings.sync();
}

QJsonObject itemUsage(const QJsonObject &usage, const CompletionItem &item)
{
    if (item.context.isEmpty() or item.stableKey.isEmpty()) return {};
    return usage.value(usageKey(item.context, item.stableKey)).toObject();
}
}

void QalamCompletionHistory::rank(std::vector<CompletionItem> &items,
                                  QSettings &settings)
{
    const QJsonObject usage = readUsage(settings);
    std::stable_sort(items.begin(), items.end(),
        [&usage](const CompletionItem &left, const CompletionItem &right) {
            if (left.serverSortText != right.serverSortText)
                return left.serverSortText < right.serverSortText;

            const QJsonObject leftUsage = itemUsage(usage, left);
            const QJsonObject rightUsage = itemUsage(usage, right);
            const int leftCount = leftUsage.value(QStringLiteral("count")).toInt();
            const int rightCount = rightUsage.value(QStringLiteral("count")).toInt();
            if (leftCount != rightCount) return leftCount > rightCount;

            const qint64 leftLast = leftUsage.value(
                QStringLiteral("last")).toVariant().toLongLong();
            const qint64 rightLast = rightUsage.value(
                QStringLiteral("last")).toVariant().toLongLong();
            if (leftLast != rightLast) return leftLast > rightLast;
            return QString::localeAwareCompare(left.label, right.label) < 0;
        });
}

void QalamCompletionHistory::record(QSettings &settings,
                                    const QString &context,
                                    const QString &stableKey,
                                    qint64 timestamp)
{
    if (context.isEmpty() or stableKey.isEmpty()) return;
    QJsonObject usage = readUsage(settings);
    const QString key = usageKey(context, stableKey);
    QJsonObject entry = usage.value(key).toObject();
    entry.insert(QStringLiteral("count"),
                 entry.value(QStringLiteral("count")).toInt() + 1);
    entry.insert(QStringLiteral("last"), timestamp > 0
        ? timestamp : QDateTime::currentMSecsSinceEpoch());
    usage.insert(key, entry);

    while (usage.size() > MaximumEntries) {
        QString oldestKey;
        qint64 oldest = std::numeric_limits<qint64>::max();
        for (auto it = usage.constBegin(); it != usage.constEnd(); ++it) {
            const qint64 last = it.value().toObject()
                .value(QStringLiteral("last")).toVariant().toLongLong();
            if (last < oldest) {
                oldest = last;
                oldestKey = it.key();
            }
        }
        if (oldestKey.isEmpty()) break;
        usage.remove(oldestKey);
    }
    writeUsage(settings, usage);
}

void QalamCompletionHistory::clear(QSettings &settings)
{
    settings.remove(Constants::SettingsKeyCompletionUsage);
    settings.sync();
}

int QalamCompletionHistory::selectionCount(QSettings &settings,
                                           const QString &context,
                                           const QString &stableKey)
{
    return readUsage(settings).value(usageKey(context, stableKey)).toObject()
        .value(QStringLiteral("count")).toInt();
}
