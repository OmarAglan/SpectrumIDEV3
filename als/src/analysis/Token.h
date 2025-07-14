/**
 * @file Token.h
 * @brief Token definitions and structures for Alif language lexical analysis
 */

#pragma once

#include <string>
#include <vector>

namespace als {
namespace analysis {

/**
 * @brief Token types for Alif language
 * 
 * Based on the existing AlifLexer implementation with Arabic language support
 */
enum class TokenType {
    // Language constructs
    Keyword,        // Arabic keywords: "اذا", "دالة", "صنف", etc.
    Keyword1,       // Built-in functions: "اطبع", "ادخل", "مدى"
    Keyword2,       // Special identifiers: "_تهيئة_", "هذا", "اصل"
    Identifier,     // Variable/function names (Arabic and Latin)
    
    // Literals
    Number,         // Numeric literals (integers and floats)
    String,         // String literals with f-string support
    
    // Comments and whitespace
    Comment,        // Comments starting with #
    Whitespace,     // Spaces, tabs, newlines
    
    // Operators and punctuation
    Operator,       // Arithmetic, comparison, logical operators
    Punctuation,    // Brackets, parentheses, commas, etc.
    
    // Special tokens
    EndOfFile,      // End of input
    Invalid,        // Invalid/unrecognized token
    
    // F-string specific
    FStringStart,   // Start of f-string with 'م' prefix
    FStringMiddle,  // Middle part of f-string
    FStringEnd      // End of f-string
};

/**
 * @brief Position information for a token
 */
struct Position {
    size_t line;        // 1-based line number
    size_t column;      // 1-based column number
    size_t offset;      // 0-based byte offset in source
    
    Position(size_t l = 1, size_t c = 1, size_t o = 0) 
        : line(l), column(c), offset(o) {}
};

/**
 * @brief Range information for a token
 */
struct Range {
    Position start;
    Position end;
    
    Range() = default;
    Range(const Position& s, const Position& e) : start(s), end(e) {}
};

/**
 * @brief Token structure containing all token information
 */
struct Token {
    TokenType type;         // Token type
    std::string text;       // Original text content
    Range range;            // Position range in source
    
    // Additional metadata
    bool isKeyword() const {
        return type == TokenType::Keyword || 
               type == TokenType::Keyword1 || 
               type == TokenType::Keyword2;
    }
    
    bool isLiteral() const {
        return type == TokenType::Number || 
               type == TokenType::String;
    }
    
    bool isOperator() const {
        return type == TokenType::Operator;
    }
    
    bool isIdentifier() const {
        return type == TokenType::Identifier;
    }
    
    // Constructor
    Token(TokenType t = TokenType::Invalid, 
          const std::string& txt = "", 
          const Range& r = Range())
        : type(t), text(txt), range(r) {}
};

/**
 * @brief Lexer error information
 */
struct LexerError {
    std::string message;    // Error description
    Position position;      // Error location
    std::string context;    // Surrounding context
    
    LexerError(const std::string& msg, const Position& pos, const std::string& ctx = "")
        : message(msg), position(pos), context(ctx) {}
};

/**
 * @brief Token type to string conversion for debugging
 */
const char* tokenTypeToString(TokenType type);

/**
 * @brief Check if a character is a valid Arabic letter
 */
bool isArabicLetter(char32_t ch);

/**
 * @brief Check if a character is a valid identifier start character
 */
bool isIdentifierStart(char32_t ch);

/**
 * @brief Check if a character is a valid identifier continuation character
 */
bool isIdentifierContinue(char32_t ch);

/**
 * @brief Check if a string contains only Arabic characters
 */
bool isArabicString(const std::string& str);

} // namespace analysis
} // namespace als
