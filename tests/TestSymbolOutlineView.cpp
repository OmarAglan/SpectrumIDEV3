#include "QalamSymbolOutlineView.h"

#include <QSignalSpy>
#include <QTest>
#include <QTreeWidget>

class TestSymbolOutlineView : public QObject
{
    Q_OBJECT

private slots:
    void rendersFiltersAndActivatesCompilerSymbols();
};

void TestSymbolOutlineView::rendersFiltersAndActivatesCompilerSymbols()
{
    BaaDocumentSymbol field;
    field.name = QStringLiteral("قيمة_عضو");
    field.detail = QStringLiteral("صحيح");
    field.kind = 8;
    field.line = 4;
    field.column = 9;

    BaaDocumentSymbol record;
    record.name = QStringLiteral("سجل");
    record.detail = QStringLiteral("هيكل");
    record.kind = 23;
    record.line = 3;
    record.column = 6;
    record.children.push_back(field);

    BaaDocumentSymbol function;
    function.name = QStringLiteral("الرئيسية");
    function.detail = QStringLiteral("← صحيح");
    function.kind = 12;
    function.line = 8;
    function.column = 6;

    QalamSymbolOutlineView view;
    view.resize(320, 400);
    view.setSymbols(
        QStringLiteral("C:/مشروع/رئيسي.baa"),
        {record, function});

    QTreeWidget *tree = view.treeWidget();
    QCOMPARE(tree->topLevelItemCount(), 2);
    QCOMPARE(tree->topLevelItem(0)->text(0),
             QStringLiteral("سجل"));
    QCOMPARE(tree->topLevelItem(0)->childCount(), 1);

    view.setFilterText(QStringLiteral("عضو"));
    QVERIFY(not tree->topLevelItem(0)->isHidden());
    QVERIFY(not tree->topLevelItem(0)->child(0)->isHidden());
    QVERIFY(tree->topLevelItem(1)->isHidden());

    QSignalSpy activated(
        &view, &QalamSymbolOutlineView::symbolActivated);
    QTreeWidgetItem *fieldItem =
        tree->topLevelItem(0)->child(0);
    QVERIFY(QMetaObject::invokeMethod(
        tree,
        "itemActivated",
        Qt::DirectConnection,
        Q_ARG(QTreeWidgetItem*, fieldItem),
        Q_ARG(int, 0)));
    QCOMPARE(activated.size(), 1);
    const QList<QVariant> arguments = activated.takeFirst();
    QCOMPARE(
        arguments.at(0).toString(),
        QStringLiteral("C:/مشروع/رئيسي.baa"));
    QCOMPARE(arguments.at(1).toInt(), 4);
    QCOMPARE(arguments.at(2).toInt(), 9);

    view.setFilterText(QStringLiteral("غير_موجود"));
    QVERIFY(tree->isHidden());
}

QTEST_MAIN(TestSymbolOutlineView)
#include "TestSymbolOutlineView.moc"
