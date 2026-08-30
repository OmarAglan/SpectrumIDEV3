#include "QalamTitleBar.h"

#include <QApplication>
#include <QMenuBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

class TestTitleBar final : public QObject
{
    Q_OBJECT

private slots:
    void adaptsCommandCenterToAvailableWidth();
    void commandCenterStillOpensOnClick();
    void doubleClickRequestsMaximizeRestore();
};

void TestTitleBar::adaptsCommandCenterToAvailableWidth()
{
    QalamTitleBar titleBar;
    auto *menuBar = new QMenuBar;
    for (const QString &title : {
             QStringLiteral("ملف"), QStringLiteral("تحرير"),
             QStringLiteral("عرض"), QStringLiteral("تشغيل"),
             QStringLiteral("الطرفية"), QStringLiteral("مساعدة")}) {
        menuBar->addMenu(title);
    }
    titleBar.addMenuBar(menuBar);
    auto *commandCenter = titleBar.findChild<QPushButton*>(
        QStringLiteral("commandCenterButton"));
    QVERIFY(commandCenter);

    titleBar.resize(1800, titleBar.height());
    titleBar.show();
    QApplication::processEvents();
    QVERIFY(commandCenter->isVisible());

    titleBar.resize(680, titleBar.height());
    QApplication::processEvents();
    QVERIFY2(not commandCenter->isVisible(),
             qPrintable(QStringLiteral("العرض الفعلي: %1؛ مركز الأوامر: %2")
                            .arg(titleBar.width())
                            .arg(commandCenter->geometry().width())));
    QVERIFY(menuBar->width() < 500);
}

void TestTitleBar::commandCenterStillOpensOnClick()
{
    QalamTitleBar titleBar;
    titleBar.resize(1400, titleBar.height());
    titleBar.show();
    QApplication::processEvents();

    auto *commandCenter = titleBar.findChild<QPushButton*>(
        QStringLiteral("commandCenterButton"));
    QVERIFY(commandCenter);
    QVERIFY(commandCenter->isVisible());
    QSignalSpy requested(&titleBar, &QalamTitleBar::commandCenterClicked);

    QTest::mouseClick(commandCenter, Qt::LeftButton);
    QCOMPARE(requested.count(), 1);
}

void TestTitleBar::doubleClickRequestsMaximizeRestore()
{
    QalamTitleBar titleBar;
    titleBar.resize(900, titleBar.height());
    titleBar.show();
    QSignalSpy requested(&titleBar,
                         &QalamTitleBar::maximizeRestoreClicked);
    QTest::mouseDClick(&titleBar, Qt::LeftButton, Qt::NoModifier,
                       QPoint(100, titleBar.height() / 2));
    QCOMPARE(requested.count(), 1);
}

QTEST_MAIN(TestTitleBar)
#include "TestTitleBar.moc"
