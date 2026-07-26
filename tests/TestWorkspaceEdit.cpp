#include "BaaWorkspaceEdit.h"

#include <QTest>

class TestWorkspaceEdit : public QObject
{
    Q_OBJECT
private slots:
    void appliesArabicUtf16EditsInDescendingOrder();
    void rejectsOverlappingAndOutOfRangeEdits();
};

void TestWorkspaceEdit::appliesArabicUtf16EditsInDescendingOrder()
{
    const QString source = QStringLiteral(
        "صحيح اجمع() {\n"
        "    إرجع اجمع().\n"
        "}\n");
    const QVector<BaaTextEdit> edits{
        {0, 5, 0, 9, QStringLiteral("احسب")},
        {1, 9, 1, 13, QStringLiteral("احسب")}
    };
    QString updated;
    QVector<BaaTextEdit> ordered;
    QString error;
    QVERIFY(applyBaaTextEdits(
        source, edits, &updated, &ordered, &error));
    QVERIFY(error.isEmpty());
    QCOMPARE(ordered.first().line, 1);
    QCOMPARE(updated, QStringLiteral(
        "صحيح احسب() {\n"
        "    إرجع احسب().\n"
        "}\n"));

    const QString supplementary =
        QString::fromUtf8("😀 اجمع");
    QCOMPARE(baaUtf16TextOffset(supplementary, 0, 3), 3);
}

void TestWorkspaceEdit::rejectsOverlappingAndOutOfRangeEdits()
{
    const QString source = QStringLiteral("صحيح اجمع().\n");
    QString updated;
    QString error;
    QVERIFY(not applyBaaTextEdits(
        source,
        {
            {0, 5, 0, 9, QStringLiteral("احسب")},
            {0, 7, 0, 9, QStringLiteral("بديل")}
        },
        &updated, nullptr, &error));
    QVERIFY(not error.isEmpty());

    error.clear();
    QVERIFY(not applyBaaTextEdits(
        source,
        {{4, 0, 4, 1, QStringLiteral("س")}},
        &updated, nullptr, &error));
    QVERIFY(not error.isEmpty());
}

QTEST_MAIN(TestWorkspaceEdit)
#include "TestWorkspaceEdit.moc"
