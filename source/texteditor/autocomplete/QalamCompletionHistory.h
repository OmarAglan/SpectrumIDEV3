#pragma once

#include "AutoComplete.h"

#include <QSettings>

#include <vector>

class QalamCompletionHistory
{
public:
    static void rank(std::vector<CompletionItem> &items, QSettings &settings);
    static void record(QSettings &settings,
                       const QString &context,
                       const QString &stableKey,
                       qint64 timestamp = 0);
    static void clear(QSettings &settings);
    static int selectionCount(QSettings &settings,
                              const QString &context,
                              const QString &stableKey);
};
