#pragma once

#include <QList>
#include <QProcessEnvironment>
#include <QString>

enum class QalamToolKind {
    Baa,
    Takween,
    Nazm
};

enum class QalamToolSource {
    Settings,
    Environment,
    Path,
    Portable,
    Missing
};

struct QalamToolResolution {
    QalamToolKind kind{QalamToolKind::Baa};
    QalamToolSource source{QalamToolSource::Missing};
    QString program;
    QString requestedProgram;

    bool isAvailable() const;
    QString sourceLabel() const;
    QString toolLabel() const;
};

class ToolchainDiscovery {
public:
    static QalamToolResolution resolve(
        QalamToolKind kind,
        const QString &applicationDirectory = QString());
    static QList<QalamToolResolution> resolveAll(
        const QString &applicationDirectory = QString());
    static QProcessEnvironment processEnvironment(
        const QString &applicationDirectory = QString());

    static QString settingsKey(QalamToolKind kind);
    static QString environmentVariable(QalamToolKind kind);
};
