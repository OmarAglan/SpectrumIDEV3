#include "WorkspaceFileService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace {

QString pathKey(const QString &path)
{
    QString key = QDir::fromNativeSeparators(
        QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
#ifdef Q_OS_WIN
    key = key.toCaseFolded();
#endif
    return key;
}

bool isSameOrChildPath(const QString &rootPath, const QString &candidatePath)
{
    const QString rootKey = pathKey(rootPath);
    const QString candidateKey = pathKey(candidatePath);
    if (rootKey.isEmpty() or candidateKey.isEmpty()) return false;
    if (candidateKey == rootKey) return true;

    const QString prefix = rootKey.endsWith(QLatin1Char('/'))
        ? rootKey
        : rootKey + QLatin1Char('/');
    return candidateKey.startsWith(prefix);
}

WorkspaceFileResult failure(const QString &message)
{
    return {false, {}, message};
}

WorkspaceFileResult validateWorkspaceRoot(const QString &workspaceRoot)
{
    const QFileInfo rootInfo(workspaceRoot);
    const QString canonicalRoot = rootInfo.canonicalFilePath();
    if (canonicalRoot.isEmpty() or not rootInfo.isDir()) {
        return failure(QStringLiteral("مجلد المشروع غير صالح أو لم يعد موجوداً."));
    }
    return {true, QDir::cleanPath(canonicalRoot), {}};
}

WorkspaceFileResult validateExistingEntry(
    const QString &workspaceRoot,
    const QString &entryPath,
    bool allowWorkspaceRoot)
{
    const WorkspaceFileResult root = validateWorkspaceRoot(workspaceRoot);
    if (not root.success) return root;

    const QFileInfo entryInfo(entryPath);
    if (not entryInfo.exists()) {
        return failure(QStringLiteral("العنصر المحدد لم يعد موجوداً."));
    }
    if (entryInfo.isSymLink()) {
        return failure(QStringLiteral("لا يمكن تعديل الروابط الرمزية من مستكشف قلم."));
    }

    const QString canonicalEntry = entryInfo.canonicalFilePath();
    if (canonicalEntry.isEmpty() or
        not isSameOrChildPath(root.path, canonicalEntry)) {
        return failure(QStringLiteral("العنصر المحدد يقع خارج مجلد المشروع."));
    }
    if (not allowWorkspaceRoot and pathKey(root.path) == pathKey(canonicalEntry)) {
        return failure(QStringLiteral("لا يمكن إعادة تسمية مجلد المشروع أو حذفه."));
    }

    return {true, QDir::cleanPath(canonicalEntry), {}};
}

WorkspaceFileResult validateCreationTarget(
    const QString &workspaceRoot,
    const QString &directoryPath,
    const QString &name)
{
    QString nameError;
    if (not WorkspaceFileService::isValidEntryName(name, &nameError)) {
        return failure(nameError);
    }

    const WorkspaceFileResult directory = validateExistingEntry(
        workspaceRoot, directoryPath, true);
    if (not directory.success) return directory;
    if (not QFileInfo(directory.path).isDir()) {
        return failure(QStringLiteral("الموقع المحدد ليس مجلداً."));
    }

    const QString targetPath = QDir(directory.path).filePath(name);
    if (QFileInfo::exists(targetPath)) {
        return failure(QStringLiteral("يوجد عنصر بهذا الاسم بالفعل."));
    }
    return {true, QDir::cleanPath(targetPath), {}};
}

}

bool WorkspaceFileService::isValidEntryName(const QString &name, QString *error)
{
    auto reject = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };

    if (name.isEmpty()) {
        return reject(QStringLiteral("يجب إدخال اسم."));
    }
    if (name != name.trimmed()) {
        return reject(QStringLiteral("لا يمكن أن يبدأ الاسم أو ينتهي بمسافة."));
    }
    if (name == QStringLiteral(".") or name == QStringLiteral("..")) {
        return reject(QStringLiteral("هذا الاسم محجوز لنظام الملفات."));
    }
    if (name.endsWith(QLatin1Char('.'))) {
        return reject(QStringLiteral("لا يمكن أن ينتهي الاسم بنقطة."));
    }

    static const QRegularExpression invalidCharacters(
        QStringLiteral(R"([<>:"/\\|?*\x{0000}-\x{001F}])"));
    if (invalidCharacters.match(name).hasMatch()) {
        return reject(QStringLiteral("يحتوي الاسم على محارف غير مسموح بها."));
    }

    const QString baseName = name.section(QLatin1Char('.'), 0, 0).toUpper();
    static const QRegularExpression reservedName(
        QStringLiteral(R"(^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$)"));
    if (reservedName.match(baseName).hasMatch()) {
        return reject(QStringLiteral("هذا الاسم محجوز لنظام الملفات."));
    }

    if (error) error->clear();
    return true;
}

WorkspaceFileResult WorkspaceFileService::createFile(
    const QString &workspaceRoot,
    const QString &directoryPath,
    const QString &name)
{
    const WorkspaceFileResult target = validateCreationTarget(
        workspaceRoot, directoryPath, name);
    if (not target.success) return target;

    QFile file(target.path);
    if (not file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        return failure(QStringLiteral("تعذر إنشاء الملف: %1").arg(file.errorString()));
    }
    file.close();
    return target;
}

WorkspaceFileResult WorkspaceFileService::createDirectory(
    const QString &workspaceRoot,
    const QString &directoryPath,
    const QString &name)
{
    const WorkspaceFileResult target = validateCreationTarget(
        workspaceRoot, directoryPath, name);
    if (not target.success) return target;

    const QFileInfo targetInfo(target.path);
    if (not QDir(targetInfo.absolutePath()).mkdir(targetInfo.fileName())) {
        return failure(QStringLiteral("تعذر إنشاء المجلد."));
    }
    return target;
}

WorkspaceFileResult WorkspaceFileService::renameEntry(
    const QString &workspaceRoot,
    const QString &entryPath,
    const QString &newName)
{
    QString nameError;
    if (not isValidEntryName(newName, &nameError)) return failure(nameError);

    const WorkspaceFileResult entry = validateExistingEntry(
        workspaceRoot, entryPath, false);
    if (not entry.success) return entry;

    const QFileInfo entryInfo(entry.path);
    if (entryInfo.fileName() == newName) return entry;

    const QString targetPath = QDir(entryInfo.absolutePath()).filePath(newName);
    if (QFileInfo::exists(targetPath)) {
        return failure(QStringLiteral("يوجد عنصر بهذا الاسم بالفعل."));
    }
    if (not QDir(entryInfo.absolutePath()).rename(entryInfo.fileName(), newName)) {
        return failure(QStringLiteral("تعذرت إعادة تسمية العنصر."));
    }

    return {true, QDir::cleanPath(targetPath), {}};
}

WorkspaceFileResult WorkspaceFileService::removeEntry(
    const QString &workspaceRoot,
    const QString &entryPath)
{
    const WorkspaceFileResult entry = validateExistingEntry(
        workspaceRoot, entryPath, false);
    if (not entry.success) return entry;

    const QFileInfo entryInfo(entry.path);
    bool removed{};
    if (entryInfo.isDir()) {
        removed = QDir(entry.path).removeRecursively();
    } else {
        removed = QFile::remove(entry.path);
    }
    if (not removed) {
        return failure(QStringLiteral("تعذر حذف العنصر المحدد."));
    }

    return entry;
}
