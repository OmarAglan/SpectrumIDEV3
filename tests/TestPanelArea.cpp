#include "QalamPanelArea.h"
#include "Constants.h"

#include <QTest>
#include <QSplitter>

class TestPanelArea : public QObject
{
    Q_OBJECT

private slots:
    void keepsBoundedPlainTextOutput();
    void deduplicatesConsecutiveLanguageEvents();
    void loadsPanelActionIcons();
    void presentsHelpfulEmptyStates();
    void reflectsPanelMaximizeState();
    void opensAtRestrainedDefaultHeight();
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

void TestPanelArea::presentsHelpfulEmptyStates()
{
    QalamPanelArea panel;
    auto *problemsStack = panel.findChild<QStackedWidget *>(
        QStringLiteral("problemsStack"));
    auto *outputStack = panel.findChild<QStackedWidget *>(
        QStringLiteral("outputStack"));
    auto *problemsEmpty = panel.findChild<QLabel *>(
        QStringLiteral("problemsEmptyState"));
    auto *outputEmpty = panel.findChild<QLabel *>(
        QStringLiteral("outputEmptyState"));
    QVERIFY(problemsStack);
    QVERIFY(outputStack);
    QVERIFY(problemsEmpty);
    QVERIFY(outputEmpty);
    QCOMPARE(problemsStack->currentWidget(), problemsEmpty);
    QCOMPARE(outputStack->currentWidget(), outputEmpty);

    panel.addProblem(QStringLiteral("مشكلة تجريبية"),
                     QStringLiteral("مثال.باء"), 2, 4);
    QVERIFY(problemsStack->currentWidget() != problemsEmpty);
    panel.appendOutput(QStringLiteral("رسالة تجريبية\n"));
    QVERIFY(outputStack->currentWidget() != outputEmpty);

    panel.clearProblems();
    panel.clearOutput();
    QCOMPARE(problemsStack->currentWidget(), problemsEmpty);
    QCOMPARE(outputStack->currentWidget(), outputEmpty);
}

void TestPanelArea::reflectsPanelMaximizeState()
{
    QalamPanelArea panel;
    auto *button = panel.findChild<QPushButton *>(
        QStringLiteral("panelMaximizeButton"));
    QVERIFY(button);

    panel.setMaximizedState(true);
    QCOMPARE(button->toolTip(), QStringLiteral("استعادة الحجم"));
    QVERIFY(not button->icon().isNull());

    panel.setMaximizedState(false);
    QCOMPARE(button->toolTip(), QStringLiteral("تكبير"));
    QVERIFY(not button->icon().isNull());
}

void TestPanelArea::opensAtRestrainedDefaultHeight()
{
    QSplitter splitter(Qt::Vertical);
    splitter.resize(1000, 800);
    splitter.addWidget(new QWidget(&splitter));
    auto *panel = new QalamPanelArea(&splitter);
    splitter.addWidget(panel);
    panel->hide();
    splitter.show();
    QTest::qWait(20);

    panel->show();
    QTest::qWait(30);
    QVERIFY(qAbs(panel->height() - Constants::Layout::PanelDefaultHeight) <= 2);
}

QTEST_MAIN(TestPanelArea)
#include "TestPanelArea.moc"
