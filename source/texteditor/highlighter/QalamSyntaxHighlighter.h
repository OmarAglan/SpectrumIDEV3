#pragma once

#include "QalamLexer.h"
#include "QalamSyntaxThemes.h"
#include "BaaSemanticToken.h"

#include <QSyntaxHighlighter>
#include <QVector>

class QalamSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit QalamSyntaxHighlighter(QTextDocument* parent = nullptr);

    // Switch theme
    void setTheme(const std::shared_ptr<SyntaxTheme>& theme);
    void setSemanticTokens(const QVector<BaaSemanticToken> &tokens);
    void clearSemanticTokens();

protected:
    void highlightBlock(const QString& text) override;

private:
    std::unique_ptr<QalamLexer> lexer{};
    QHash<TokenType, QTextCharFormat> currentThemeFormats{};
    QHash<int, QVector<BaaSemanticToken>> semanticTokensByLine{};
};
