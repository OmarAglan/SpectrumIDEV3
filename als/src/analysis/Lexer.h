/**
 * @file Lexer.h
 * @brief Lexer interface for Alif language tokenization
 */

#pragma once

#include "Token.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>

namespace als {
namespace analysis {

/**
 * @brief Lexer for Alif programming language
 * 
 * This lexer tokenizes Alif source code, supporting:
 * - Arabic keywords and identifiers
 * - Unicode text processing
 * - F-string literals with Arabic prefix 'م'
 * - Error recovery and position tracking
 * - RTL text handling
 */
class Lexer {
public:
    /**
     * @brief Construct lexer with source code
     * @param source Source code to tokenize
     */
    explicit Lexer(const std::string& source);
    
    /**
     * @brief Destructor
     */
    ~Lexer();
    
    /**
     * @brief Tokenize the entire source code
     * @return Vector of tokens
     */
    std::vector<Token> tokenize();
    
    /**
     * @brief Get next token from current position
     * @return Next token
     */
    Token nextToken();
    
    /**
     * @brief Check if there are more tokens to process
     * @return True if more tokens available
     */
    bool hasMoreTokens() const;
    
    /**
     * @brief Reset lexer with new source code
     * @param source New source code
     */
    void reset(const std::string& source);
    
    /**
     * @brief Get all lexer errors
     * @return Vector of lexer errors
     */
    const std::vector<LexerError>& getErrors() const;
    
    /**
     * @brief Get current position in source
     * @return Current position
     */
    Position getCurrentPosition() const;
    
    /**
     * @brief Check if a word is an Alif keyword
     * @param word Word to check
     * @return True if word is a keyword
     */
    static bool isKeyword(const std::string& word);
    
    /**
     * @brief Check if a word is a built-in function (Keyword1)
     * @param word Word to check
     * @return True if word is a built-in function
     */
    static bool isKeyword1(const std::string& word);
    
    /**
     * @brief Check if a word is a special identifier (Keyword2)
     * @param word Word to check
     * @return True if word is a special identifier
     */
    static bool isKeyword2(const std::string& word);

private:
    // Source code and position tracking
    std::string source_;
    size_t pos_;
    size_t line_;
    size_t column_;
    
    // Token and error storage
    std::vector<Token> tokens_;
    std::vector<LexerError> errors_;
    
    // F-string state tracking
    int quoteCount_;
    bool isFString_;
    
    // Static keyword sets
    static const std::unordered_set<std::string> keywords_;
    static const std::unordered_set<std::string> keywords1_;
    static const std::unordered_set<std::string> keywords2_;
    
    // Core tokenization methods
    void skipWhitespace();
    Token tokenizeNumber();
    Token tokenizeIdentifier();
    Token tokenizeString(char quote);
    Token tokenizeComment();
    Token tokenizeOperator();
    Token tokenizePunctuation();
    
    // Helper methods
    char32_t getCurrentChar() const;
    char32_t peekChar(size_t offset = 1) const;
    void advance();
    void advanceBy(size_t count);
    bool isAtEnd() const;
    
    // Position tracking
    Position getPosition() const;
    void updatePosition(char32_t ch);
    
    // Error handling
    void addError(const std::string& message);
    void addError(const std::string& message, const Position& pos);
    
    // Token creation helpers
    Token createToken(TokenType type, const Position& start, const Position& end);
    Token createToken(TokenType type, const Position& start, const Position& end, const std::string& text);
    
    // Character classification
    bool isDigit(char32_t ch) const;
    bool isLetter(char32_t ch) const;
    bool isOperatorChar(char32_t ch) const;
    bool isPunctuationChar(char32_t ch) const;
    
    // UTF-8 handling
    char32_t readUtf8Char();
    size_t utf8CharLength(char ch) const;
    std::string utf32ToString(char32_t ch) const;
    
    // Keyword classification
    TokenType classifyIdentifier(const std::string& text) const;
};

} // namespace analysis
} // namespace als
