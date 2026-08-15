#include "TPanelArea.h"

#include <QTest>

class TestPanelArea : public QObject
{
    Q_OBJECT

private slots:
    void keepsBoundedPlainTextOutput();
};

void TestPanelArea::keepsBoundedPlainTextOutput()
{
    TPanelArea panel;
    panel.appendOutput(QStringLiteral("<b>نص خادم اللغة</b>\n"));
    QVERIFY(panel.outputText().contains(
        QStringLiteral("<b>نص خادم اللغة</b>")));

    for (int index = 0; index < 650; ++index) {
        panel.appendOutput(QStringLiteral("سطر %1\n").arg(index));
    }
    QVERIFY(panel.outputBlockCount() <= 500);
    QVERIFY(not panel.outputText().contains(QStringLiteral("سطر 0\n")));
    QVERIFY(panel.outputText().contains(QStringLiteral("سطر 649")));

    panel.clearOutput();
    QVERIFY(panel.outputText().isEmpty());
    QCOMPARE(panel.outputBlockCount(), 1);
}

QTEST_MAIN(TestPanelArea)
#include "TestPanelArea.moc"
