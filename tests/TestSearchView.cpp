#include "QalamSearchView.h"

#include <QIcon>
#include <QPixmap>
#include <QPushButton>
#include <QtTest>

class TestSearchView : public QObject {
    Q_OBJECT

private slots:
    void exposesVisibleSearchOptionIcons();
};

void TestSearchView::exposesVisibleSearchOptionIcons()
{
    QalamSearchView view;
    view.resize(280, 420);
    view.show();
    QCoreApplication::processEvents();

    for (const QString &objectName : {
             QStringLiteral("caseSensitiveButton"),
             QStringLiteral("wholeWordButton"),
             QStringLiteral("regexButton")}) {
        auto *button = view.findChild<QPushButton *>(objectName);
        QVERIFY2(button, qPrintable(QStringLiteral("Missing button: %1").arg(objectName)));
        QVERIFY2(button->isVisibleTo(&view),
                 qPrintable(QStringLiteral("Hidden button: %1").arg(objectName)));
        QVERIFY2(not button->icon().isNull(),
                 qPrintable(QStringLiteral("Missing icon: %1").arg(objectName)));
        QVERIFY(button->text().isEmpty());
        QVERIFY(view.rect().contains(button->geometry().center()));

        const QPixmap normalPixmap = button->icon().pixmap(button->iconSize());
        QVERIFY2(not normalPixmap.isNull(),
                 qPrintable(QStringLiteral("Unrenderable icon: %1").arg(objectName)));

        const qint64 normalCacheKey = button->icon().cacheKey();
        button->click();
        QVERIFY(button->isChecked());
        const QPixmap checkedPixmap = button->icon().pixmap(button->iconSize());
        QVERIFY2(not checkedPixmap.isNull(),
                 qPrintable(QStringLiteral("Missing checked icon: %1").arg(objectName)));
        QVERIFY(button->icon().cacheKey() != normalCacheKey);
    }

    const QString screenshotPath =
        qEnvironmentVariable("QALAM_SEARCH_VIEW_SCREENSHOT");
    if (not screenshotPath.isEmpty()) {
        QVERIFY(view.grab().save(screenshotPath));
    }
}

QTEST_MAIN(TestSearchView)
#include "TestSearchView.moc"
