#include "QalamPanelArea.h"

#include <QTest>

class TestPanelArea : public QObject
{
    Q_OBJECT

private slots:
    void keepsBoundedPlainTextOutput();
    void deduplicatesConsecutiveLanguageEvents();
    void loadsPanelActionIcons();
};

void TestPanelArea::keepsBoundedPlainTextOutput()
{
    QalamPanelArea panel;
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

void TestPanelArea::deduplicatesConsecutiveLanguageEvents()
{
    QalamPanelArea panel;
    const QString failure = QStringLiteral("[خطأ] فشل تحليل مصرف باء.\n");
    const QString recovery = QStringLiteral("[معلومة] استعاد خادم اللغة عمله.\n");

    panel.appendOutput(failure);
    panel.appendOutput(failure);
    QCOMPARE(panel.outputText().count(QStringLiteral("فشل تحليل مصرف باء.")), 1);

    panel.appendOutput(recovery);
    panel.appendOutput(failure);
    QCOMPARE(panel.outputText().count(QStringLiteral("فشل تحليل مصرف باء.")), 2);

    panel.clearOutput();
    panel.appendOutput(failure);
    QCOMPARE(panel.outputText(), failure);
}

void TestPanelArea::loadsPanelActionIcons()
{
    QalamPanelArea panel;
    const auto requireIcon = [&panel](const QString &objectName) {
        auto *button = panel.findChild<QPushButton *>(objectName);
        QVERIFY2(button, qPrintable(QStringLiteral("Missing button: %1").arg(objectName)));
        QVERIFY2(not button->icon().isNull(),
                 qPrintable(QStringLiteral("Missing icon: %1").arg(objectName)));
    };

    requireIcon(QStringLiteral("panelMaximizeButton"));
    requireIcon(QStringLiteral("panelCloseButton"));
    requireIcon(QStringLiteral("debugRunButton"));
    requireIcon(QStringLiteral("debugStopButton"));
}

QTEST_MAIN(TestPanelArea)
#include "TestPanelArea.moc"
