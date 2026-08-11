#include "BaaLanguageClient.h"

#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>

#ifndef QALAM_FAKE_BAA_LSP_PATH
#error QALAM_FAKE_BAA_LSP_PATH must be defined
#endif

class TestBaaLanguageClient : public QObject
{
    Q_OBJECT
private slots:
    void synchronizesDocumentsAndRejectsStaleDiagnostics();
    void restartsAfterUnexpectedExitAndReopensDocuments();
    void stopsRestartingAfterTheConfiguredLimit();
    void rejectsNonBaaDocuments();
};

void TestBaaLanguageClient::synchronizesDocumentsAndRejectsStaleDiagnostics()
{
    QTemporaryDir workspace(QStringLiteral("qalam-lsp-مسار-XXXXXX"));
    QVERIFY(workspace.isValid());
    const QString filePath = QDir(workspace.path()).filePath(QStringLiteral("رئيسي.baa"));

    BaaLanguageClient client;
    client.setServerProgram(QString::fromUtf8(QALAM_FAKE_BAA_LSP_PATH));
    client.setChangeDebounceInterval(0);

    int publicationCount = 0;
    int lastVersion = 0;
    int symbolVersion = 0;
    int semanticTokenVersion = 0;
    int foldingVersion = 0;
    int selectionVersion = 0;
    bool workspaceSymbolsReady = false;
    int completionVersion = 0;
    int hoverVersion = 0;
    int signatureVersion = 0;
    int definitionVersion = 0;
    int referencesVersion = 0;
    int codeActionVersion = 0;
    int formattingVersion = 0;
    int prepareRenameVersion = 0;
    int renameEditVersion = 0;
    QVector<Diagnostic> lastDiagnostics;
    QVector<BaaDocumentSymbol> lastSymbols;
    QVector<BaaSemanticToken> lastSemanticTokens;
    QVector<BaaFoldingRange> lastFoldingRanges;
    QVector<BaaSelectionRange> lastSelectionRanges;
    QVector<BaaWorkspaceSymbol> lastWorkspaceSymbols;
    QVector<BaaCompletionItem> lastCompletions;
    BaaHover lastHover;
    BaaSignatureHelp lastSignature;
    BaaLocation lastDefinition;
    QVector<BaaLocation> lastReferences;
    QVector<BaaCodeAction> lastCodeActions;
    BaaWorkspaceEdit lastFormattingEdit;
    QString renamePlaceholder;
    BaaLocation renameRange;
    BaaWorkspaceEdit lastWorkspaceEdit;
    connect(&client, &BaaLanguageClient::diagnosticsPublished, this,
            [&](const QString &publishedPath, int version,
                const QVector<Diagnostic> &diagnostics) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                ++publicationCount;
                lastVersion = version;
                lastDiagnostics = diagnostics;
            });
    connect(&client, &BaaLanguageClient::documentSymbolsPublished, this,
            [&](const QString &publishedPath, int version,
                const QVector<BaaDocumentSymbol> &symbols) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                symbolVersion = version;
                lastSymbols = symbols;
            });
    connect(&client, &BaaLanguageClient::semanticTokensPublished, this,
            [&](const QString &publishedPath, int version,
                const QVector<BaaSemanticToken> &tokens) {
                QCOMPARE(QDir::cleanPath(publishedPath),
                         QDir::cleanPath(filePath));
                semanticTokenVersion = version;
                lastSemanticTokens = tokens;
            });
    connect(&client, &BaaLanguageClient::foldingRangesPublished, this,
            [&](const QString &publishedPath, int version,
                const QVector<BaaFoldingRange> &ranges) {
                QCOMPARE(QDir::cleanPath(publishedPath),
                         QDir::cleanPath(filePath));
                foldingVersion = version;
                lastFoldingRanges = ranges;
            });
    connect(&client, &BaaLanguageClient::selectionRangesPublished, this,
            [&](const QString &publishedPath, int version,
                int line, int character,
                const QVector<BaaSelectionRange> &ranges) {
                QCOMPARE(QDir::cleanPath(publishedPath),
                         QDir::cleanPath(filePath));
                QCOMPARE(line, 1);
                QCOMPARE(character, 4);
                selectionVersion = version;
                lastSelectionRanges = ranges;
            });

    QCOMPARE(client.synchronizeDocument(filePath,
        QStringLiteral("صحيح الرئيسية() {\n    مفقود = ١.\n}\n"), 7, workspace.path()), 1);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), BaaLanguageClient::State::Ready, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(lastVersion, 1, 5000);
    QCOMPARE(lastDiagnostics.size(), 1);
    QCOMPARE(lastDiagnostics.first().line, 2);
    QCOMPARE(lastDiagnostics.first().column, 5);
    QCOMPARE(lastDiagnostics.first().endColumn, 10);
    QCOMPARE(lastDiagnostics.first().code, QStringLiteral("B1000"));
    QCOMPARE(lastDiagnostics.first().hint, QStringLiteral("عرّف الرمز"));
    QTRY_COMPARE_WITH_TIMEOUT(symbolVersion, 1, 5000);
    QCOMPARE(lastSymbols.size(), 1);
    QCOMPARE(lastSymbols.first().name, QStringLiteral("الرئيسية"));
    QCOMPARE(lastSymbols.first().kind, 12);
    QCOMPARE(lastSymbols.first().line, 1);
    QCOMPARE(lastSymbols.first().column, 6);
    QCOMPARE(client.documentSymbols(filePath).first().detail,
             QStringLiteral("-> صحيح"));

    QTRY_COMPARE_WITH_TIMEOUT(semanticTokenVersion, 1, 5000);
    QCOMPARE(lastSemanticTokens.size(), 3);
    QCOMPARE(lastSemanticTokens.first().line, 0);
    QCOMPARE(lastSemanticTokens.first().character, 0);
    QCOMPARE(lastSemanticTokens.first().length, 4);
    QCOMPARE(lastSemanticTokens.first().type, QStringLiteral("type"));
    QCOMPARE(lastSemanticTokens.at(1).line, 0);
    QCOMPARE(lastSemanticTokens.at(1).character, 5);
    QCOMPARE(lastSemanticTokens.at(1).length, 8);
    QCOMPARE(lastSemanticTokens.at(1).type, QStringLiteral("function"));
    QCOMPARE(lastSemanticTokens.last().line, 1);
    QCOMPARE(lastSemanticTokens.last().character, 12);
    QCOMPARE(lastSemanticTokens.last().length, 1);
    QCOMPARE(lastSemanticTokens.last().type, QStringLiteral("number"));

    QTRY_COMPARE_WITH_TIMEOUT(foldingVersion, 1, 5000);
    QCOMPARE(lastFoldingRanges.size(), 1);
    QCOMPARE(lastFoldingRanges.first().startLine, 0);
    QCOMPARE(lastFoldingRanges.first().startCharacter, 15);
    QCOMPARE(lastFoldingRanges.first().endLine, 2);
    QCOMPARE(lastFoldingRanges.first().endCharacter, 1);
    QCOMPARE(lastFoldingRanges.first().kind, QStringLiteral("region"));

    client.requestSelectionRanges(filePath, 1, 4);
    QTRY_COMPARE_WITH_TIMEOUT(selectionVersion, 1, 5000);
    QCOMPARE(lastSelectionRanges.size(), 4);
    QCOMPARE(lastSelectionRanges.first().line, 1);
    QCOMPARE(lastSelectionRanges.first().character, 4);
    QCOMPARE(lastSelectionRanges.first().endCharacter, 9);
    QCOMPARE(lastSelectionRanges.last().line, 0);
    QCOMPARE(lastSelectionRanges.last().endLine, 3);

    connect(&client, &BaaLanguageClient::workspaceSymbolsPublished, this,
            [&](const QString &query,
                const QVector<BaaWorkspaceSymbol> &symbols) {
                QCOMPARE(query, QStringLiteral("ر"));
                workspaceSymbolsReady = true;
                lastWorkspaceSymbols = symbols;
            });
    client.requestWorkspaceSymbols(QStringLiteral("ر"));
    QTRY_VERIFY_WITH_TIMEOUT(workspaceSymbolsReady, 5000);
    QCOMPARE(lastWorkspaceSymbols.size(), 2);
    const auto workspaceMain = std::ranges::find_if(
        lastWorkspaceSymbols,
        [](const BaaWorkspaceSymbol &symbol) {
            return symbol.name == QStringLiteral("الرئيسية");
        });
    QVERIFY(workspaceMain != lastWorkspaceSymbols.cend());
    QCOMPARE(QDir::cleanPath(workspaceMain->filePath),
             QDir::cleanPath(filePath));
    QCOMPARE(workspaceMain->kind, 12);
    QCOMPARE(workspaceMain->line, 0);
    QCOMPARE(workspaceMain->character, 5);
    QCOMPARE(workspaceMain->detail,
             QStringLiteral("صحيح الرئيسية()"));
    const auto workspaceField = std::ranges::find_if(
        lastWorkspaceSymbols,
        [](const BaaWorkspaceSymbol &symbol) {
            return symbol.name == QStringLiteral("قيمة_عضو");
        });
    QVERIFY(workspaceField != lastWorkspaceSymbols.cend());
    QCOMPARE(workspaceField->containerName, QStringLiteral("سجل"));
    QCOMPARE(workspaceField->kind, 8);

    bool workspaceSymbolsFailed = false;
    connect(
        &client,
        &BaaLanguageClient::workspaceSymbolsFailed,
        this,
        [&](const QString &query, int code, const QString &message) {
            QCOMPARE(query, QStringLiteral("خطأ"));
            QCOMPARE(code, -32801);
            QCOMPARE(message, QStringLiteral("Workspace index changed"));
            workspaceSymbolsFailed = true;
        });
    client.requestWorkspaceSymbols(QStringLiteral("خطأ"));
    QTRY_VERIFY_WITH_TIMEOUT(workspaceSymbolsFailed, 5000);

    connect(&client, &BaaLanguageClient::completionPublished, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const QVector<BaaCompletionItem> &items) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 0);
                QCOMPARE(character, 7);
                completionVersion = version;
                lastCompletions = items;
            });
    client.requestCompletion(filePath, 0, 7);
    QTRY_COMPARE_WITH_TIMEOUT(completionVersion, 1, 5000);
    QCOMPARE(lastCompletions.size(), 3);
    auto completionByLabel = [&](const QString &label) {
        return std::ranges::find_if(
            lastCompletions,
            [&](const BaaCompletionItem &item) {
                return item.label == label;
            });
    };
    const auto mainCompletion =
        completionByLabel(QStringLiteral("الرئيسية"));
    QVERIFY(mainCompletion != lastCompletions.cend());
    QCOMPARE(mainCompletion->detail, QStringLiteral("دالة ← صحيح"));
    QCOMPARE(mainCompletion->newText, QStringLiteral("الرئيسية"));
    QCOMPARE(mainCompletion->startCharacter, 5);
    QCOMPARE(mainCompletion->endCharacter, 7);
    QCOMPARE(mainCompletion->kind, 3);
    const auto localCompletion =
        completionByLabel(QStringLiteral("قيمة_محلية"));
    QVERIFY(localCompletion != lastCompletions.cend());
    QCOMPARE(localCompletion->detail,
             QStringLiteral("صحيح قيمة_محلية"));
    QCOMPARE(localCompletion->kind, 6);
    const auto includedCompletion =
        completionByLabel(QStringLiteral("من_واجهة"));
    QVERIFY(includedCompletion != lastCompletions.cend());
    QCOMPARE(includedCompletion->detail,
             QStringLiteral("صحيح من_واجهة()"));
    QCOMPARE(includedCompletion->kind, 3);

    connect(&client, &BaaLanguageClient::hoverPublished, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const BaaHover &hover) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 0);
                QCOMPARE(character, 5);
                hoverVersion = version;
                lastHover = hover;
            });
    client.requestHover(filePath, 0, 5);
    QTRY_COMPARE_WITH_TIMEOUT(hoverVersion, 1, 5000);
    QVERIFY(lastHover.isValid());
    QCOMPARE(lastHover.contentKind, QStringLiteral("markdown"));
    QVERIFY(lastHover.contents.contains(
        QStringLiteral("صحيح اجمع(صحيح أول، صحيح ثان)")));
    QCOMPARE(lastHover.startCharacter, 5);
    QCOMPARE(lastHover.endCharacter, 9);

    connect(&client, &BaaLanguageClient::signatureHelpPublished, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const BaaSignatureHelp &signatureHelp) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 1);
                QCOMPARE(character, 12);
                signatureVersion = version;
                lastSignature = signatureHelp;
            });
    client.requestSignatureHelp(filePath, 1, 12);
    QTRY_COMPARE_WITH_TIMEOUT(signatureVersion, 1, 5000);
    QVERIFY(lastSignature.isValid());
    QCOMPARE(lastSignature.label,
             QStringLiteral("صحيح اجمع(صحيح أول، صحيح ثان)"));
    QCOMPARE(lastSignature.parameters,
             QStringList({QStringLiteral("صحيح أول"),
                          QStringLiteral("صحيح ثان")}));
    QCOMPARE(lastSignature.activeParameter, 1);

    connect(&client, &BaaLanguageClient::definitionPublished, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const BaaLocation &definition) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 1);
                QCOMPARE(character, 10);
                definitionVersion = version;
                lastDefinition = definition;
            });
    client.requestDefinition(filePath, 1, 10);
    QTRY_COMPARE_WITH_TIMEOUT(definitionVersion, 1, 5000);
    QVERIFY(lastDefinition.isValid());
    QCOMPARE(QDir::cleanPath(lastDefinition.filePath), QDir::cleanPath(filePath));
    QCOMPARE(lastDefinition.line, 0);
    QCOMPARE(lastDefinition.character, 5);
    QCOMPARE(lastDefinition.endCharacter, 9);

    connect(&client, &BaaLanguageClient::referencesPublished, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const QVector<BaaLocation> &references) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 1);
                QCOMPARE(character, 10);
                referencesVersion = version;
                lastReferences = references;
            });
    client.requestReferences(filePath, 1, 10, true);
    QTRY_COMPARE_WITH_TIMEOUT(referencesVersion, 1, 5000);
    QCOMPARE(lastReferences.size(), 2);
    QCOMPARE(lastReferences.first().line, 0);
    QCOMPARE(lastReferences.last().line, 1);

    connect(&client, &BaaLanguageClient::codeActionsPublished, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const QVector<BaaCodeAction> &actions) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 1);
                QCOMPARE(character, 4);
                codeActionVersion = version;
                lastCodeActions = actions;
            });
    client.requestCodeActions(filePath, 1, 4);
    QTRY_COMPARE_WITH_TIMEOUT(codeActionVersion, 1, 5000);
    QCOMPARE(lastCodeActions.size(), 1);
    QVERIFY(lastCodeActions.first().isValid());
    QCOMPARE(lastCodeActions.first().id,
             QStringLiteral("B1000.insert-int-type"));
    QCOMPARE(lastCodeActions.first().title,
             QStringLiteral("عرّف المتغير بإضافة نوعه"));
    QVERIFY(lastCodeActions.first().preferred);
    QCOMPARE(lastCodeActions.first().edit.documents.size(), 1);
    QCOMPARE(lastCodeActions.first().edit.documents.first().version, 1);
    QCOMPARE(lastCodeActions.first().edit.documents.first().edits.size(), 1);
    QCOMPARE(lastCodeActions.first().edit.documents.first().edits.first().newText,
             QStringLiteral("صحيح "));

    connect(&client, &BaaLanguageClient::formattingPublished, this,
            [&](const QString &publishedPath, int version,
                const BaaWorkspaceEdit &edit) {
                QCOMPARE(QDir::cleanPath(publishedPath),
                         QDir::cleanPath(filePath));
                formattingVersion = version;
                lastFormattingEdit = edit;
            });
    client.requestFormatting(filePath);
    QTRY_COMPARE_WITH_TIMEOUT(formattingVersion, 1, 5000);
    QVERIFY(lastFormattingEdit.isValid());
    QCOMPARE(lastFormattingEdit.documents.size(), 1);
    QCOMPARE(lastFormattingEdit.documents.first().version, 1);
    QCOMPARE(lastFormattingEdit.editCount(), 1);
    const BaaTextEdit formattingEdit =
        lastFormattingEdit.documents.first().edits.first();
    QCOMPARE(formattingEdit.line, 0);
    QCOMPARE(formattingEdit.endLine, 3);
    QCOMPARE(formattingEdit.newText,
             QStringLiteral("صحيح الرئيسية() {\n    أرجع ٠.\n}\n"));

    connect(&client, &BaaLanguageClient::renamePrepared, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const QString &placeholder, const BaaLocation &range) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 1);
                QCOMPARE(character, 10);
                prepareRenameVersion = version;
                renamePlaceholder = placeholder;
                renameRange = range;
            });
    client.requestPrepareRename(filePath, 1, 10);
    QTRY_COMPARE_WITH_TIMEOUT(prepareRenameVersion, 1, 5000);
    QCOMPARE(renamePlaceholder, QStringLiteral("اجمع"));
    QVERIFY(renameRange.isValid());
    QCOMPARE(renameRange.line, 1);
    QCOMPARE(renameRange.character, 8);

    connect(&client, &BaaLanguageClient::renameEditPublished, this,
            [&](const QString &publishedPath, int version, int line, int character,
                const BaaWorkspaceEdit &edit) {
                QCOMPARE(QDir::cleanPath(publishedPath), QDir::cleanPath(filePath));
                QCOMPARE(line, 1);
                QCOMPARE(character, 10);
                renameEditVersion = version;
                lastWorkspaceEdit = edit;
            });
    client.requestRename(filePath, 1, 10, QStringLiteral("احسب"));
    QTRY_COMPARE_WITH_TIMEOUT(renameEditVersion, 1, 5000);
    QVERIFY(lastWorkspaceEdit.isValid());
    QCOMPARE(lastWorkspaceEdit.documents.size(), 1);
    QCOMPARE(lastWorkspaceEdit.editCount(), 2);
    QCOMPARE(lastWorkspaceEdit.documents.first().version, 1);
    QCOMPARE(lastWorkspaceEdit.documents.first().edits.first().newText,
             QStringLiteral("احسب"));

    QCOMPARE(client.synchronizeDocument(filePath,
        QStringLiteral("صحيح الرئيسية() {\n    مفقود = ٢.\n}\n"), 8, workspace.path()), 2);
    QTRY_COMPARE_WITH_TIMEOUT(lastVersion, 2, 5000);
    QCOMPARE(publicationCount, 2);
    QTRY_COMPARE_WITH_TIMEOUT(symbolVersion, 2, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(semanticTokenVersion, 2, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(foldingVersion, 2, 5000);

    client.closeDocument(filePath);
    client.stop();
    QTRY_COMPARE_WITH_TIMEOUT(client.state(), BaaLanguageClient::State::Stopped, 5000);
}

void TestBaaLanguageClient::rejectsNonBaaDocuments()
{
    BaaLanguageClient client;
    QCOMPARE(client.synchronizeDocument("notes.txt", "text", 1), 0);
    QCOMPARE(client.state(), BaaLanguageClient::State::Stopped);
}

void TestBaaLanguageClient::restartsAfterUnexpectedExitAndReopensDocuments()
{
    QTemporaryDir workspace(QStringLiteral("qalam-lsp-restart-XXXXXX"));
    QVERIFY(workspace.isValid());
    const QString filePath =
        QDir(workspace.path()).filePath(QStringLiteral("رئيسي.baa"));
    const QString markerPath =
        QDir(workspace.path()).filePath(QStringLiteral("crash-once.marker"));
    QVERIFY(qputenv("QALAM_FAKE_LSP_CRASH_ONCE_MARKER",
                    markerPath.toUtf8()));
    const auto clearEnvironment = qScopeGuard([]() {
        qunsetenv("QALAM_FAKE_LSP_CRASH_ONCE_MARKER");
    });

    BaaLanguageClient client;
    client.setServerProgram(QString::fromUtf8(QALAM_FAKE_BAA_LSP_PATH));
    client.setRestartPolicy(3, 10, 10000);

    int restartCount = 0;
    int publishedVersion = 0;
    QStringList logs;
    connect(&client, &BaaLanguageClient::stateChanged, this,
            [&](BaaLanguageClient::State state) {
                if (state == BaaLanguageClient::State::Restarting) {
                    ++restartCount;
                }
            });
    connect(&client, &BaaLanguageClient::diagnosticsPublished, this,
            [&](const QString &publishedPath, int version,
                const QVector<Diagnostic> &) {
                QCOMPARE(QDir::cleanPath(publishedPath),
                         QDir::cleanPath(filePath));
                publishedVersion = version;
            });
    connect(&client, &BaaLanguageClient::logMessage, this,
            [&](const QString &message, int) { logs.push_back(message); });

    QCOMPARE(client.synchronizeDocument(
                 filePath,
                 QStringLiteral("صحيح الرئيسية() {\n    مفقود = ١.\n}\n"),
                 1,
                 workspace.path()),
             1);
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(markerPath), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(restartCount, 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(publishedVersion, 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(),
                              BaaLanguageClient::State::Ready,
                              5000);
    QVERIFY(std::any_of(logs.cbegin(), logs.cend(), [](const QString &message) {
        return message.contains(QStringLiteral("المحاولة 1 من 3"));
    }));

    client.stop();
    QTRY_COMPARE_WITH_TIMEOUT(client.state(),
                              BaaLanguageClient::State::Stopped,
                              5000);
}

void TestBaaLanguageClient::stopsRestartingAfterTheConfiguredLimit()
{
    QTemporaryDir workspace(QStringLiteral("qalam-lsp-limit-XXXXXX"));
    QVERIFY(workspace.isValid());
    const QString filePath =
        QDir(workspace.path()).filePath(QStringLiteral("رئيسي.baa"));
    QVERIFY(qputenv("QALAM_FAKE_LSP_ALWAYS_CRASH", "1"));
    const auto clearEnvironment = qScopeGuard([]() {
        qunsetenv("QALAM_FAKE_LSP_ALWAYS_CRASH");
    });

    BaaLanguageClient client;
    client.setServerProgram(QString::fromUtf8(QALAM_FAKE_BAA_LSP_PATH));
    client.setRestartPolicy(2, 10, 10000);

    int restartCount = 0;
    QStringList logs;
    connect(&client, &BaaLanguageClient::stateChanged, this,
            [&](BaaLanguageClient::State state) {
                if (state == BaaLanguageClient::State::Restarting) {
                    ++restartCount;
                }
            });
    connect(&client, &BaaLanguageClient::logMessage, this,
            [&](const QString &message, int) { logs.push_back(message); });

    QCOMPARE(client.synchronizeDocument(
                 filePath,
                 QStringLiteral("صحيح الرئيسية() {\n    مفقود = ١.\n}\n"),
                 1,
                 workspace.path()),
             1);
    QTRY_COMPARE_WITH_TIMEOUT(client.state(),
                              BaaLanguageClient::State::Error,
                              5000);
    QCOMPARE(restartCount, 2);
    QVERIFY(std::any_of(logs.cbegin(), logs.cend(), [](const QString &message) {
        return message.contains(QStringLiteral("حد إعادة التشغيل (2 محاولات)"));
    }));

    client.stop();
    QCOMPARE(client.state(), BaaLanguageClient::State::Stopped);
}

QTEST_MAIN(TestBaaLanguageClient)
#include "TestBaaLanguageClient.moc"
