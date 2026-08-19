#include "QalamSyntaxHighlighter.h"

#include <QRegularExpression>

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
    if (type == QStringLiteral("function")) return TokenType::Function;
    if (type == QStringLiteral("variable") or
        type == QStringLiteral("parameter") or
        type == QStringLiteral("property") or
        type == QStringLiteral("enumMember"))
        return TokenType::Identifier;
    return TokenType::None;
}
}

// ==================== Syntax Highlighter ====================

QalamSyntaxHighlighter::QalamSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    lexer = std::make_unique<QalamLexer>();
}

void QalamSyntaxHighlighter::setTheme(const std::shared_ptr<SyntaxTheme>& theme) {
    currentThemeFormats.clear();
    if (theme) {
        theme->apply(currentThemeFormats);
    }
    rehighlight();
}

void QalamSyntaxHighlighter::setSemanticTokens(
    const QVector<BaaSemanticToken> &tokens)
{
    semanticTokensByLine.clear();
    for (const BaaSemanticToken &token : tokens) {
        if (token.isValid())
            semanticTokensByLine[token.line].push_back(token);
    }
    rehighlight();
}

void QalamSyntaxHighlighter::clearSemanticTokens()
{
    if (semanticTokensByLine.isEmpty()) return;
    semanticTokensByLine.clear();
    rehighlight();
}

void QalamSyntaxHighlighter::setLanguageMode(QalamLanguageMode mode)
{
    if (m_languageMode == mode) return;
    m_languageMode = mode;
    semanticTokensByLine.clear();
    rehighlight();
}

void QalamSyntaxHighlighter::highlightBlock(const QString& text)
{
    if (m_languageMode == QalamLanguageMode::Nazm) {
        highlightNazmBlock(text);
        return;
    }
    if (m_languageMode == QalamLanguageMode::PlainText) {
        setCurrentBlockState(StateMasks::Normal);
        return;
    }
    highlightBaaBlock(text);
}

void QalamSyntaxHighlighter::highlightBaaBlock(const QString &text)
{
    int startState = previousBlockState();
    if (startState == -1) startState = StateMasks::Normal;

    QVector<QalamToken> tokens = lexer->tokenize(text, startState);

    for (const QalamToken& token : tokens) {
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

void QalamSyntaxHighlighter::highlightNazmBlock(const QString &text)
{
    setCurrentBlockState(StateMasks::Normal);
    auto applyMatches = [this, &text](const QRegularExpression &pattern,
                                     TokenType type,
                                     int capture = 0) {
        const auto format = currentThemeFormats.constFind(type);
        if (format == currentThemeFormats.cend()) return;
        QRegularExpressionMatchIterator matches = pattern.globalMatch(text);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            if (match.capturedStart(capture) >= 0) {
                setFormat(match.capturedStart(capture),
                          match.capturedLength(capture),
                          format.value());
            }
        }
    };

    static const QRegularExpression directive(
        QStringLiteral(R"((?:^|\s)(\.[\p{L}\p{N}_]+))"));
    static const QRegularExpression number(
        QStringLiteral(R"((?<![\p{L}_])[+-]?(?:٠[xX][٠-٩A-Fa-f]+|٠[bB][٠-١]+|0[xX][0-9A-Fa-f]+|0[bB][01]+|[٠-٩]+|[0-9]+))"));
    static const QRegularExpression registerName(
        QStringLiteral(R"(((?:سجل|مؤشر|فهرس)_[\p{L}\p{N}_]+))"));
    static const QRegularExpression label(
        QStringLiteral(R"(^\s*([\p{L}_][\p{L}\p{N}_]*)(?=\s*:))"));
    static const QRegularExpression instruction(
        QStringLiteral(R"(^\s*([\p{L}_][\p{L}\p{N}_]*)(?=\s|$))"));
    static const QRegularExpression stringLiteral(
        QStringLiteral(R"("(?:\\.|[^"\\])*")"));
    static const QRegularExpression operators(QStringLiteral(R"([\[\]+\-,:،])"));

    applyMatches(directive, TokenType::Preprocessor, 1);
    applyMatches(number, TokenType::Number);
    applyMatches(registerName, TokenType::Identifier, 1);
    applyMatches(label, TokenType::Function, 1);
    if (not text.trimmed().startsWith(QLatin1Char('.'))) {
        applyMatches(instruction, TokenType::Keyword, 1);
    }
    applyMatches(operators, TokenType::Operator);
    applyMatches(stringLiteral, TokenType::String);

    bool inString = false;
    bool escaped = false;
    int commentStart = -1;
    for (int index = 0; index < text.size(); ++index) {
        const QChar character = text.at(index);
        if (escaped) {
            escaped = false;
            continue;
        }
        if (inString and character == QLatin1Char('\\')) {
            escaped = true;
            continue;
        }
        if (character == QLatin1Char('"')) {
            inString = not inString;
            continue;
        }
        if (not inString and character == QLatin1Char(';')) {
            commentStart = index;
            break;
        }
    }
    const auto commentFormat = currentThemeFormats.constFind(TokenType::Comment);
    if (commentStart >= 0 and commentFormat != currentThemeFormats.cend()) {
        setFormat(commentStart, text.size() - commentStart, commentFormat.value());
    }
}
