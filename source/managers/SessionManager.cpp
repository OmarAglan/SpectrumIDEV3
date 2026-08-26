#include "SessionManager.h"
#include "QalamExplorerView.h"

#include <QSettings>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTextStream>
#include <QVariantList>
#include <QVariantMap>

#include <utility>

SessionManager::SessionManager(QTabWidget *tabWidget,
                               QObject *parent,
                               const QString &settingsFilePath)
    : QObject(parent)
    , m_tabWidget(tabWidget)
    , m_settingsFilePath(settingsFilePath)
{
}

void SessionManager::saveSession(const QString &folderPath,
                                 const QByteArray &windowGeometry,
                                 bool cleanShutdown)
{
    const std::unique_ptr<QSettings> settings = createSettings();

    QStringList openFiles;
    QVariantList documents;
    QStringList usedRecoveryPaths;
    int activeDocumentIndex = -1;

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(m_tabWidget->widget(i));
        if (not editor) continue;

        const QString filePath = editor->currentFilePath();
        QString displayName = m_tabWidget->tabText(i);
        if (displayName.endsWith(QStringLiteral("[*]"))) displayName.chop(3);
        const bool modified = editor->document()->isModified();

        // A clean shutdown follows an explicit save/discard decision. Do not
        // resurrect discarded unnamed buffers on the next launch.
        if (cleanShutdown and filePath.isEmpty()) continue;

        QVariantMap document;
        document.insert(QStringLiteral("filePath"), filePath);
        document.insert(QStringLiteral("displayName"), displayName);
        document.insert(QStringLiteral("modified"), modified and not cleanShutdown);

        if (not cleanShutdown and modified) {
            const QString identity = filePath.isEmpty()
                ? QStringLiteral("untitled:%1:%2").arg(i).arg(displayName)
                : QStringLiteral("file:%1").arg(QDir::cleanPath(filePath));
            const QString backupPath = recoveryPath(identity);
            QDir().mkpath(QFileInfo(backupPath).absolutePath());

            QSaveFile recovery(backupPath);
            if (recovery.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&recovery);
                out.setEncoding(QStringConverter::Utf8);
                out << editor->toPlainText();
                if (recovery.commit()) {
                    document.insert(QStringLiteral("recoveryPath"), backupPath);
                    usedRecoveryPaths.append(backupPath);
                }
            }
        }

        if (not filePath.isEmpty()) openFiles.append(filePath);
        if (i == m_tabWidget->currentIndex()) activeDocumentIndex = documents.size();
        documents.append(document);
    }

    settings->setValue(Constants::SessionKeyOpenFiles, openFiles);
    settings->setValue(Constants::SessionKeyDocuments, documents);
    settings->setValue(Constants::SessionKeyActiveTab, activeDocumentIndex);
    settings->setValue(Constants::SessionKeyFolderPath, folderPath);
    settings->setValue(Constants::SessionKeyWindowGeometry, windowGeometry);
    settings->setValue(Constants::SessionKeyCleanShutdown, cleanShutdown);
    settings->sync();

    removeUnusedRecoveryFiles(usedRecoveryPaths);
}

SessionManager::SessionData SessionManager::restoreSession() const
{
    const std::unique_ptr<QSettings> settings = createSettings();

    SessionData data;
    data.openFiles = settings->value(Constants::SessionKeyOpenFiles).toStringList();
    data.activeTabIndex = settings->value(Constants::SessionKeyActiveTab, -1).toInt();
    data.folderPath = settings->value(Constants::SessionKeyFolderPath).toString();
    data.windowGeometry = settings->value(Constants::SessionKeyWindowGeometry).toByteArray();
    const bool cleanShutdown = settings->value(
        Constants::SessionKeyCleanShutdown, true).toBool();
    data.recoveredAfterInterruption = not cleanShutdown;

    const QVariantList documents = settings->value(
        Constants::SessionKeyDocuments).toList();
    for (const QVariant &value : documents) {
        const QVariantMap saved = value.toMap();
        DocumentData document;
        document.filePath = saved.value(QStringLiteral("filePath")).toString();
        document.displayName = saved.value(QStringLiteral("displayName")).toString();
        document.modified = saved.value(QStringLiteral("modified")).toBool();

        const QString recoveryPath = cleanShutdown
            ? QString() : saved.value(QStringLiteral("recoveryPath")).toString();
        if (not recoveryPath.isEmpty()) {
            QFile recovery(recoveryPath);
            if (recovery.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&recovery);
                in.setEncoding(QStringConverter::Utf8);
                document.recoveredContent = in.readAll();
                document.hasRecovery = true;
            }
        }

        if (not document.filePath.isEmpty() or document.hasRecovery) {
            data.documents.push_back(std::move(document));
        }
    }

    // Compatibility with sessions written before structured documents.
    if (data.documents.isEmpty()) {
        for (const QString &filePath : data.openFiles) {
            DocumentData document;
            document.filePath = filePath;
            document.displayName = QFileInfo(filePath).fileName();
            data.documents.push_back(std::move(document));
        }
    }
    return data;
}

void SessionManager::markSessionRunning()
{
    const std::unique_ptr<QSettings> settings = createSettings();
    settings->setValue(Constants::SessionKeyCleanShutdown, false);
    settings->sync();
}

QString SessionManager::recoveryDirectory() const
{
    if (not m_settingsFilePath.isEmpty()) {
        return QDir(QFileInfo(m_settingsFilePath).absolutePath())
            .filePath(QStringLiteral("recovery"));
    }
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
        .filePath(QStringLiteral("recovery"));
}

QString SessionManager::recoveryPath(const QString &identity) const
{
    const QByteArray digest = QCryptographicHash::hash(
        identity.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QDir(recoveryDirectory()).filePath(
        QString::fromLatin1(digest) + QStringLiteral(".recovery"));
}

void SessionManager::removeUnusedRecoveryFiles(const QStringList &usedPaths) const
{
    QDir directory(recoveryDirectory());
    if (not directory.exists()) return;

    const QStringList files = directory.entryList(
        {QStringLiteral("*.recovery")}, QDir::Files);
    for (const QString &file : files) {
        const QString path = directory.filePath(file);
        if (not usedPaths.contains(path)) QFile::remove(path);
    }
}

std::unique_ptr<QSettings> SessionManager::createSettings() const
{
    if (not m_settingsFilePath.isEmpty()) {
        return std::make_unique<QSettings>(
            m_settingsFilePath, QSettings::IniFormat);
    }
    return std::make_unique<QSettings>(Constants::OrgName, Constants::AppName);
}

bool SessionManager::isUsableWindowGeometry(
    const QRect &windowGeometry,
    const QList<QRect> &availableScreens)
{
    constexpr int minimumWidth = 640;
    constexpr int minimumHeight = 480;
    constexpr int minimumVisibleWidth = 160;
    constexpr int minimumVisibleHeight = 80;

    if (windowGeometry.width() < minimumWidth
        or windowGeometry.height() < minimumHeight) {
        return false;
    }

    for (const QRect &screen : availableScreens) {
        const QRect visible = windowGeometry.intersected(screen);
        if (visible.width() >= minimumVisibleWidth
            and visible.height() >= minimumVisibleHeight) {
            return true;
        }
    }

    return false;
}

void SessionManager::savePreferences(QalamEditor *editor, int themeIndex)
{
    if (not editor) return;

    QSettings settings(Constants::OrgName, Constants::AppName);
    settings.setValue(Constants::SettingsKeyFontSize, editor->font().pixelSize());
    settings.setValue(Constants::SettingsKeyFontType, editor->font().family());
    settings.setValue(Constants::SettingsKeyTheme, themeIndex);
    settings.sync();
}

void SessionManager::syncOpenEditors(QalamExplorerView *explorerView)
{
    if (not explorerView) return;

    explorerView->clearOpenEditors();

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(m_tabWidget->widget(i));
        if (editor) {
            QString filePath = editor->currentFilePath();
            bool modified = editor->document()->isModified();

            // Use tab text if no file path (unsaved file)
            if (filePath.isEmpty()) {
                filePath = m_tabWidget->tabText(i);
                if (filePath.endsWith("[*]")) {
                    filePath.chop(3);
                }
            }
            explorerView->addOpenEditor(filePath, modified);
        }
    }
}
