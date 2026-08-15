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
