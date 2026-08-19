#include "FileManager.h"

#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <QStringConverter>
#include <QSettings>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>
#include <QTextDocument>
#include <QPushButton>
#include <QAbstractButton>
#include <QSet>

FileManager::FileManager(QTabWidget *tabWidget, QWidget *parentWindow, QObject *parent)
    : QObject(parent)
    , m_tabWidget(tabWidget)
    , m_parentWindow(parentWindow)
{
}

QalamEditor *FileManager::currentEditor() const
{
    return qobject_cast<QalamEditor*>(m_tabWidget->currentWidget());
}

QString FileManager::normalizePath(const QString &filePath) const
{
    if (filePath.trimmed().isEmpty()) return QString();

    QFileInfo info(filePath);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) return QDir::cleanPath(canonical);

    return QDir::cleanPath(info.absoluteFilePath());
}

QString FileManager::nextUntitledName() const
{
    QSet<QString> existingNames;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        existingNames.insert(m_tabWidget->tabText(i).remove("[*]"));
    }

    if (!existingNames.contains(Constants::NewFileLabel)) {
        return Constants::NewFileLabel;
    }

    int counter = 2;
    while (existingNames.contains(QString("%1 %2").arg(Constants::NewFileLabel).arg(counter))) {
        ++counter;
    }
    return QString("%1 %2").arg(Constants::NewFileLabel).arg(counter);
}

void FileManager::removeBackupForPath(const QString &filePath) const
{
    const QString normalizedPath = normalizePath(filePath);
    if (normalizedPath.isEmpty()) return;

    const QString backupPath = normalizedPath + Constants::BackupExtension;
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
}

QalamEditor *FileManager::createEditor(const QString &filePath)
{
    auto *editor = new QalamEditor(m_parentWindow);
    editor->setFilePath(filePath);

    connect(editor, &QalamEditor::openRequest, this, [this](const QString &requestedPath) {
        openFile(requestedPath);
    });
    connect(editor->document(), &QTextDocument::modificationChanged, this, [this]() {
        emit fileStateChanged();
        emit openEditorsChanged();
    });

    return editor;
}

void FileManager::addRecentFile(const QString &filePath)
{
    const QString normalizedPath = normalizePath(filePath);
    if (normalizedPath.isEmpty()) return;

    QSettings settings(Constants::OrgName, Constants::AppName);
    QStringList recentFiles = settings.value(Constants::SettingsKeyRecentFiles).toStringList();
    recentFiles.removeAll(normalizedPath);
    recentFiles.prepend(normalizedPath);
    while (recentFiles.size() > 10) {
        recentFiles.removeLast();
    }
    settings.setValue(Constants::SettingsKeyRecentFiles, recentFiles);
}

FileManager::SaveAction FileManager::needSave()
{
    return needSave(currentEditor());
}

FileManager::SaveAction FileManager::needSave(QalamEditor *editor)
{
    if (editor and editor->document()->isModified()) {
        QString displayName;
        if (editor->currentFilePath().isEmpty()) {
            const int index = m_tabWidget->indexOf(editor);
            displayName = index >= 0 ? m_tabWidget->tabText(index) : Constants::NewFileLabel;
            if (displayName.endsWith("[*]")) {
                displayName.chop(3);
            }
        } else {
            displayName = QFileInfo(editor->currentFilePath()).fileName();
        }

        QMessageBox msgBox(m_parentWindow);
        msgBox.setWindowTitle("قلم");
        msgBox.setText(QString("تم تعديل الملف:\n%1\n\nهل تريد حفظ التغييرات؟").arg(displayName));
        QPushButton *saveButton = msgBox.addButton("حفظ", QMessageBox::AcceptRole);
        QPushButton *discardButton = msgBox.addButton("تجاهل", QMessageBox::DestructiveRole);
        QPushButton *cancelButton = msgBox.addButton("إلغاء", QMessageBox::RejectRole);
        msgBox.setDefaultButton(cancelButton);

        QFont msgFont = m_parentWindow ? m_parentWindow->font() : msgBox.font();
        msgFont.setPointSize(10);
        saveButton->setFont(msgFont);
        discardButton->setFont(msgFont);
        cancelButton->setFont(msgFont);

        msgBox.exec();

        QAbstractButton *clickedButton = msgBox.clickedButton();
        if (clickedButton == saveButton) {
            return SaveAction::Save;
        }
        if (clickedButton == discardButton) {
            return SaveAction::Discard;
        }
        return SaveAction::Cancel;
    }

    return SaveAction::Discard;
}

void FileManager::newFile()
{
    QalamEditor *newEditor = createEditor();
    const int index = m_tabWidget->addTab(newEditor, nextUntitledName());
    m_tabWidget->setCurrentIndex(index);

    emit fileStateChanged();
    emit openEditorsChanged();
}

void FileManager::openFile(QString filePath)
{
    if (filePath.isEmpty()) {
        filePath = QFileDialog::getOpenFileName(m_parentWindow, "فتح ملف", "",
                                                 "مصادر باء ونظم (*.baa *.baahd *.نظم);;ملف باء (*.baa *.baahd);;ملف نظم (*.نظم);;Text Files (*.txt);;All Files (*)");
    }

    if (filePath.isEmpty()) return;

    const QString normalizedPath = normalizePath(filePath);
    if (normalizedPath.isEmpty()) return;

    // Check if file is already open in a tab
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(m_tabWidget->widget(i));
        if (editor and normalizePath(editor->currentFilePath()) == normalizedPath) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }

    // File size safety check
    constexpr qint64 WarnThreshold = 10 * 1024 * 1024;   // 10 MB
    constexpr qint64 RejectThreshold = 50 * 1024 * 1024;  // 50 MB
    QFileInfo fileCheck(normalizedPath);
    qint64 fileSize = fileCheck.size();

    if (fileSize > RejectThreshold) {
        QMessageBox::warning(m_parentWindow, "ملف كبير جداً",
            QString("حجم الملف (%1 MB) يتجاوز الحد المسموح (50 MB).\n"
                    "لا يمكن فتح هذا الملف.")
                .arg(fileSize / (1024 * 1024)));
        return;
    }

    if (fileSize > WarnThreshold) {
        auto reply = QMessageBox::question(m_parentWindow, "ملف كبير",
            QString("حجم الملف (%1 MB) كبير وقد يؤثر على الأداء.\n"
                    "هل تريد المتابعة؟")
                .arg(fileSize / (1024 * 1024)),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
    }

    QFile file(normalizedPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(m_parentWindow, "خطأ",
                             "لا يمكن فتح الملف:\n" + file.errorString());
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    QalamEditor *newEditor = createEditor(normalizedPath);
    newEditor->setPlainText(content);
    newEditor->document()->setModified(false);

    // Check for backup recovery. Only prompt when the backup is newer than the file.
    const QString backupPath = normalizedPath + Constants::BackupExtension;
    if (QFile::exists(backupPath)) {
        const QFileInfo sourceInfo(normalizedPath);
        const QFileInfo backupInfo(backupPath);
        const bool backupIsNewer = backupInfo.lastModified() > sourceInfo.lastModified();

        if (backupIsNewer) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(m_parentWindow, "استعادة ملف",
                                         "يبدو أن البرنامج أُغلق بشكل غير متوقع.\n"
                                         "يوجد نسخة محفوظة تلقائيًا أحدث من الملف الأصلي.\n\n"
                                         "هل تريد استعادتها؟",
                                         QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                QFile backup(backupPath);
                if (backup.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream backupIn(&backup);
                    backupIn.setEncoding(QStringConverter::Utf8);
                    newEditor->setPlainText(backupIn.readAll());
                    newEditor->document()->setModified(true);
                    backup.close();
                }
            } else {
                QFile::remove(backupPath);
            }
        } else {
            QFile::remove(backupPath);
        }
    }

    QFileInfo fileInfo(normalizedPath);
    const int index = m_tabWidget->addTab(newEditor, fileInfo.fileName());
    m_tabWidget->setCurrentIndex(index);
    m_tabWidget->setTabToolTip(index, normalizedPath);

    addRecentFile(normalizedPath);

    emit fileStateChanged();
    emit openEditorsChanged();
}

bool FileManager::saveEditor(QalamEditor *editor)
{
    if (!editor) return false;

    QString filePath = editor->currentFilePath();
    QString content = editor->toPlainText();

    if (filePath.isEmpty()) {
        return saveEditorAs(editor);
    }

    filePath = normalizePath(filePath);
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(m_parentWindow, "خطأ",
                             "لا يمكن حفظ الملف:\n" + file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;

    if (!file.commit()) {
        QMessageBox::warning(m_parentWindow, "خطأ",
                             "فشل إكمال حفظ الملف:\n" + file.errorString());
        return false;
    }

    editor->setFilePath(filePath);
    editor->document()->setModified(false);

    int index = m_tabWidget->indexOf(editor);
    if (index != -1) {
        QFileInfo fileInfo(filePath);
        m_tabWidget->setTabText(index, fileInfo.fileName());
        m_tabWidget->setTabToolTip(index, filePath);
    }

    editor->removeBackupFile();
    addRecentFile(filePath);
    emit fileStateChanged();
    emit openEditorsChanged();
    return true;
}

void FileManager::saveFile()
{
    (void) saveEditor(currentEditor());
}

bool FileManager::saveEditorAs(QalamEditor *editor)
{
    if (!editor) return false;

    QString content = editor->toPlainText();
    const QString oldPath = editor->currentFilePath();
    QString currentPath = oldPath;
    QString currentName = currentPath.isEmpty() ? "ملف جديد.baa" : QFileInfo(currentPath).fileName();
    QString fileName = QFileDialog::getSaveFileName(m_parentWindow, "حفظ الملف", currentName,
                                                     "ملف باء (*.baa);;مكتبة باء (*.baahd);;ملف نظم (*.نظم);;Text Files (*.txt);;All Files (*)");

    if (fileName.isEmpty()) return false;

    const QString normalizedPath = normalizePath(fileName);
    QSaveFile file(normalizedPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(m_parentWindow, "خطأ",
                             "لا يمكن حفظ الملف:\n" + file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;

    if (!file.commit()) {
        QMessageBox::warning(m_parentWindow, "خطأ",
                             "فشل إكمال حفظ الملف:\n" + file.errorString());
        return false;
    }

    editor->setFilePath(normalizedPath);
    editor->document()->setModified(false);

    int index = m_tabWidget->indexOf(editor);
    if (index != -1) {
        QFileInfo fileInfo(normalizedPath);
        m_tabWidget->setTabText(index, fileInfo.fileName());
        m_tabWidget->setTabToolTip(index, normalizedPath);
    }

    editor->removeBackupFile();
    if (!oldPath.isEmpty() and normalizePath(oldPath) != normalizedPath) {
        removeBackupForPath(oldPath);
    }
    addRecentFile(normalizedPath);
    emit fileStateChanged();
    emit openEditorsChanged();
    return true;
}

void FileManager::saveFileAs()
{
    (void) saveEditorAs(currentEditor());
}
