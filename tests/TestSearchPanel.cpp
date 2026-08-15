#include "QalamSearchPanel.h"
#include "QalamEditor.h"

#include <QCheckBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QtTest>

class TestSearchPanel : public QObject {
    Q_OBJECT

private slots:
    void loadsSearchActionIcons();
    void findsArabicTextAndWrapsNavigation();
    void supportsArabicWholeWordsAndRegularExpressions();
    void replacesOneAndAllAsUndoableEdits();
    void expandsRegularExpressionCaptures();
    void preservesDiagnosticDecorations();
    void keepsClosedSearchHighlightsInactiveAcrossEditors();
    void rejectsInvalidAndZeroLengthPatternsSafely();
    void rendersCompactRtlLayout();
};

void TestSearchPanel::loadsSearchActionIcons()
{
    for (const QString &resourcePath : {
             QStringLiteral(":/icons/resources/search.svg"),
             QStringLiteral(":/icons/resources/trash.svg")}) {
        QVERIFY2(not QIcon(resourcePath).isNull(),
                 qPrintable(QStringLiteral("Missing resource icon: %1").arg(resourcePath)));
    }

    QalamSearchPanel panel;
    for (const QString &objectName : {
             QStringLiteral("searchCloseButton"),
             QStringLiteral("searchNextButton"),
             QStringLiteral("searchPreviousButton"),
             QStringLiteral("replaceButton"),
             QStringLiteral("replaceAllButton")}) {
        auto *button = panel.findChild<QPushButton *>(objectName);
        QVERIFY2(button, qPrintable(QStringLiteral("Missing button: %1").arg(objectName)));
        QVERIFY2(not button->icon().isNull(),
                 qPrintable(QStringLiteral("Missing icon: %1").arg(objectName)));
    }
}

void TestSearchPanel::findsArabicTextAndWrapsNavigation()
{
    QalamEditor editor;
    editor.setPlainText(QStringLiteral("متغير قيمة متغير\nمتغير"));

    QalamSearchPanel panel;
    panel.setEditor(&editor);
    panel.show();

    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    auto *nextButton = panel.findChild<QPushButton *>(QStringLiteral("searchNextButton"));
    auto *previousButton =
        panel.findChild<QPushButton *>(QStringLiteral("searchPreviousButton"));
    QVERIFY(searchInput);
    QVERIFY(nextButton);
    QVERIFY(previousButton);

    searchInput->setText(QStringLiteral("متغير"));
    QCOMPARE(panel.matchCount(), 3);
    QCOMPARE(panel.currentMatchNumber(), 1);
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("متغير"));
    QCOMPARE(editor.searchHighlightCount(), 3);

    nextButton->click();
    QCOMPARE(panel.currentMatchNumber(), 2);
    previousButton->click();
    QCOMPARE(panel.currentMatchNumber(), 1);
    previousButton->click();
    QCOMPARE(panel.currentMatchNumber(), 3);
}

void TestSearchPanel::supportsArabicWholeWordsAndRegularExpressions()
{
    QalamEditor editor;
    editor.setPlainText(QStringLiteral("سجل سجل_ثان سجل2 مسجل سجل سطر"));

    QalamSearchPanel panel;
    panel.setEditor(&editor);
    panel.show();

    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    auto *wordCheck = panel.findChild<QCheckBox *>(QStringLiteral("searchWholeWord"));
    auto *regexCheck = panel.findChild<QCheckBox *>(QStringLiteral("searchRegex"));
    QVERIFY(searchInput);
    QVERIFY(wordCheck);
    QVERIFY(regexCheck);

    wordCheck->setChecked(true);
    searchInput->setText(QStringLiteral("سجل"));
    QCOMPARE(panel.matchCount(), 2);

    editor.setPlainText(QStringLiteral("س سَجل"));
    searchInput->setText(QStringLiteral("س"));
    QCOMPARE(panel.matchCount(), 1);

    editor.setPlainText(QStringLiteral("سجل سجل_ثان سجل2 مسجل سجل سطر"));
    wordCheck->setChecked(false);
    regexCheck->setChecked(true);
    searchInput->setText(QStringLiteral("س(جل|طر)"));
    QCOMPARE(panel.matchCount(), 6);
    QVERIFY(panel.hasValidPattern());
}

void TestSearchPanel::replacesOneAndAllAsUndoableEdits()
{
    QalamEditor editor;
    editor.setPlainText(QStringLiteral("قيمة = قيمة.\nقيمة."));

    QalamSearchPanel panel;
    panel.setEditor(&editor);
    panel.show();

    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    auto *replaceInput = panel.findChild<QLineEdit *>(QStringLiteral("replaceInput"));
    auto *replaceButton = panel.findChild<QPushButton *>(QStringLiteral("replaceButton"));
    auto *replaceAllButton =
        panel.findChild<QPushButton *>(QStringLiteral("replaceAllButton"));
    QVERIFY(searchInput);
    QVERIFY(replaceInput);
    QVERIFY(replaceButton);
    QVERIFY(replaceAllButton);

    searchInput->setText(QStringLiteral("قيمة"));
    replaceInput->setText(QStringLiteral("عدد"));
    replaceButton->click();
    QCOMPARE(editor.toPlainText(), QStringLiteral("عدد = قيمة.\nقيمة."));
    QCOMPARE(panel.matchCount(), 2);

    replaceAllButton->click();
    QCOMPARE(editor.toPlainText(), QStringLiteral("عدد = عدد.\nعدد."));
    QCOMPARE(panel.matchCount(), 0);

    editor.undo();
    QCOMPARE(editor.toPlainText(), QStringLiteral("عدد = قيمة.\nقيمة."));
}

void TestSearchPanel::expandsRegularExpressionCaptures()
{
    QalamEditor editor;
    editor.setPlainText(QStringLiteral("س1 س22"));

    QalamSearchPanel panel;
    panel.setEditor(&editor);
    panel.show();

    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    auto *replaceInput = panel.findChild<QLineEdit *>(QStringLiteral("replaceInput"));
    auto *regexCheck = panel.findChild<QCheckBox *>(QStringLiteral("searchRegex"));
    auto *replaceAllButton =
        panel.findChild<QPushButton *>(QStringLiteral("replaceAllButton"));
    QVERIFY(searchInput);
    QVERIFY(replaceInput);
    QVERIFY(regexCheck);
    QVERIFY(replaceAllButton);

    regexCheck->setChecked(true);
    searchInput->setText(QStringLiteral("س(\\d+)"));
    replaceInput->setText(QStringLiteral("قيمة_$1"));
    QCOMPARE(panel.matchCount(), 2);

    replaceAllButton->click();
    QCOMPARE(editor.toPlainText(), QStringLiteral("قيمة_1 قيمة_22"));
}

void TestSearchPanel::preservesDiagnosticDecorations()
{
    QalamEditor editor;
    editor.setPlainText(QStringLiteral("قيمة + قيمة"));
    editor.setDiagnostics({{
        QStringLiteral("fixture.baa"), 1, 1,
        QStringLiteral("warning"), QStringLiteral("تحذير")
    }});

    QalamSearchPanel panel;
    panel.setEditor(&editor);
    panel.show();
    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    QVERIFY(searchInput);
    searchInput->setText(QStringLiteral("قيمة"));

    QCOMPARE(editor.searchHighlightCount(), 2);
    bool diagnosticUnderlinePresent = false;
    for (const QTextEdit::ExtraSelection &selection : editor.extraSelections()) {
        if (selection.format.underlineStyle() == QTextCharFormat::SpellCheckUnderline) {
            diagnosticUnderlinePresent = true;
            break;
        }
    }
    QVERIFY(diagnosticUnderlinePresent);

    panel.clearHighlights();
    QCOMPARE(editor.searchHighlightCount(), 0);
    diagnosticUnderlinePresent = false;
    for (const QTextEdit::ExtraSelection &selection : editor.extraSelections()) {
        if (selection.format.underlineStyle() == QTextCharFormat::SpellCheckUnderline) {
            diagnosticUnderlinePresent = true;
            break;
        }
    }
    QVERIFY(diagnosticUnderlinePresent);
}

void TestSearchPanel::keepsClosedSearchHighlightsInactiveAcrossEditors()
{
    QalamEditor firstEditor;
    firstEditor.setPlainText(QStringLiteral("قيمة قيمة"));
    QalamEditor secondEditor;
    secondEditor.setPlainText(QStringLiteral("قيمة قيمة"));

    QalamSearchPanel panel;
    panel.setEditor(&firstEditor);
    panel.show();

    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    QVERIFY(searchInput);
    searchInput->setText(QStringLiteral("قيمة"));
    QCOMPARE(firstEditor.searchHighlightCount(), 2);

    panel.hide();
    QCOMPARE(firstEditor.searchHighlightCount(), 0);
    panel.setEditor(&secondEditor);
    QCOMPARE(secondEditor.searchHighlightCount(), 0);

    panel.show();
    QCOMPARE(secondEditor.searchHighlightCount(), 2);
}

void TestSearchPanel::rejectsInvalidAndZeroLengthPatternsSafely()
{
    QalamEditor editor;
    editor.setPlainText(QStringLiteral("قيمة"));

    QalamSearchPanel panel;
    panel.setEditor(&editor);
    panel.show();

    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    auto *regexCheck = panel.findChild<QCheckBox *>(QStringLiteral("searchRegex"));
    auto *countLabel = panel.findChild<QLabel *>(QStringLiteral("searchMatchCount"));
    QVERIFY(searchInput);
    QVERIFY(regexCheck);
    QVERIFY(countLabel);

    regexCheck->setChecked(true);
    searchInput->setText(QStringLiteral("["));
    QVERIFY(not panel.hasValidPattern());
    QCOMPARE(panel.matchCount(), 0);
    QCOMPARE(countLabel->text(), QStringLiteral("نمط غير صالح"));

    searchInput->setText(QStringLiteral("^|$"));
    QVERIFY(panel.hasValidPattern());
    QCOMPARE(panel.matchCount(), 0);
}

void TestSearchPanel::rendersCompactRtlLayout()
{
    QalamEditor editor;
    editor.setPlainText(QStringLiteral("دالة احسب(قيمة) { أرجع قيمة. }"));

    QalamSearchPanel panel;
    panel.setLayoutDirection(Qt::RightToLeft);
    panel.resize(920, 78);
    panel.setEditor(&editor);
    panel.show();

    auto *searchInput = panel.findChild<QLineEdit *>(QStringLiteral("searchInput"));
    auto *replaceInput = panel.findChild<QLineEdit *>(QStringLiteral("replaceInput"));
    auto *countLabel = panel.findChild<QLabel *>(QStringLiteral("searchMatchCount"));
    QVERIFY(searchInput);
    QVERIFY(replaceInput);
    QVERIFY(countLabel);

    searchInput->setText(QStringLiteral("قيمة"));
    replaceInput->setText(QStringLiteral("عدد"));
    QCoreApplication::processEvents();

    QCOMPARE(panel.height(), 78);
    QVERIFY(searchInput->width() >= 240);
    QVERIFY(replaceInput->width() >= 300);
    QVERIFY(countLabel->width() >= 74);
    QVERIFY(searchInput->geometry().intersected(countLabel->geometry()).isEmpty());

    const QString screenshotPath =
        qEnvironmentVariable("QALAM_SEARCH_PANEL_SCREENSHOT");
    if (not screenshotPath.isEmpty()) {
        QVERIFY(panel.grab().save(screenshotPath));
    }
}

QTEST_MAIN(TestSearchPanel)
#include "TestSearchPanel.moc"
