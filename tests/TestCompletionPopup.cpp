#include "texteditor/autocomplete/AutoCompleteUI.h"

#include <QLabel>
#include <QTest>

class TestCompletionPopup : public QObject
{
    Q_OBJECT

private slots:
    void showsTheSelectedCompletionDescription();
};

void TestCompletionPopup::showsTheSelectedCompletionDescription()
{
    CompletionModel model;
    CompletionItem item;
    item.label = QStringLiteral("اطبع");
    item.completion = item.label;
    item.description = QStringLiteral("يطبع النص أو القيمة في نافذة المخرجات.");
    item.type = Function;
    model.updateData({item});

    QalamCompletionPopup popup;
    popup.resize(340, 190);
    popup.setModel(&model);
    popup.setCurrentIndex(model.index(0, 0));
    QCoreApplication::processEvents();

    QLabel *footer = popup.findChild<QLabel *>(
        QStringLiteral("completionInfoLabel"));
    QVERIFY(footer != nullptr);
    QVERIFY(footer->text().contains(item.description));
    QVERIFY(footer->height() >= 72);
    QVERIFY(footer->geometry().top() >= popup.contentsRect().top());
    QVERIFY(footer->geometry().bottom() <= popup.contentsRect().bottom());
    QCOMPARE(popup.property("qalam.completionFooterHeight").toInt(),
             footer->height());
}

QTEST_MAIN(TestCompletionPopup)
#include "TestCompletionPopup.moc"
