#include "WorkspaceIndexer.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QStringConverter>
#include <QTextStream>
#include <QtGlobal>

#include <algorithm>

WorkspaceIndexer::WorkspaceIndexer(QObject *parent)
    : QObject(parent)
{
}

void WorkspaceIndexer::setRootPath(const QString &rootPath)
{
    const QString clean = QDir::cleanPath(rootPath);
    if (m_rootPath == clean) return;
    m_rootPath = clean;
    refresh();
}

QString WorkspaceIndexer::rootPath() const
{
    return m_rootPath;
}

void WorkspaceIndexer::refresh()
{
    m_files.clear();
    m_ignoreRules.clear();
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        emit indexUpdated();
        return;
    }

    loadIgnoreRules();

    QDirIterator it(m_rootPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = QDir::cleanPath(it.next());
        if (isIgnoredPath(path)) continue;
        if (!isAllowedFile(path)) continue;
        m_files << path;
    }
    m_files.sort(Qt::CaseInsensitive);
    emit indexUpdated();
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

void WorkspaceIndexer::loadIgnoreRules()
{
    QStringList ignoreFiles;
    QDirIterator iterator(
        m_rootPath,
        QStringList{QStringLiteral(".gitignore")},
        QDir::Files | QDir::Hidden | QDir::NoSymLinks,
        QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString path = QDir::cleanPath(iterator.next());
        if (not isAlwaysIgnoredPath(path)) ignoreFiles.push_back(path);
    }
    std::sort(ignoreFiles.begin(), ignoreFiles.end(),
              [](const QString &left, const QString &right) {
                  const int leftDepth = left.count(QDir::separator());
                  const int rightDepth = right.count(QDir::separator());
                  return leftDepth == rightDepth
                      ? left < right
                      : leftDepth < rightDepth;
              });

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
            if (not compiled.isValid()) continue;
            m_ignoreRules.push_back({basePath, compiled, negated});
        }
    }
}

bool WorkspaceIndexer::isAlwaysIgnoredPath(const QString &filePath) const
{
    const QString normalized = QDir::fromNativeSeparators(QDir::cleanPath(filePath));
    const QStringList ignored = {
        "/.git/", "/.hg/", "/.svn/", "/build/", "/dist/", "/out/",
        "/node_modules/", "/.cache/", "/CMakeFiles/", "/.vs/"
    };
    for (const QString &segment : ignored) {
        if (normalized.contains(segment, Qt::CaseInsensitive)) return true;
    }
    return false;
}

bool WorkspaceIndexer::isIgnoredByWorkspaceRules(
    const QString &filePath) const
{
    bool ignored = false;
    for (const IgnoreRule &rule : m_ignoreRules) {
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

bool WorkspaceIndexer::isAllowedFile(const QString &filePath) const
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) return false;
    if (info.size() > 5 * 1024 * 1024) return false;

    const QString suffix = info.suffix().toLower();
    const QStringList allowed = {"baa", "baahd", "txt", "md", "json", "cmake", "cpp", "c", "h", "hpp"};
    return allowed.contains(suffix) || info.fileName().compare("CMakeLists.txt", Qt::CaseInsensitive) == 0;
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
