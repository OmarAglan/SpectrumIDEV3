#include "QalamSearchView.h"

#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPixmap>
#include <QPushButton>
#include <QtTest>

class TestSearchView : public QObject {
    Q_OBJECT

private slots:
    void exposesVisibleSearchOptionIcons();
    void exposesArabicReplaceAndProgressWorkflow();
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

void TestSearchView::exposesArabicReplaceAndProgressWorkflow()
{
    QalamSearchView view;
    view.resize(280, 420);
    view.show();
    QCoreApplication::processEvents();

    auto *toggle = view.findChild<QPushButton *>(
        QStringLiteral("toggleReplaceBtn"));
    auto *replaceRow = view.findChild<QWidget *>(
        QStringLiteral("projectReplaceRow"));
    auto *replaceButton = view.findChild<QPushButton *>(
        QStringLiteral("projectReplaceAllButton"));
    auto *searchInput = view.findChild<QLineEdit *>(
        QStringLiteral("searchInput"));
    auto *replaceInput = view.findChild<QLineEdit *>(
        QStringLiteral("replaceInput"));
    auto *summary = view.findChild<QLabel *>(
        QStringLiteral("resultSummary"));
    QVERIFY(toggle);
    QVERIFY(replaceRow);
    QVERIFY(replaceButton);
    QVERIFY(searchInput);
    QVERIFY(replaceInput);
    QVERIFY(summary);
    QVERIFY(not replaceRow->isVisibleTo(&view));

    toggle->click();
    QVERIFY(replaceRow->isVisibleTo(&view));
    QVERIFY(replaceButton->isVisibleTo(&view));
    QVERIFY(not replaceButton->icon().isNull());
    QVERIFY(replaceButton->text().isEmpty());
    QVERIFY(replaceButton->accessibleName().contains(
        QStringLiteral("استبدال")));

    QSignalSpy replaceSpy(&view, &QalamSearchView::replaceRequested);
    searchInput->setText(QStringLiteral("س"));
    replaceInput->setText(QStringLiteral("ص"));
    replaceButton->click();
    QCOMPARE(replaceSpy.count(), 1);
    const QList<QVariant> replaceArguments = replaceSpy.takeFirst();
    QCOMPARE(replaceArguments.at(0).toString(), QStringLiteral("س"));
    QCOMPARE(replaceArguments.at(1).toString(), QStringLiteral("ص"));

    QSignalSpy searchSpy(&view, &QalamSearchView::searchRequested);
    QTest::keyClick(searchInput, Qt::Key_Return);
    QCOMPARE(searchSpy.count(), 1);
    QCOMPARE(searchSpy.constFirst().at(0).toString(), QStringLiteral("س"));

    view.setSearchProgress(16, 32);
    const QLocale arabic(QLocale::Arabic, QLocale::SaudiArabia);
    QVERIFY(summary->isVisibleTo(&view));
    QVERIFY(summary->text().contains(arabic.toString(16)));
    QVERIFY(summary->text().contains(arabic.toString(32)));
    QVERIFY(summary->text().contains(QStringLiteral("البحث")));

    const QString screenshotPath =
        qEnvironmentVariable("QALAM_PROJECT_REPLACE_SCREENSHOT");
    if (not screenshotPath.isEmpty())
        QVERIFY(view.grab().save(screenshotPath));
}

QTEST_MAIN(TestSearchView)
#include "TestSearchView.moc"
