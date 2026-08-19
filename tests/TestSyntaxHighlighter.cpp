#include "QalamSyntaxHighlighter.h"

#include <QTest>
#include <QTextBlock>
#include <QTextDocument>

#include <memory>

class TestSyntaxHighlighter : public QObject
{
    Q_OBJECT

private slots:
    void overlaysCompilerTokensAndRestoresLocalHighlighting();
    void highlightsArabicNazmSource();
};

namespace {
QColor foregroundAt(const QTextDocument &document, int position)
{
    const QTextBlock block = document.firstBlock();
    if (not block.isValid() or not block.layout()) return {};
    for (const QTextLayout::FormatRange &range :
         block.layout()->formats()) {
        if (position >= range.start and
            position < range.start + range.length)
            return range.format.foreground().color();
    }
    return {};
}
}

void TestSyntaxHighlighter::highlightsArabicNazmSource()
{
    QTextDocument document;
    QalamSyntaxHighlighter highlighter(&document);
    highlighter.setTheme(std::make_shared<VSCodeDarkTheme>());
    highlighter.setLanguageMode(QalamLanguageMode::Nazm);
    const QString source = QStringLiteral(
        ".نص\nالرئيسية:\n    انقل سجل_المركم، ٤٢ ; تعليق\n");
    document.setPlainText(source);
    highlighter.rehighlight();

    const QTextBlock directive = document.findBlockByNumber(0);
    const QTextBlock label = document.findBlockByNumber(1);
    const QTextBlock instruction = document.findBlockByNumber(2);
    QVERIFY(directive.isValid());
    QVERIFY(label.isValid());
    QVERIFY(instruction.isValid());

    auto colorInBlock = [](const QTextBlock &block, int position) {
        if (not block.layout()) return QColor();
        for (const QTextLayout::FormatRange &range : block.layout()->formats()) {
            if (position >= range.start and position < range.start + range.length)
                return range.format.foreground().color();
        }
        return QColor();
    };
    QCOMPARE(colorInBlock(directive, 0), QColor(197, 134, 192));
    QCOMPARE(colorInBlock(label, 0), QColor(220, 220, 170));
    QCOMPARE(colorInBlock(instruction, instruction.text().indexOf(QStringLiteral("انقل"))),
             QColor(197, 134, 192));
    QCOMPARE(colorInBlock(instruction, instruction.text().indexOf(QStringLiteral("سجل_المركم"))),
             QColor(156, 220, 254));
    QCOMPARE(colorInBlock(instruction, instruction.text().indexOf(QStringLiteral("٤٢"))),
             QColor(181, 206, 168));
    QCOMPARE(colorInBlock(instruction, instruction.text().indexOf(QLatin1Char(';'))),
             QColor(106, 153, 85));
}

void TestSyntaxHighlighter::overlaysCompilerTokensAndRestoresLocalHighlighting()
{
    QTextDocument document;
    QalamSyntaxHighlighter highlighter(&document);
    highlighter.setTheme(std::make_shared<VSCodeDarkTheme>());
    document.setPlainText(QStringLiteral("اسم ١"));
    highlighter.rehighlight();

    QCOMPARE(foregroundAt(document, 0), QColor(156, 220, 254));

    highlighter.setSemanticTokens({
        BaaSemanticToken{0, 0, 3, QStringLiteral("function"), 0},
        BaaSemanticToken{0, 4, 1, QStringLiteral("number"), 0}
    });
    QCOMPARE(foregroundAt(document, 0), QColor(220, 220, 170));
    QCOMPARE(foregroundAt(document, 4), QColor(181, 206, 168));

    highlighter.setSemanticTokens({
        BaaSemanticToken{0, 0, 3, QStringLiteral("property"), 0}
    });
    QCOMPARE(foregroundAt(document, 0), QColor(156, 220, 254));

    highlighter.clearSemanticTokens();
    QCOMPARE(foregroundAt(document, 0), QColor(156, 220, 254));
}

QTEST_MAIN(TestSyntaxHighlighter)
#include "TestSyntaxHighlighter.moc"
