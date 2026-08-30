#include "SessionManager.h"
#include "FileManager.h"
#include "QalamEditor.h"
#include "QalamEditorWorkspace.h"
#include "Constants.h"

#include <QSettings>
#include <QFile>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTest>
#include <QTemporaryDir>

class TestSessionManager final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void acceptsReachableDesktopGeometry();
    void acceptsCompactSnapGeometry();
    void acceptsGeometryOnSecondaryMonitor();
    void rejectsTinyGeometry();
    void rejectsOffscreenGeometry();
    void rejectsBarelyVisibleGeometry();
    void restoresInterruptedUntitledBuffer();
    void restoresInterruptedWorkbenchThroughFileManager();
    void restoresSharedSplitWorkspaceLayout();
    void restoresActiveDocumentInEachEditorGroup();
    void cleanShutdownDoesNotRestoreDiscardedUntitledBuffer();
    void persistsMultipleWorkspaceRoots();

private:
    QTemporaryDir m_settingsDirectory;
};

void TestSessionManager::initTestCase()
{
    QVERIFY(m_settingsDirectory.isValid());
    QStandardPaths::setTestModeEnabled(true);
}

void TestSessionManager::acceptsReachableDesktopGeometry()
{
    QVERIFY(SessionManager::isUsableWindowGeometry(
        QRect(100, 100, 1280, 720), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::acceptsCompactSnapGeometry()
{
    QVERIFY(SessionManager::isUsableWindowGeometry(
        QRect(0, 0,
              Constants::Layout::WindowMinWidth,
              Constants::Layout::WindowMinHeight),
        {QRect(0, 0, 1366, 728)}));
}

void TestSessionManager::acceptsGeometryOnSecondaryMonitor()
{
    QVERIFY(SessionManager::isUsableWindowGeometry(
        QRect(-1700, 100, 1200, 700),
        {QRect(0, 0, 1920, 1040), QRect(-1920, 0, 1920, 1080)}));
}

void TestSessionManager::rejectsTinyGeometry()
{
    QVERIFY(not SessionManager::isUsableWindowGeometry(
        QRect(48, 982, 50, 50), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::rejectsOffscreenGeometry()
{
    QVERIFY(not SessionManager::isUsableWindowGeometry(
        QRect(2400, 1200, 1280, 720), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::rejectsBarelyVisibleGeometry()
{
    QVERIFY(not SessionManager::isUsableWindowGeometry(
        QRect(48, 982, 900, 600), {QRect(0, 0, 1920, 1040)}));
}

void TestSessionManager::restoresInterruptedUntitledBuffer()
{
    QalamEditorWorkspace tabs;
    auto *editor = new QalamEditor(&tabs);
    tabs.addTab(editor, QStringLiteral("غير معنون"));
    editor->setPlainText(QStringLiteral("صحيح الرئيسية() {\n    ارجع ٠.\n}"));
    editor->document()->setModified(true);

    const QString settingsPath = m_settingsDirectory.filePath(
        QStringLiteral("interrupted-session.ini"));
    SessionManager manager(&tabs, nullptr, settingsPath);
    manager.saveSession(QStringLiteral("C:/مشروع"), QByteArray("geometry"));

    QSettings savedSettings(settingsPath, QSettings::IniFormat);
    QCOMPARE(savedSettings.value(Constants::SessionKeyFolderPath).toString(),
             QStringLiteral("C:/مشروع"));

    const SessionManager::SessionData restored = manager.restoreSession();
    QCOMPARE(restored.folderPath, QStringLiteral("C:/مشروع"));
    QCOMPARE(restored.activeTabIndex, 0);
    QCOMPARE(restored.documents.size(), 1);
    QVERIFY(restored.recoveredAfterInterruption);
    QVERIFY(restored.documents.constFirst().hasRecovery);
    QCOMPARE(restored.documents.constFirst().recoveredContent,
             editor->toPlainText());
}

void TestSessionManager::cleanShutdownDoesNotRestoreDiscardedUntitledBuffer()
{
    QalamEditorWorkspace tabs;
    auto *editor = new QalamEditor(&tabs);
    tabs.addTab(editor, QStringLiteral("غير معنون"));
    editor->setPlainText(QStringLiteral("تعديل لن يعود"));
    editor->document()->setModified(true);

    SessionManager manager(
        &tabs, nullptr,
        m_settingsDirectory.filePath(QStringLiteral("clean-session.ini")));
    manager.saveSession(QString(), QByteArray(), true);

    const SessionManager::SessionData restored = manager.restoreSession();
    QVERIFY(restored.documents.isEmpty());
    QVERIFY(not restored.recoveredAfterInterruption);
}

void TestSessionManager::persistsMultipleWorkspaceRoots()
{
    QalamEditorWorkspace workspace;
    SessionManager manager(
        &workspace, nullptr,
        m_settingsDirectory.filePath(QStringLiteral("multi-root.ini")));
    const QStringList roots{
        QStringLiteral("C:/مشروع أول"),
        QStringLiteral("D:/مشروع ثان")};
    manager.saveSession(roots, QByteArray("geometry"), true);

    const SessionManager::SessionData restored = manager.restoreSession();
    QCOMPARE(restored.folderPaths, roots);
    QCOMPARE(restored.folderPath, roots.constFirst());
}

void TestSessionManager::restoresInterruptedWorkbenchThroughFileManager()
{
    const QString sourcePath = m_settingsDirectory.filePath(
        QStringLiteral("ملف محفوظ.باء"));
    QFile source(sourcePath);
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Text));
    source.write("original\n");
    source.close();

    QalamEditorWorkspace originalTabs;
    auto *savedEditor = new QalamEditor(&originalTabs);
    savedEditor->setFilePath(sourcePath);
    savedEditor->setPlainText(QStringLiteral("تعديل قبل الانقطاع"));
    savedEditor->document()->setModified(true);
    originalTabs.addTab(savedEditor, QStringLiteral("ملف محفوظ.باء[*]"));

    auto *untitledEditor = new QalamEditor(&originalTabs);
    untitledEditor->setPlainText(QStringLiteral("مسودة غير محفوظة"));
    untitledEditor->document()->setModified(true);
    originalTabs.addTab(untitledEditor, QStringLiteral("غير معنون[*]"));
    originalTabs.setCurrentIndex(1);

    const QString settingsPath = m_settingsDirectory.filePath(
        QStringLiteral("workbench-recovery.ini"));
    SessionManager originalSession(&originalTabs, nullptr, settingsPath);
    originalSession.saveSession(m_settingsDirectory.path(), QByteArray());

    QalamEditorWorkspace restoredTabs;
    FileManager fileManager(&restoredTabs, &restoredTabs);
    SessionManager restoredSession(&restoredTabs, nullptr, settingsPath);
    const SessionManager::SessionData data = restoredSession.restoreSession();
    QVERIFY(data.recoveredAfterInterruption);
    QCOMPARE(data.documents.size(), 2);
    QCOMPARE(data.activeTabIndex, 1);

    for (const SessionManager::DocumentData &document : data.documents) {
        QVERIFY(fileManager.restoreDocument(
            document.filePath, document.displayName,
            document.recoveredContent, document.hasRecovery));
    }
    restoredTabs.setCurrentIndex(data.activeTabIndex);

    QCOMPARE(restoredTabs.count(), 2);
    auto *restoredSaved = qobject_cast<QalamEditor*>(restoredTabs.widget(0));
    auto *restoredUntitled = qobject_cast<QalamEditor*>(restoredTabs.widget(1));
    QVERIFY(restoredSaved);
    QVERIFY(restoredUntitled);
    QCOMPARE(restoredSaved->toPlainText(),
             QStringLiteral("تعديل قبل الانقطاع"));
    QCOMPARE(restoredSaved->currentFilePath(), sourcePath);
    QVERIFY(restoredSaved->document()->isModified());
    QCOMPARE(restoredUntitled->toPlainText(),
             QStringLiteral("مسودة غير محفوظة"));
    QVERIFY(restoredUntitled->currentFilePath().isEmpty());
    QVERIFY(restoredUntitled->document()->isModified());
    QCOMPARE(restoredTabs.currentIndex(), 1);
}

void TestSessionManager::restoresSharedSplitWorkspaceLayout()
{
    QalamEditorWorkspace originalWorkspace;
    originalWorkspace.resize(900, 600);
    auto *editor = new QalamEditor(&originalWorkspace);
    originalWorkspace.addTab(editor, QStringLiteral("مشترك[*]"));
    editor->setPlainText(QStringLiteral("محتوى مستعاد في نافذتين"));
    editor->document()->setModified(true);
    QVERIFY(originalWorkspace.splitCurrent(Qt::Vertical));
    originalWorkspace.setSplitSizes({360, 240});
    QCOMPARE(originalWorkspace.activeGroupIndex(), 1);

    const QString settingsPath = m_settingsDirectory.filePath(
        QStringLiteral("split-workspace.ini"));
    SessionManager originalSession(
        &originalWorkspace, nullptr, settingsPath);
    originalSession.saveSession(QStringLiteral("C:/مشروع مشترك"),
                                QByteArray());

    const SessionManager::SessionData data = originalSession.restoreSession();
    QCOMPARE(data.documents.size(), 1);
    QCOMPARE(data.views.size(), 2);
    QCOMPARE(data.views.at(0).documentIndex, 0);
    QCOMPARE(data.views.at(1).documentIndex, 0);
    QCOMPARE(data.views.at(0).groupIndex, 0);
    QCOMPARE(data.views.at(1).groupIndex, 1);
    QCOMPARE(data.activeGroupIndex, 1);
    QCOMPARE(data.splitOrientation, Qt::Vertical);

    QalamEditorWorkspace restoredWorkspace;
    restoredWorkspace.resize(900, 600);
    FileManager fileManager(&restoredWorkspace, &restoredWorkspace);
    SessionManager restoredSession(
        &restoredWorkspace, nullptr, settingsPath);
    QVERIFY(restoredSession.restoreWorkspace(data, &fileManager));

    QCOMPARE(restoredWorkspace.groupCount(), 2);
    QCOMPARE(restoredWorkspace.editors().size(), 2);
    QCOMPARE(restoredWorkspace.activeGroupIndex(), 1);
    QCOMPARE(restoredWorkspace.splitOrientation(), Qt::Vertical);
    QalamEditor *first = restoredWorkspace.editors().at(0);
    QalamEditor *second = restoredWorkspace.editors().at(1);
    QCOMPARE(first->documentModel(), second->documentModel());
    QCOMPARE(first->document(), second->document());
    QCOMPARE(first->toPlainText(),
             QStringLiteral("محتوى مستعاد في نافذتين"));
    QVERIFY(first->document()->isModified());
}

void TestSessionManager::restoresActiveDocumentInEachEditorGroup()
{
    const QStringList names{
        QStringLiteral("الأول.باء"),
        QStringLiteral("الثاني.باء"),
        QStringLiteral("الثالث.باء")};
    QStringList paths;
    for (const QString &name : names) {
        const QString path = m_settingsDirectory.filePath(name);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("صحيح الرئيسية() { ارجع ٠. }\n");
        file.close();
        paths.push_back(path);
    }

    QalamEditorWorkspace originalWorkspace;
    FileManager originalFiles(&originalWorkspace, &originalWorkspace);
    originalFiles.openFile(paths.at(0));
    originalFiles.openFile(paths.at(1));
    QalamEditor *sharedDocument = originalWorkspace.currentEditor();
    QVERIFY(sharedDocument);
    QVERIFY(originalWorkspace.splitCurrent(Qt::Horizontal));
    originalFiles.openFile(paths.at(2));

    QTabWidget *primary = originalWorkspace.group(0);
    QTabWidget *secondary = originalWorkspace.group(1);
    QVERIFY(primary);
    QVERIFY(secondary);
    primary->setCurrentIndex(0);
    secondary->setCurrentIndex(0);
    originalWorkspace.setActiveGroupIndex(1);

    const QString settingsPath = m_settingsDirectory.filePath(
        QStringLiteral("two-group-active-tabs.ini"));
    SessionManager originalSession(
        &originalWorkspace, nullptr, settingsPath);
    originalSession.saveSession(m_settingsDirectory.path(), QByteArray());
    const SessionManager::SessionData data = originalSession.restoreSession();

    QalamEditorWorkspace restoredWorkspace;
    FileManager restoredFiles(&restoredWorkspace, &restoredWorkspace);
    SessionManager restoredSession(
        &restoredWorkspace, nullptr, settingsPath);
    QVERIFY(restoredSession.restoreWorkspace(data, &restoredFiles));

    QCOMPARE(restoredWorkspace.groupCount(), 2);
    primary = restoredWorkspace.group(0);
    secondary = restoredWorkspace.group(1);
    QCOMPARE(primary->count(), 2);
    QCOMPARE(secondary->count(), 2);
    QCOMPARE(primary->tabText(primary->currentIndex()), names.at(0));
    QCOMPARE(secondary->tabText(secondary->currentIndex()), names.at(1));
    QCOMPARE(restoredWorkspace.activeGroupIndex(), 1);

    auto *restoredSharedPrimary = qobject_cast<QalamEditor*>(
        primary->widget(1));
    auto *restoredSharedSecondary = qobject_cast<QalamEditor*>(
        secondary->widget(0));
    QVERIFY(restoredSharedPrimary);
    QVERIFY(restoredSharedSecondary);
    QCOMPARE(restoredSharedPrimary->documentModel(),
             restoredSharedSecondary->documentModel());
}

QTEST_MAIN(TestSessionManager)

#include "TestSessionManager.moc"
