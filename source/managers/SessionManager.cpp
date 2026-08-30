#include "SessionManager.h"
#include "QalamExplorerView.h"
#include "QalamEditorWorkspace.h"
#include "QalamDocumentModel.h"
#include "FileManager.h"

#include <QSettings>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QStringConverter>
#include <QTextStream>
#include <QVariantList>
#include <QVariantMap>

#include <utility>
#include <algorithm>

SessionManager::SessionManager(QalamEditorWorkspace *editorWorkspace,
                               QObject *parent,
                               const QString &settingsFilePath)
    : QObject(parent)
    , m_editorWorkspace(editorWorkspace)
    , m_settingsFilePath(settingsFilePath)
{
}

void SessionManager::saveSession(const QString &folderPath,
                                 const QByteArray &windowGeometry,
                                 bool cleanShutdown)
{
    saveSession(folderPath.isEmpty()
                    ? QStringList{}
                    : QStringList{folderPath},
                windowGeometry, cleanShutdown);
}

void SessionManager::saveSession(const QStringList &folderPaths,
                                 const QByteArray &windowGeometry,
                                 bool cleanShutdown)
{
    const std::unique_ptr<QSettings> settings = createSettings();

    QStringList openFiles;
    QVariantList documents;
    QVariantList views;
    QStringList usedRecoveryPaths;
    int activeDocumentIndex = -1;
    QHash<QalamDocumentModel*, int> documentIndices;

    for (int groupIndex = 0;
         groupIndex < m_editorWorkspace->groupCount(); ++groupIndex) {
        QTabWidget *group = m_editorWorkspace->group(groupIndex);
        if (not group) continue;

        for (int tabIndex = 0; tabIndex < group->count(); ++tabIndex) {
            QalamEditor *editor = qobject_cast<QalamEditor*>(
                group->widget(tabIndex));
            if (not editor) continue;

            const QString filePath = editor->currentFilePath();
            QString displayName = group->tabText(tabIndex);
            if (displayName.endsWith(QStringLiteral("[*]"))) {
                displayName.chop(3);
            }

            // A clean shutdown follows an explicit save/discard decision. Do
            // not resurrect discarded unnamed buffers on the next launch.
            if (cleanShutdown and filePath.isEmpty()) continue;

            int documentIndex = documentIndices.value(
                editor->documentModel(), -1);
            if (documentIndex < 0) {
                documentIndex = documents.size();
                documentIndices.insert(editor->documentModel(), documentIndex);

                const bool modified = editor->document()->isModified();
                QVariantMap document;
                document.insert(QStringLiteral("filePath"), filePath);
                document.insert(QStringLiteral("displayName"), displayName);
                document.insert(QStringLiteral("modified"),
                                modified and not cleanShutdown);

                if (not cleanShutdown and modified) {
                    const QString identity = filePath.isEmpty()
                        ? QStringLiteral("untitled:%1:%2")
                              .arg(documentIndex).arg(displayName)
                        : QStringLiteral("file:%1")
                              .arg(QDir::cleanPath(filePath));
                    const QString backupPath = recoveryPath(identity);
                    QDir().mkpath(QFileInfo(backupPath).absolutePath());

                    QSaveFile recovery(backupPath);
                    if (recovery.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&recovery);
                        out.setEncoding(QStringConverter::Utf8);
                        out << editor->toPlainText();
                        if (recovery.commit()) {
                            document.insert(QStringLiteral("recoveryPath"),
                                            backupPath);
                            usedRecoveryPaths.append(backupPath);
                        }
                    }
                }

                if (not filePath.isEmpty()
                    and not openFiles.contains(filePath)) {
                    openFiles.append(filePath);
                }
                documents.append(document);
            }

            QVariantMap view;
            view.insert(QStringLiteral("documentIndex"), documentIndex);
            view.insert(QStringLiteral("groupIndex"), groupIndex);
            view.insert(QStringLiteral("tabIndex"), tabIndex);
            const bool active = group->currentIndex() == tabIndex;
            view.insert(QStringLiteral("active"), active);
            views.append(view);

            if (groupIndex == m_editorWorkspace->activeGroupIndex()
                and active) {
                activeDocumentIndex = documentIndex;
            }
        }
    }

    settings->setValue(Constants::SessionKeyOpenFiles, openFiles);
    settings->setValue(Constants::SessionKeyDocuments, documents);
    settings->setValue(Constants::SessionKeyEditorViews, views);
    settings->setValue(Constants::SessionKeyActiveTab, activeDocumentIndex);
    settings->setValue(Constants::SessionKeyEditorGroupCount,
                       m_editorWorkspace->groupCount());
    settings->setValue(Constants::SessionKeyEditorSplitOrientation,
                       int(m_editorWorkspace->splitOrientation()));
    QVariantList splitSizes;
    for (int size : m_editorWorkspace->splitSizes()) splitSizes.append(size);
    settings->setValue(Constants::SessionKeyEditorSplitSizes, splitSizes);
    settings->setValue(Constants::SessionKeyActiveEditorGroup,
                       m_editorWorkspace->activeGroupIndex());
    settings->setValue(Constants::SessionKeyFolderPaths, folderPaths);
    settings->setValue(Constants::SessionKeyFolderPath,
                       folderPaths.value(0));
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
    data.activeGroupIndex = settings->value(
        Constants::SessionKeyActiveEditorGroup, 0).toInt();
    data.splitOrientation = Qt::Orientation(settings->value(
        Constants::SessionKeyEditorSplitOrientation,
        int(Qt::Horizontal)).toInt());
    const QVariantList savedSplitSizes = settings->value(
        Constants::SessionKeyEditorSplitSizes).toList();
    for (const QVariant &size : savedSplitSizes) {
        data.splitSizes.append(size.toInt());
    }
    data.folderPaths = settings->value(
        Constants::SessionKeyFolderPaths).toStringList();
    data.folderPath = settings->value(
        Constants::SessionKeyFolderPath).toString();
    if (data.folderPaths.isEmpty() and not data.folderPath.isEmpty()) {
        data.folderPaths.push_back(data.folderPath);
    }
    if (data.folderPath.isEmpty()) data.folderPath = data.folderPaths.value(0);
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

    const QVariantList views = settings->value(
        Constants::SessionKeyEditorViews).toList();
    for (const QVariant &value : views) {
        const QVariantMap saved = value.toMap();
        ViewData view;
        view.documentIndex = saved.value(
            QStringLiteral("documentIndex"), -1).toInt();
        view.groupIndex = saved.value(QStringLiteral("groupIndex"), 0).toInt();
        view.tabIndex = saved.value(QStringLiteral("tabIndex"), 0).toInt();
        view.active = saved.value(QStringLiteral("active"), false).toBool();
        if (view.documentIndex >= 0
            and view.documentIndex < data.documents.size()
            and view.groupIndex >= 0 and view.groupIndex < 2) {
            data.views.push_back(view);
        }
    }
    if (data.views.isEmpty()) {
        for (int index = 0; index < data.documents.size(); ++index) {
            data.views.push_back({index, 0, index,
                                  index == data.activeTabIndex});
        }
    }
    return data;
}

bool SessionManager::restoreWorkspace(const SessionData &session,
                                      FileManager *fileManager)
{
    if (not fileManager or not m_editorWorkspace) return false;

    QVector<QalamEditor*> restoredDocuments(session.documents.size());
    const bool needsSecondGroup = std::any_of(
        session.views.cbegin(), session.views.cend(),
        [](const ViewData &view) { return view.groupIndex == 1; });
    if (needsSecondGroup) {
        m_editorWorkspace->ensureTwoGroups(session.splitOrientation);
    }

    bool restoredAny = false;
    for (int groupIndex = 0; groupIndex < 2; ++groupIndex) {
        QVector<ViewData> groupViews;
        for (const ViewData &view : session.views) {
            if (view.groupIndex == groupIndex) groupViews.push_back(view);
        }
        std::sort(groupViews.begin(), groupViews.end(),
                  [](const ViewData &left, const ViewData &right) {
            return left.tabIndex < right.tabIndex;
        });

        for (const ViewData &view : groupViews) {
            if (view.documentIndex < 0
                or view.documentIndex >= session.documents.size()) {
                continue;
            }
            const DocumentData &document = session.documents.at(
                view.documentIndex);
            QalamEditor *editor = restoredDocuments.at(view.documentIndex);
            if (not editor) {
                m_editorWorkspace->setActiveGroupIndex(groupIndex);
                if (not fileManager->restoreDocument(
                        document.filePath, document.displayName,
                        document.recoveredContent, document.hasRecovery)) {
                    continue;
                }
                editor = m_editorWorkspace->currentEditor();
                restoredDocuments[view.documentIndex] = editor;
                restoredAny = true;
            } else {
                m_editorWorkspace->addSharedView(
                    editor, groupIndex, document.displayName,
                    document.filePath);
            }
        }
    }

    if (not restoredAny) return false;

    m_editorWorkspace->setSplitSizes(session.splitSizes);
    for (const ViewData &view : session.views) {
        if (not view.active
            or view.documentIndex < 0
            or view.documentIndex >= restoredDocuments.size()) {
            continue;
        }
        QalamEditor *modelView = restoredDocuments.at(view.documentIndex);
        QTabWidget *group = m_editorWorkspace->group(view.groupIndex);
        if (not modelView or not group) continue;
        for (int index = 0; index < group->count(); ++index) {
            auto *candidate = qobject_cast<QalamEditor*>(group->widget(index));
            if (candidate and candidate->documentModel()
                == modelView->documentModel()) {
                group->setCurrentIndex(index);
                break;
            }
        }
    }
    m_editorWorkspace->setActiveGroupIndex(session.activeGroupIndex);
    return true;
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
    constexpr int minimumWidth = Constants::Layout::WindowMinWidth;
    constexpr int minimumHeight = Constants::Layout::WindowMinHeight;
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
    QSet<QalamDocumentModel*> seenDocuments;

    for (int i = 0; i < m_editorWorkspace->count(); ++i) {
        QalamEditor *editor = qobject_cast<QalamEditor*>(
            m_editorWorkspace->widget(i));
        if (editor) {
            if (seenDocuments.contains(editor->documentModel())) continue;
            seenDocuments.insert(editor->documentModel());
            QString filePath = editor->currentFilePath();
            bool modified = editor->document()->isModified();

            // Use tab text if no file path (unsaved file)
            if (filePath.isEmpty()) {
                filePath = m_editorWorkspace->tabText(i);
                if (filePath.endsWith("[*]")) {
                    filePath.chop(3);
                }
            }
            explorerView->addOpenEditor(filePath, modified);
        }
    }
}
