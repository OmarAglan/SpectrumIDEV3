#pragma once

#include "QalamEditor.h"
#include "Constants.h"
#include <QTabWidget>

class FileManager : public QObject {
    Q_OBJECT

public:
    /// Return values for the save confirmation dialog
    enum class SaveAction {
        Cancel,   ///< User cancelled the operation
        Save,     ///< User chose to save
        Discard   ///< User chose to discard changes
    };

    explicit FileManager(QTabWidget *tabWidget, QWidget *parentWindow, QObject *parent = nullptr);

    /// Get the currently active editor from the tab widget
    QalamEditor *currentEditor() const;

    /// Show save confirmation dialog if the current document is modified
    SaveAction needSave();
    SaveAction needSave(QalamEditor *editor);

    /// Save the current or specified editor. Returns false if the save was cancelled or failed.
    bool saveEditor(QalamEditor *editor);
    bool saveEditorAs(QalamEditor *editor);

public slots:
    void newFile();
    void openFile(QString filePath);
    void saveFile();
    void saveFileAs();

signals:
    /// Emitted after any file operation that changes window state
    void fileStateChanged();
    /// Emitted when the set of open editors changes (tab added/removed)
    void openEditorsChanged();

private:
    QalamEditor *createEditor(const QString &filePath = QString());
    QString normalizePath(const QString &filePath) const;
    QString nextUntitledName() const;
    void removeBackupForPath(const QString &filePath) const;
    void addRecentFile(const QString &filePath);

    QTabWidget *m_tabWidget{};
    QWidget *m_parentWindow{};
};
