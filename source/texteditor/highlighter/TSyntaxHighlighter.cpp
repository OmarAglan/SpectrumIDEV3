#include "TSyntaxHighlighter.h"

namespace {
TokenType formatTypeForSemanticToken(const QString &type)
{
    if (type == QStringLiteral("type") or
        type == QStringLiteral("keyword") or
        type == QStringLiteral("modifier"))
        return TokenType::Keyword;
    if (type == QStringLiteral("macro")) return TokenType::Preprocessor;
    if (type == QStringLiteral("comment")) return TokenType::Comment;
    if (type == QStringLiteral("string")) return TokenType::String;
    if (type == QStringLiteral("number")) return TokenType::Number;
    if (type == QStringLiteral("operator")) return TokenType::Operator;
    return TokenType::None;
}
}

// ==================== Syntax Highlighter ====================

TSyntaxHighlighter::TSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    lexer = std::make_unique<TLexer>();
}

void TSyntaxHighlighter::setTheme(const std::shared_ptr<SyntaxTheme>& theme) {
    currentThemeFormats.clear();
    if (theme) {
        theme->apply(currentThemeFormats);
    }
    rehighlight();
}

void TSyntaxHighlighter::setSemanticTokens(
    const QVector<BaaSemanticToken> &tokens)
{
    semanticTokensByLine.clear();
    for (const BaaSemanticToken &token : tokens) {
        if (token.isValid())
            semanticTokensByLine[token.line].push_back(token);
    }
    rehighlight();
}

void TSyntaxHighlighter::clearSemanticTokens()
{
    if (semanticTokensByLine.isEmpty()) return;
    semanticTokensByLine.clear();
    rehighlight();
}

void TSyntaxHighlighter::highlightBlock(const QString& text) {
    int startState = previousBlockState();
    if (startState == -1) startState = StateMasks::Normal;

    QVector<TToken> tokens = lexer->tokenize(text, startState);

    for (const TToken& token : tokens) {
        auto it = currentThemeFormats.find(token.type);
        if (it != currentThemeFormats.end()) {
            setFormat(token.start, token.length, *it);
        }
    }

    const auto semanticTokens =
        semanticTokensByLine.constFind(currentBlock().blockNumber());
    if (semanticTokens != semanticTokensByLine.cend()) {
        for (const BaaSemanticToken &token : semanticTokens.value()) {
            const TokenType formatType =
                formatTypeForSemanticToken(token.type);
            const auto format = currentThemeFormats.constFind(formatType);
            const int start = qBound(0, token.character, text.size());
            const int length =
                qBound(0, token.length, text.size() - start);
            if (format != currentThemeFormats.cend() and length > 0)
                setFormat(start, length, format.value());
        }
    }

    setCurrentBlockState(lexer->getFinalState());
}
