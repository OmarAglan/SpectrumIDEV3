#include "texteditor/autocomplete/QalamCompletionHistory.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

class TestCompletionHistory : public QObject
{
    Q_OBJECT

private slots:
    void ranksOnlyEqualServerBucketsFromActualSelections();
    void isolatesContextsAndClearsStoredUsage();
};

void TestCompletionHistory::ranksOnlyEqualServerBucketsFromActualSelections()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QSettings settings(temporary.filePath(QStringLiteral("history.ini")),
                       QSettings::IniFormat);

    CompletionItem first;
    first.label = QStringLiteral("أول");
    first.serverSortText = QStringLiteral("00000010");
    first.context = QStringLiteral("call-argument");
    first.stableKey = QStringLiteral("variable:أول");

    CompletionItem frequent = first;
    frequent.label = QStringLiteral("ثان");
    frequent.stableKey = QStringLiteral("variable:ثان");

    CompletionItem stronger = first;
    stronger.label = QStringLiteral("أقوى");
    stronger.serverSortText = QStringLiteral("00000005");
    stronger.stableKey = QStringLiteral("variable:أقوى");

    QalamCompletionHistory::record(
        settings, frequent.context, frequent.stableKey, 10);
    QalamCompletionHistory::record(
        settings, frequent.context, frequent.stableKey, 20);

    std::vector<CompletionItem> items{first, frequent, stronger};
    QalamCompletionHistory::rank(items, settings);
    QCOMPARE(items.at(0).label, stronger.label);
    QCOMPARE(items.at(1).label, frequent.label);
    QCOMPARE(items.at(2).label, first.label);
}

void TestCompletionHistory::isolatesContextsAndClearsStoredUsage()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QSettings settings(temporary.filePath(QStringLiteral("history.ini")),
                       QSettings::IniFormat);

    QalamCompletionHistory::record(
        settings, QStringLiteral("statement"),
        QStringLiteral("variable:قيمة"), 10);
    QCOMPARE(QalamCompletionHistory::selectionCount(
                 settings, QStringLiteral("statement"),
                 QStringLiteral("variable:قيمة")), 1);
    QCOMPARE(QalamCompletionHistory::selectionCount(
                 settings, QStringLiteral("call-argument"),
                 QStringLiteral("variable:قيمة")), 0);

    QalamCompletionHistory::clear(settings);
    QCOMPARE(QalamCompletionHistory::selectionCount(
                 settings, QStringLiteral("statement"),
                 QStringLiteral("variable:قيمة")), 0);
}

QTEST_MAIN(TestCompletionHistory)
#include "TestCompletionHistory.moc"
