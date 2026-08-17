#pragma once

#include <QString>

struct WorkspaceFileResult {
    bool success{};
    QString path;
    QString error;
};

class WorkspaceFileService {
public:
    static WorkspaceFileResult createFile(
        const QString &workspaceRoot,
        const QString &directoryPath,
        const QString &name);
    static WorkspaceFileResult createDirectory(
        const QString &workspaceRoot,
        const QString &directoryPath,
        const QString &name);
    static WorkspaceFileResult renameEntry(
        const QString &workspaceRoot,
        const QString &entryPath,
        const QString &newName);
    static WorkspaceFileResult removeEntry(
        const QString &workspaceRoot,
        const QString &entryPath);

    static bool isValidEntryName(const QString &name, QString *error = nullptr);
};
