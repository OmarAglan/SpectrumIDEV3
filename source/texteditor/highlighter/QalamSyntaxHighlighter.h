#pragma once

#include "QalamLexer.h"
#include "QalamSyntaxThemes.h"
#include "BaaSemanticToken.h"

#include <QSyntaxHighlighter>
#include <QVector>

enum class QalamLanguageMode {
    Baa,
    Nazm,
    PlainText
};

class QalamSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit QalamSyntaxHighlighter(QTextDocument* parent = nullptr);

    // Switch theme
    void setTheme(const std::shared_ptr<SyntaxTheme>& theme);
    void setSemanticTokens(const QVector<BaaSemanticToken> &tokens);
    void clearSemanticTokens();
    void setLanguageMode(QalamLanguageMode mode);
    QalamLanguageMode languageMode() const { return m_languageMode; }

protected:
    void highlightBlock(const QString& text) override;

private:
    void highlightBaaBlock(const QString &text);
    void highlightNazmBlock(const QString &text);
    std::unique_ptr<QalamLexer> lexer{};
    QHash<TokenType, QTextCharFormat> currentThemeFormats{};
    QHash<int, QVector<BaaSemanticToken>> semanticTokensByLine{};
    QalamLanguageMode m_languageMode{QalamLanguageMode::Baa};
};
