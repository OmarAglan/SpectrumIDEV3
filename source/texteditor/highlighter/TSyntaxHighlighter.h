#pragma once

#include "TLexer.h"
#include "TSyntaxThemes.h"
#include "BaaSemanticToken.h"

#include <QSyntaxHighlighter>
#include <QVector>

class TSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit TSyntaxHighlighter(QTextDocument* parent = nullptr);

    // Switch theme
    void setTheme(const std::shared_ptr<SyntaxTheme>& theme);
    void setSemanticTokens(const QVector<BaaSemanticToken> &tokens);
    void clearSemanticTokens();

protected:
    void highlightBlock(const QString& text) override;

private:
    std::unique_ptr<TLexer> lexer{};
    QHash<TokenType, QTextCharFormat> currentThemeFormats{};
    QHash<int, QVector<BaaSemanticToken>> semanticTokensByLine{};
};
