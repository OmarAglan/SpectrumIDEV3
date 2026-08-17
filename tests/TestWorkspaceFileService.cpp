#include "WorkspaceFileService.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class TestWorkspaceFileService : public QObject {
    Q_OBJECT

private slots:
    void acceptsPortableArabicNames();
    void rejectsUnsafeNames_data();
    void rejectsUnsafeNames();
    void createsRenamesAndDeletesArabicFile();
    void removesDirectoryRecursively();
    void preventsWorkspaceEscapeAndRootMutation();
    void neverOverwritesExistingEntry();
};

void TestWorkspaceFileService::acceptsPortableArabicNames()
{
    QString error;
    QVERIFY(WorkspaceFileService::isValidEntryName(
        QStringLiteral("برنامج_التجربة.baa"), &error));
    QVERIFY(error.isEmpty());
    QVERIFY(WorkspaceFileService::isValidEntryName(
        QStringLiteral("مجلد المشروع"), &error));
}

void TestWorkspaceFileService::rejectsUnsafeNames_data()
{
    QTest::addColumn<QString>("name");
    QTest::newRow("empty") << QString();
    QTest::newRow("parent") << QStringLiteral("..");
    QTest::newRow("slash") << QStringLiteral("مجلد/ملف");
    QTest::newRow("backslash") << QStringLiteral("مجلد\\ملف");
    QTest::newRow("leading-space") << QStringLiteral(" ملف");
    QTest::newRow("trailing-space") << QStringLiteral("ملف ");
    QTest::newRow("trailing-dot") << QStringLiteral("ملف.");
    QTest::newRow("portable-reserved") << QStringLiteral("CON.baa");
    QTest::newRow("wildcard") << QStringLiteral("ملف*.baa");
}

void TestWorkspaceFileService::rejectsUnsafeNames()
{
    QFETCH(QString, name);
    QString error;
    QVERIFY(not WorkspaceFileService::isValidEntryName(name, &error));
    QVERIFY(not error.isEmpty());
}

void TestWorkspaceFileService::createsRenamesAndDeletesArabicFile()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString root = QDir(temporary.path()).filePath(QStringLiteral("مشروع باء"));
    QVERIFY(QDir().mkdir(root));

    const WorkspaceFileResult created = WorkspaceFileService::createFile(
        root, root, QStringLiteral("البداية.baa"));
    QVERIFY2(created.success, qPrintable(created.error));
    QVERIFY(QFileInfo::exists(created.path));

    const WorkspaceFileResult renamed = WorkspaceFileService::renameEntry(
        root, created.path, QStringLiteral("الرئيسية.baa"));
    QVERIFY2(renamed.success, qPrintable(renamed.error));
    QVERIFY(not QFileInfo::exists(created.path));
    QVERIFY(QFileInfo::exists(renamed.path));

    const WorkspaceFileResult removed = WorkspaceFileService::removeEntry(
        root, renamed.path);
    QVERIFY2(removed.success, qPrintable(removed.error));
    QVERIFY(not QFileInfo::exists(renamed.path));
}

void TestWorkspaceFileService::removesDirectoryRecursively()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString root = QDir(temporary.path()).filePath(QStringLiteral("مشروع"));
    QVERIFY(QDir().mkdir(root));

    const WorkspaceFileResult directory = WorkspaceFileService::createDirectory(
        root, root, QStringLiteral("مصادر"));
    QVERIFY2(directory.success, qPrintable(directory.error));
    const WorkspaceFileResult file = WorkspaceFileService::createFile(
        root, directory.path, QStringLiteral("وحدة.baa"));
    QVERIFY2(file.success, qPrintable(file.error));

    const WorkspaceFileResult removed = WorkspaceFileService::removeEntry(
        root, directory.path);
    QVERIFY2(removed.success, qPrintable(removed.error));
    QVERIFY(not QFileInfo::exists(directory.path));
}

void TestWorkspaceFileService::preventsWorkspaceEscapeAndRootMutation()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString root = QDir(temporary.path()).filePath(QStringLiteral("المشروع"));
    const QString outside = QDir(temporary.path()).filePath(QStringLiteral("خارجي"));
    QVERIFY(QDir().mkdir(root));
    QVERIFY(QDir().mkdir(outside));

    const WorkspaceFileResult escapedCreation = WorkspaceFileService::createFile(
        root, outside, QStringLiteral("خارج.baa"));
    QVERIFY(not escapedCreation.success);
    QVERIFY(not QFileInfo::exists(QDir(outside).filePath(QStringLiteral("خارج.baa"))));

    const WorkspaceFileResult rootRename = WorkspaceFileService::renameEntry(
        root, root, QStringLiteral("مشروع_آخر"));
    QVERIFY(not rootRename.success);
    const WorkspaceFileResult rootRemoval = WorkspaceFileService::removeEntry(root, root);
    QVERIFY(not rootRemoval.success);
    QVERIFY(QFileInfo::exists(root));
}

void TestWorkspaceFileService::neverOverwritesExistingEntry()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString root = QDir(temporary.path()).filePath(QStringLiteral("المشروع"));
    QVERIFY(QDir().mkdir(root));

    const WorkspaceFileResult first = WorkspaceFileService::createFile(
        root, root, QStringLiteral("ملف.baa"));
    QVERIFY(first.success);
    const WorkspaceFileResult duplicate = WorkspaceFileService::createFile(
        root, root, QStringLiteral("ملف.baa"));
    QVERIFY(not duplicate.success);

    const WorkspaceFileResult other = WorkspaceFileService::createFile(
        root, root, QStringLiteral("آخر.baa"));
    QVERIFY(other.success);
    const WorkspaceFileResult collision = WorkspaceFileService::renameEntry(
        root, other.path, QStringLiteral("ملف.baa"));
    QVERIFY(not collision.success);
    QVERIFY(QFileInfo::exists(first.path));
    QVERIFY(QFileInfo::exists(other.path));
}

QTEST_MAIN(TestWorkspaceFileService)
#include "TestWorkspaceFileService.moc"
