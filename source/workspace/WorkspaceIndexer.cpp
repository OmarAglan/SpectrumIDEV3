#include "WorkspaceIndexer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QSet>
#include <QStringConverter>
#include <QTextStream>

#include <algorithm>

namespace {

constexpr qint64 MaximumIndexedFileSize = 5 * 1024 * 1024;

QString normalizedAbsolutePath(const QString &filePath)
{
    return QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
}

QString watchPathKey(const QString &filePath)
{
    QString key = QDir::fromNativeSeparators(
        normalizedAbsolutePath(filePath));
#ifdef Q_OS_WIN
    key = key.toCaseFolded();
#endif
    return key;
}

bool isPathInsideRoot(const QString &rootPath, const QString &filePath)
{
    const QString relative = QDir::fromNativeSeparators(
        QDir(rootPath).relativeFilePath(filePath));
    return not QDir::isAbsolutePath(relative) and
        relative != QStringLiteral("..") and
        not relative.startsWith(QStringLiteral("../"));
}

bool isAlwaysIgnored(const QString &rootPath, const QString &filePath)
{
    if (rootPath.isEmpty() or not isPathInsideRoot(rootPath, filePath))
        return true;

    const QString relative = QDir::fromNativeSeparators(
        QDir(rootPath).relativeFilePath(filePath));
    const QStringList parts = relative.split(
        QLatin1Char('/'), Qt::SkipEmptyParts);
    const QStringList ignoredDirectories = {
        QStringLiteral(".git"), QStringLiteral(".hg"),
        QStringLiteral(".svn"), QStringLiteral("build"),
        QStringLiteral("dist"), QStringLiteral("out"),
        QStringLiteral("node_modules"), QStringLiteral(".cache"),
        QStringLiteral("CMakeFiles"), QStringLiteral(".vs")
    };
    for (const QString &part : parts) {
        for (const QString &ignored : ignoredDirectories) {
            if (part.compare(ignored, Qt::CaseInsensitive) == 0)
                return true;
        }
    }
    return false;
}

bool hasIndexedExtension(const QString &filePath)
{
    const QFileInfo info(filePath);
    const QString suffix = info.suffix().toLower();
    const QStringList allowed = {
        QStringLiteral("baa"), QStringLiteral("baahd"),
        QStringLiteral("txt"), QStringLiteral("md"),
        QStringLiteral("json"), QStringLiteral("cmake"),
        QStringLiteral("cpp"), QStringLiteral("c"),
        QStringLiteral("h"), QStringLiteral("hpp")
    };
    return allowed.contains(suffix) or
        info.fileName().compare(
            QStringLiteral("CMakeLists.txt"), Qt::CaseInsensitive) == 0;
}

}

WorkspaceIndexer::WorkspaceIndexer(QObject *parent)
    : QObject(parent)
{
    m_threadPool.setMaxThreadCount(1);
    m_threadPool.setExpiryTimeout(-1);

    m_refreshDebounce.setSingleShot(true);
    m_refreshDebounce.setInterval(160);
    connect(&m_refreshDebounce, &QTimer::timeout,
            this, &WorkspaceIndexer::refresh);

    m_fallbackRefreshTimer.setInterval(3000);
    connect(&m_fallbackRefreshTimer, &QTimer::timeout, this, [this]() {
        if (not m_indexing) refresh();
    });

    auto fileSystemChanged = [this](const QString &) {
        if (not m_updatingWatchers) scheduleRefresh();
    };
    connect(&m_fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, fileSystemChanged);
    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, fileSystemChanged);
}

WorkspaceIndexer::~WorkspaceIndexer()
{
    ++m_generation;
    m_threadPool.clear();
    m_threadPool.waitForDone();
}

void WorkspaceIndexer::setRootPath(const QString &rootPath)
{
    const QString clean = rootPath.isEmpty()
        ? QString()
        : normalizedAbsolutePath(rootPath);
    if (m_rootPath == clean) return;

    ++m_generation;
    m_threadPool.clear();
    m_rootPath = clean;
    m_files.clear();
    m_ignoreRules.clear();
    m_indexing = false;
    m_refreshPending = false;
    m_refreshDebounce.stop();
    m_fallbackRefreshTimer.stop();
    clearWatchers();
    refresh();
}

QString WorkspaceIndexer::rootPath() const
{
    return m_rootPath;
}

void WorkspaceIndexer::refresh()
{
    if (m_indexing) {
        m_refreshPending = true;
        return;
    }
    startRefresh();
}

bool WorkspaceIndexer::isIndexing() const
{
    return m_indexing;
}

void WorkspaceIndexer::startRefresh()
{
    if (m_rootPath.isEmpty() or not QDir(m_rootPath).exists()) {
        ++m_generation;
        m_files.clear();
        m_ignoreRules.clear();
        clearWatchers();
        if (not m_rootPath.isEmpty()) m_fallbackRefreshTimer.start();
        emit indexUpdated();
        return;
    }

    const int requestId = ++m_generation;
    const QString rootPath = m_rootPath;
    m_indexing = true;
    m_threadPool.start([this, rootPath, requestId]() {
        const IndexSnapshot snapshot = buildSnapshot(
            rootPath, requestId, m_generation);
        if (m_generation.load() != requestId) return;
        QMetaObject::invokeMethod(
            this,
            [this, requestId, snapshot]() {
                if (m_generation.load() == requestId)
                    applySnapshot(requestId, snapshot);
            },
            Qt::QueuedConnection);
    });
}

void WorkspaceIndexer::scheduleRefresh()
{
    if (not m_rootPath.isEmpty()) m_refreshDebounce.start();
}

void WorkspaceIndexer::applySnapshot(
    int requestId, const IndexSnapshot &snapshot)
{
    if (m_generation.load() != requestId) return;
    m_indexing = false;
    m_files = snapshot.files;
    m_ignoreRules = snapshot.ignoreRules;
    configureWatchers(snapshot);
    emit indexUpdated();

    if (m_refreshPending) {
        m_refreshPending = false;
        QTimer::singleShot(0, this, &WorkspaceIndexer::refresh);
    }
}

void WorkspaceIndexer::configureWatchers(const IndexSnapshot &snapshot)
{
    m_updatingWatchers = true;

    QStringList candidates;
    if (QDir(m_rootPath).exists()) candidates.push_back(m_rootPath);
    candidates.append(snapshot.ignoreFiles);
    candidates.append(snapshot.watchDirectories);
    candidates.append(snapshot.watchFiles);

    QStringList requested;
    requested.reserve(qMin(candidates.size(), MaximumNativeWatchPaths));
    QSet<QString> seen;
    bool truncated = false;
    for (const QString &candidate : candidates) {
        if (not QFileInfo::exists(candidate)) continue;
        const QString key = watchPathKey(candidate);
        if (seen.contains(key)) continue;
        seen.insert(key);
        if (requested.size() >= MaximumNativeWatchPaths) {
            truncated = true;
            continue;
        }
        requested.push_back(candidate);
    }

    const QStringList current =
        m_fileWatcher.directories() + m_fileWatcher.files();
    QSet<QString> requestedKeys;
    for (const QString &path : requested)
        requestedKeys.insert(watchPathKey(path));

    QStringList obsolete;
    QSet<QString> currentKeys;
    for (const QString &path : current) {
        const QString key = watchPathKey(path);
        if (requestedKeys.contains(key)) {
            currentKeys.insert(key);
        } else {
            obsolete.push_back(path);
        }
    }
    if (not obsolete.isEmpty()) m_fileWatcher.removePaths(obsolete);

    QStringList additions;
    for (const QString &path : requested) {
        if (not currentKeys.contains(watchPathKey(path)))
            additions.push_back(path);
    }
    const QStringList rejected = additions.isEmpty()
        ? QStringList{}
        : m_fileWatcher.addPaths(additions);
    const bool degraded = truncated or not rejected.isEmpty();
    m_updatingWatchers = false;

    if (degraded) {
        m_fallbackRefreshTimer.start();
    } else {
        m_fallbackRefreshTimer.stop();
    }
}

void WorkspaceIndexer::clearWatchers()
{
    const bool wasUpdating = m_updatingWatchers;
    m_updatingWatchers = true;
    const QStringList files = m_fileWatcher.files();
    const QStringList directories = m_fileWatcher.directories();
    if (not files.isEmpty()) m_fileWatcher.removePaths(files);
    if (not directories.isEmpty()) m_fileWatcher.removePaths(directories);
    m_updatingWatchers = wasUpdating;
}

WorkspaceIndexer::IndexSnapshot WorkspaceIndexer::buildSnapshot(
    const QString &rootPath,
    int requestId,
    const std::atomic_int &generation)
{
    IndexSnapshot snapshot;
    QStringList candidateFiles;
    QStringList pendingDirectories{rootPath};

    while (not pendingDirectories.isEmpty()) {
        if (generation.load() != requestId) return {};
        const QString directoryPath = pendingDirectories.takeLast();
        snapshot.watchDirectories.push_back(directoryPath);

        const QFileInfoList entries = QDir(directoryPath).entryInfoList(
            QDir::Dirs | QDir::Files | QDir::Hidden | QDir::System |
                QDir::NoDotAndDotDot | QDir::NoSymLinks,
            QDir::NoSort);
        for (const QFileInfo &entry : entries) {
            if (generation.load() != requestId) return {};
            const QString path = QDir::cleanPath(entry.absoluteFilePath());
            if (entry.isSymLink()) continue;
            if (entry.isDir()) {
                if (not isAlwaysIgnored(rootPath, path))
                    pendingDirectories.push_back(path);
                continue;
            }
            if (not entry.isFile()) continue;
            if (entry.fileName() == QStringLiteral(".gitignore"))
                snapshot.ignoreFiles.push_back(path);
            if (hasIndexedExtension(path)) candidateFiles.push_back(path);
        }
    }

    snapshot.ignoreRules = parseIgnoreRules(
        rootPath, snapshot.ignoreFiles);
    for (const QString &filePath : candidateFiles) {
        if (generation.load() != requestId) return {};
        const QFileInfo info(filePath);
        if (not info.exists() or not info.isFile() or
            isIgnoredByRules(filePath, snapshot.ignoreRules))
            continue;
        snapshot.watchFiles.push_back(filePath);
        if (info.size() > MaximumIndexedFileSize) continue;
        snapshot.files.push_back(filePath);
    }
    snapshot.files.sort(Qt::CaseInsensitive);
    snapshot.ignoreFiles.sort(Qt::CaseInsensitive);
    snapshot.watchDirectories.sort(Qt::CaseInsensitive);
    snapshot.watchFiles.sort(Qt::CaseInsensitive);
    return snapshot;
}

QVector<WorkspaceIndexer::IgnoreRule> WorkspaceIndexer::parseIgnoreRules(
    const QString &rootPath, QStringList ignoreFiles)
{
    std::sort(ignoreFiles.begin(), ignoreFiles.end(),
              [&rootPath](const QString &left, const QString &right) {
                  const QString leftRelative = QDir::fromNativeSeparators(
                      QDir(rootPath).relativeFilePath(left));
                  const QString rightRelative = QDir::fromNativeSeparators(
                      QDir(rootPath).relativeFilePath(right));
                  const int leftDepth = leftRelative.count(QLatin1Char('/'));
                  const int rightDepth = rightRelative.count(QLatin1Char('/'));
                  return leftDepth == rightDepth
                      ? leftRelative < rightRelative
                      : leftDepth < rightDepth;
              });

    QVector<IgnoreRule> rules;
    for (const QString &ignoreFile : ignoreFiles) {
        QFile file(ignoreFile);
        if (not file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        const QString basePath = QDir::cleanPath(
            QFileInfo(ignoreFile).absolutePath());

        while (not stream.atEnd()) {
            QString pattern = stream.readLine();
            if (pattern.endsWith(QLatin1Char('\r'))) pattern.chop(1);
            if (pattern.isEmpty()) continue;
            if (pattern.startsWith(QStringLiteral("\\#"))) {
                pattern.remove(0, 1);
            } else if (pattern.startsWith(QLatin1Char('#'))) {
                continue;
            }

            bool negated = false;
            if (pattern.startsWith(QStringLiteral("\\!"))) {
                pattern.remove(0, 1);
            } else if (pattern.startsWith(QLatin1Char('!'))) {
                negated = true;
                pattern.remove(0, 1);
            }
            if (pattern.isEmpty()) continue;

            while (pattern.endsWith(QLatin1Char(' ')) and
                   not pattern.endsWith(QStringLiteral("\\ ")))
                pattern.chop(1);
            pattern.replace(QStringLiteral("\\ "), QStringLiteral(" "));

            if (pattern.endsWith(QLatin1Char('/'))) pattern.chop(1);
            const bool anchored = pattern.startsWith(QLatin1Char('/'));
            if (anchored) pattern.remove(0, 1);
            if (pattern.isEmpty()) continue;

            const bool containsSlash = pattern.contains(QLatin1Char('/'));
            const QString body = globToRegularExpression(pattern);
            const QString expression = anchored or containsSlash
                ? QStringLiteral("^(?:%1)(?:/.*)?$").arg(body)
                : QStringLiteral("^(?:.*/)?(?:%1)(?:/.*)?$").arg(body);
            QRegularExpression compiled(
                expression,
                QRegularExpression::UseUnicodePropertiesOption);
            if (compiled.isValid())
                rules.push_back({basePath, compiled, negated});
        }
    }
    return rules;
}

bool WorkspaceIndexer::isIgnoredByRules(
    const QString &filePath,
    const QVector<IgnoreRule> &rules)
{
    bool ignored = false;
    for (const IgnoreRule &rule : rules) {
        const QString relative = QDir::fromNativeSeparators(
            QDir(rule.basePath).relativeFilePath(filePath));
        if (relative == QStringLiteral("..") or
            relative.startsWith(QStringLiteral("../")))
            continue;
        if (rule.expression.match(relative).hasMatch())
            ignored = not rule.negated;
    }
    return ignored;
}

QStringList WorkspaceIndexer::files() const
{
    return m_files;
}

QStringList WorkspaceIndexer::quickOpenFiles() const
{
    return m_files;
}

bool WorkspaceIndexer::isIgnoredPath(const QString &filePath) const
{
    return isAlwaysIgnoredPath(filePath) or
        isIgnoredByWorkspaceRules(filePath);
}

bool WorkspaceIndexer::isAlwaysIgnoredPath(const QString &filePath) const
{
    return isAlwaysIgnored(m_rootPath, filePath);
}

bool WorkspaceIndexer::isIgnoredByWorkspaceRules(
    const QString &filePath) const
{
    return isIgnoredByRules(filePath, m_ignoreRules);
}

QString WorkspaceIndexer::globToRegularExpression(const QString &pattern)
{
    QString expression;
    for (qsizetype index = 0; index < pattern.size(); ++index) {
        const QChar character = pattern.at(index);
        if (character == QLatin1Char('*')) {
            if (index + 1 < pattern.size() and
                pattern.at(index + 1) == QLatin1Char('*')) {
                ++index;
                if (index + 1 < pattern.size() and
                    pattern.at(index + 1) == QLatin1Char('/')) {
                    ++index;
                    expression += QStringLiteral("(?:.*/)?");
                } else {
                    expression += QStringLiteral(".*");
                }
            } else {
                expression += QStringLiteral("[^/]*");
            }
            continue;
        }
        if (character == QLatin1Char('?')) {
            expression += QStringLiteral("[^/]");
            continue;
        }
        if (character == QLatin1Char('\\') and
            index + 1 < pattern.size()) {
            expression += QRegularExpression::escape(
                QString(pattern.at(++index)));
            continue;
        }
        expression += QRegularExpression::escape(QString(character));
    }
    return expression;
}
