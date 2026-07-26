#include "WorkspaceIndexer.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QtGlobal>

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
    if (m_rootPath.isEmpty() || !QDir(m_rootPath).exists()) {
        emit indexUpdated();
        return;
    }

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

bool WorkspaceIndexer::isAllowedFile(const QString &filePath) const
{
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) return false;
    if (info.size() > 5 * 1024 * 1024) return false;

    const QString suffix = info.suffix().toLower();
    const QStringList allowed = {"baa", "baahd", "txt", "md", "json", "cmake", "cpp", "c", "h", "hpp"};
    return allowed.contains(suffix) || info.fileName().compare("CMakeLists.txt", Qt::CaseInsensitive) == 0;
}
