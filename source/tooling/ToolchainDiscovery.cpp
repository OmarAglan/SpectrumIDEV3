#include "ToolchainDiscovery.h"

#include "Constants.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QSet>

namespace {

QString effectiveApplicationDirectory(const QString &provided)
{
    return provided.isEmpty()
        ? QCoreApplication::applicationDirPath()
        : QDir::cleanPath(provided);
}

QString executableFromCandidate(const QString &candidate)
{
    const QString trimmed = candidate.trimmed();
    if (trimmed.isEmpty()) return QString();

    const QFileInfo direct(trimmed);
    if (direct.isExecutable()) return direct.absoluteFilePath();

    return QStandardPaths::findExecutable(trimmed);
}

QStringList pathNames(QalamToolKind kind)
{
#if defined(Q_OS_WIN)
    switch (kind) {
    case QalamToolKind::Baa: return {QStringLiteral("baa.exe"), QStringLiteral("baa")};
    case QalamToolKind::Takween:
        return {QStringLiteral("تكوين.exe"), QStringLiteral("takween.exe"),
                QStringLiteral("takween")};
    case QalamToolKind::Nazm:
        return {QStringLiteral("نظم.exe"), QStringLiteral("nazm.exe"),
                QStringLiteral("nazm")};
    }
#else
    switch (kind) {
    case QalamToolKind::Baa: return {QStringLiteral("baa")};
    case QalamToolKind::Takween:
        return {QStringLiteral("تكوين"), QStringLiteral("takween")};
    case QalamToolKind::Nazm:
        return {QStringLiteral("نظم"), QStringLiteral("nazm")};
    }
#endif
    return {};
}

QStringList portableCandidates(QalamToolKind kind, const QString &applicationDirectory)
{
    const QDir app(applicationDirectory);
#if defined(Q_OS_WIN)
    switch (kind) {
    case QalamToolKind::Baa:
        return {app.filePath(QStringLiteral("baa/baa.exe")),
                app.filePath(QStringLiteral("baa.exe"))};
    case QalamToolKind::Takween:
        return {app.filePath(QStringLiteral("takween/تكوين.exe")),
                app.filePath(QStringLiteral("takween/takween.exe")),
                app.filePath(QStringLiteral("تكوين.exe")),
                app.filePath(QStringLiteral("takween.exe"))};
    case QalamToolKind::Nazm:
        return {app.filePath(QStringLiteral("nazm/نظم.exe")),
                app.filePath(QStringLiteral("nazm/nazm.exe")),
                app.filePath(QStringLiteral("نظم.exe")),
                app.filePath(QStringLiteral("nazm.exe")),
                app.filePath(QStringLiteral("baa/نظم.exe"))};
    }
#else
    switch (kind) {
    case QalamToolKind::Baa:
        return {app.filePath(QStringLiteral("baa/baa")),
                app.filePath(QStringLiteral("baa"))};
    case QalamToolKind::Takween:
        return {app.filePath(QStringLiteral("takween/تكوين")),
                app.filePath(QStringLiteral("takween/takween")),
                app.filePath(QStringLiteral("تكوين")),
                app.filePath(QStringLiteral("takween"))};
    case QalamToolKind::Nazm:
        return {app.filePath(QStringLiteral("nazm/نظم")),
                app.filePath(QStringLiteral("nazm/nazm")),
                app.filePath(QStringLiteral("نظم")),
                app.filePath(QStringLiteral("nazm")),
                app.filePath(QStringLiteral("baa/نظم"))};
    }
#endif
    return {};
}

QalamToolResolution explicitResolution(QalamToolKind kind,
                                       QalamToolSource source,
                                       const QString &requested)
{
    const QString trimmed = requested.trimmed();
    const QString executable = executableFromCandidate(trimmed);
    return {kind, source, executable.isEmpty() ? trimmed : executable, trimmed};
}

QString normalizedPathEntry(const QString &entry)
{
    QString result = QDir::fromNativeSeparators(entry.trimmed());
    while (result.endsWith(QLatin1Char('/')) and result.size() > 1) result.chop(1);
    return result.toCaseFolded();
}

}

bool QalamToolResolution::isAvailable() const
{
    return not program.isEmpty() and QFileInfo(program).isExecutable();
}

QString QalamToolResolution::sourceLabel() const
{
    switch (source) {
    case QalamToolSource::Settings: return QStringLiteral("الإعدادات");
    case QalamToolSource::Environment: return QStringLiteral("متغيرات البيئة");
    case QalamToolSource::Path: return QStringLiteral("PATH");
    case QalamToolSource::Portable: return QStringLiteral("حزمة محمولة قديمة");
    case QalamToolSource::Missing: return QStringLiteral("غير موجود");
    }
    return QStringLiteral("غير موجود");
}

QString QalamToolResolution::toolLabel() const
{
    switch (kind) {
    case QalamToolKind::Baa: return QStringLiteral("باء");
    case QalamToolKind::Takween: return QStringLiteral("تكوين");
    case QalamToolKind::Nazm: return QStringLiteral("نظم");
    }
    return QString();
}

QString ToolchainDiscovery::settingsKey(QalamToolKind kind)
{
    switch (kind) {
    case QalamToolKind::Baa: return Constants::SettingsKeyCompilerPath;
    case QalamToolKind::Takween: return Constants::SettingsKeyTakweenPath;
    case QalamToolKind::Nazm: return Constants::SettingsKeyNazmPath;
    }
    return QString();
}

QString ToolchainDiscovery::environmentVariable(QalamToolKind kind)
{
    switch (kind) {
    case QalamToolKind::Baa: return QStringLiteral("QALAM_BAA_PATH");
    case QalamToolKind::Takween: return QStringLiteral("QALAM_TAKWEEN_PATH");
    case QalamToolKind::Nazm: return QStringLiteral("QALAM_NAZM_PATH");
    }
    return QString();
}

QalamToolResolution ToolchainDiscovery::resolve(
    QalamToolKind kind,
    const QString &applicationDirectory)
{
    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                       Constants::OrgName, Constants::AppName);
    const QString configured = settings.value(settingsKey(kind)).toString().trimmed();
    if (not configured.isEmpty()) {
        return explicitResolution(kind, QalamToolSource::Settings, configured);
    }

    const QString environment = qEnvironmentVariable(environmentVariable(kind).toUtf8().constData()).trimmed();
    if (not environment.isEmpty()) {
        return explicitResolution(kind, QalamToolSource::Environment, environment);
    }

    for (const QString &name : pathNames(kind)) {
        const QString found = QStandardPaths::findExecutable(name);
        if (not found.isEmpty()) {
            return {kind, QalamToolSource::Path, found, name};
        }
    }

    const QString appDirectory = effectiveApplicationDirectory(applicationDirectory);
    for (const QString &candidate : portableCandidates(kind, appDirectory)) {
        const QString found = executableFromCandidate(candidate);
        if (not found.isEmpty()) {
            return {kind, QalamToolSource::Portable, found, candidate};
        }
    }

    return {kind, QalamToolSource::Missing, QString(), QString()};
}

QList<QalamToolResolution> ToolchainDiscovery::resolveAll(
    const QString &applicationDirectory)
{
    return {
        resolve(QalamToolKind::Baa, applicationDirectory),
        resolve(QalamToolKind::Takween, applicationDirectory),
        resolve(QalamToolKind::Nazm, applicationDirectory)
    };
}

QProcessEnvironment ToolchainDiscovery::processEnvironment(
    const QString &applicationDirectory)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QList<QalamToolResolution> tools = resolveAll(applicationDirectory);

    QStringList entries;
    QSet<QString> seen;
    auto appendEntry = [&entries, &seen](const QString &entry) {
        if (entry.trimmed().isEmpty()) return;
        const QString normalized = normalizedPathEntry(entry);
        if (normalized.isEmpty() or seen.contains(normalized)) return;
        seen.insert(normalized);
        entries.push_back(QDir::toNativeSeparators(entry));
    };

    for (const QalamToolResolution &tool : tools) {
        if (tool.isAvailable()) appendEntry(QFileInfo(tool.program).absolutePath());
    }
    const QString existingPath = environment.value(QStringLiteral("PATH"));
    for (const QString &entry : existingPath.split(QDir::listSeparator(), Qt::SkipEmptyParts)) {
        appendEntry(entry);
    }
    environment.insert(QStringLiteral("PATH"), entries.join(QDir::listSeparator()));

    const QalamToolResolution baa = tools.at(0);
    const QalamToolResolution nazm = tools.at(2);
    if (nazm.isAvailable()) {
        environment.insert(QStringLiteral("BAA_NAZM"), nazm.program);
    }
    if (baa.isAvailable() and not environment.contains(QStringLiteral("BAA_HOME"))) {
        const QString baaHome = QFileInfo(baa.program).absolutePath();
        if (QFileInfo(QDir(baaHome).filePath(QStringLiteral("stdlib"))).isDir()) {
            environment.insert(QStringLiteral("BAA_HOME"), baaHome);
        }
    }
    return environment;
}
