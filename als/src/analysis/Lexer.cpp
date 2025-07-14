/**
 * @file Lexer.cpp
 * @brief Implementation of Alif language lexer
 */

#include "Lexer.h"
#include <algorithm>
#include <cctype>
#include <codecvt>
#include <locale>

namespace als {
namespace analysis {

// Static keyword definitions (ported from AlifLexer.cpp)
const std::unordered_set<std::string> Lexer::keywords_ = {
    "ك", "و", "في", "او", "أو", "من", "مع", "صح", "هل",
    "اذا", "إذا", "ليس", "مرر", "عدم", "ولد", "صنف", "خطا", "خطأ", "عام",
    "احذف", "دالة", "لاجل", "لأجل", "والا", "وإلا", "توقف", "نطاق", "ارجع",
    "اواذا", "أوإذا", "بينما", "انتظر", "استمر", "مزامنة", "استورد",
    "حاول", "خلل", "نهاية"
};

const std::unordered_set<std::string> Lexer::keywords1_ = {
    "اطبع", "ادخل", "مدى"
};

const std::unordered_set<std::string> Lexer::keywords2_ = {
    "_تهيئة_", "هذا", "اصل"
};

Lexer::Lexer(const std::string& source)
    : source_(source), pos_(0), line_(1), column_(1), 
      quoteCount_(0), isFString_(false) {
}

Lexer::~Lexer() = default;

std::vector<Token> Lexer::tokenize() {
    tokens_.clear();
    errors_.clear();
    pos_ = 0;
    line_ = 1;
    column_ = 1;
    quoteCount_ = 0;
    isFString_ = false;
    
    while (!isAtEnd()) {
        Token token = nextToken();
        if (token.type != TokenType::Invalid) {
            tokens_.push_back(token);
        }
        
        if (token.type == TokenType::EndOfFile) {
            break;
        }
    }
    
    // Add EOF token if not already added
    if (tokens_.empty() || tokens_.back().type != TokenType::EndOfFile) {
        Position eofPos = getPosition();
        tokens_.push_back(createToken(TokenType::EndOfFile, eofPos, eofPos));
    }
    
    return tokens_;
}

Token Lexer::nextToken() {
    if (isAtEnd()) {
        Position pos = getPosition();
        return createToken(TokenType::EndOfFile, pos, pos);
    }
    
    // Skip whitespace but track position
    skipWhitespace();
    
    if (isAtEnd()) {
        Position pos = getPosition();
        return createToken(TokenType::EndOfFile, pos, pos);
    }
    
    Position startPos = getPosition();
    char32_t ch = getCurrentChar();
    
    // Numbers
    if (isDigit(ch)) {
        return tokenizeNumber();
    }
    
    // Identifiers and keywords
    if (isIdentifierStart(ch)) {
        return tokenizeIdentifier();
    }
    
    // Strings
    if (ch == '"' || ch == '\'') {
        return tokenizeString(static_cast<char>(ch));
    }
    
    // Comments
    if (ch == '#') {
        return tokenizeComment();
    }
    
    // Operators
    if (isOperatorChar(ch)) {
        return tokenizeOperator();
    }
    
    // Punctuation
    if (isPunctuationChar(ch)) {
        return tokenizePunctuation();
    }
    
    // Unknown character - create error and skip
    std::string errorMsg = "Unexpected character: ";
    errorMsg += utf32ToString(ch);
    addError(errorMsg);
    advance();
    
    return createToken(TokenType::Invalid, startPos, getPosition());
}

bool Lexer::hasMoreTokens() const {
    return !isAtEnd();
}

void Lexer::reset(const std::string& source) {
    source_ = source;
    pos_ = 0;
    line_ = 1;
    column_ = 1;
    quoteCount_ = 0;
    isFString_ = false;
    tokens_.clear();
    errors_.clear();
}

const std::vector<LexerError>& Lexer::getErrors() const {
    return errors_;
}

Position Lexer::getCurrentPosition() const {
    return getPosition();
}

bool Lexer::isKeyword(const std::string& word) {
    return keywords_.find(word) != keywords_.end();
}

bool Lexer::isKeyword1(const std::string& word) {
    return keywords1_.find(word) != keywords1_.end();
}

bool Lexer::isKeyword2(const std::string& word) {
    return keywords2_.find(word) != keywords2_.end();
}

// Private implementation methods

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char32_t ch = getCurrentChar();
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            advance();
        } else {
            break;
        }
    }
}

Token Lexer::tokenizeNumber() {
    Position startPos = getPosition();
    std::string text;
    
    // Read digits and optional decimal point
    while (!isAtEnd()) {
        char32_t ch = getCurrentChar();
        if (isDigit(ch) || ch == '.') {
            text += utf32ToString(ch);
            advance();
        } else {
            break;
        }
    }
    
    Position endPos = getPosition();
    return createToken(TokenType::Number, startPos, endPos, text);
}

Token Lexer::tokenizeIdentifier() {
    Position startPos = getPosition();
    std::string text;
    
    // Read identifier characters
    while (!isAtEnd()) {
        char32_t ch = getCurrentChar();
        if (isIdentifierContinue(ch)) {
            text += utf32ToString(ch);
            advance();
        } else {
            break;
        }
    }
    
    Position endPos = getPosition();
    TokenType type = classifyIdentifier(text);
    return createToken(type, startPos, endPos, text);
}

Token Lexer::tokenizeString(char quote) {
    Position startPos = getPosition();
    std::string text;
    text += quote;
    advance(); // Skip opening quote
    
    // Check for f-string (Arabic prefix 'م')
    bool isFStringToken = false;
    if (startPos.offset > 0 && pos_ > 1) {
        // Check previous character for Arabic 'م'
        size_t prevPos = startPos.offset - 1;
        if (prevPos < source_.length()) {
            // Simple check for Arabic 'م' (U+0645)
            // This is a simplified check - full UTF-8 handling would be more complex
            if (static_cast<unsigned char>(source_[prevPos]) == 0xD9 && prevPos + 1 < source_.length() &&
                static_cast<unsigned char>(source_[prevPos + 1]) == 0x85) {
                isFStringToken = true;
                isFString_ = true;
                quoteCount_++;
            }
        }
    }
    
    // Read string content
    while (!isAtEnd()) {
        char32_t ch = getCurrentChar();
        
        if (ch == '\\') {
            // Handle escape sequences
            text += utf32ToString(ch);
            advance();
            if (!isAtEnd()) {
                text += utf32ToString(getCurrentChar());
                advance();
            }
        } else if (ch == quote) {
            // Found closing quote
            text += utf32ToString(ch);
            advance();
            if (isFStringToken) {
                quoteCount_--;
                if (quoteCount_ <= 0) {
                    isFString_ = false;
                }
            }
            break;
        } else if (isFStringToken && ch == '{') {
            // F-string expression start - this would need more complex handling
            text += utf32ToString(ch);
            advance();
        } else {
            text += utf32ToString(ch);
            advance();
        }
    }
    
    Position endPos = getPosition();
    return createToken(TokenType::String, startPos, endPos, text);
}

Token Lexer::tokenizeComment() {
    Position startPos = getPosition();
    std::string text;
    
    // Read until end of line
    while (!isAtEnd()) {
        char32_t ch = getCurrentChar();
        if (ch == '\n') {
            break;
        }
        text += utf32ToString(ch);
        advance();
    }
    
    Position endPos = getPosition();
    return createToken(TokenType::Comment, startPos, endPos, text);
}

Token Lexer::tokenizeOperator() {
    Position startPos = getPosition();
    std::string text;
    char32_t ch = getCurrentChar();
    
    text += utf32ToString(ch);
    advance();
    
    // Check for two-character operators
    if (!isAtEnd()) {
        char32_t nextCh = getCurrentChar();
        if ((ch == '=' && nextCh == '=') ||
            (ch == '!' && nextCh == '=') ||
            (ch == '<' && nextCh == '=') ||
            (ch == '>' && nextCh == '=')) {
            text += utf32ToString(nextCh);
            advance();
        }
    }
    
    Position endPos = getPosition();
    return createToken(TokenType::Operator, startPos, endPos, text);
}

Token Lexer::tokenizePunctuation() {
    Position startPos = getPosition();
    char32_t ch = getCurrentChar();
    std::string text = utf32ToString(ch);
    advance();
    
    Position endPos = getPosition();
    return createToken(TokenType::Punctuation, startPos, endPos, text);
}

// Helper method implementations

char32_t Lexer::getCurrentChar() const {
    if (isAtEnd()) {
        return 0;
    }
    return readUtf8Char();
}

char32_t Lexer::peekChar(size_t offset) const {
    size_t savedPos = pos_;
    size_t savedLine = line_;
    size_t savedColumn = column_;

    // Temporarily advance by offset
    for (size_t i = 0; i < offset && pos_ < source_.length(); ++i) {
        const_cast<Lexer*>(this)->advance();
    }

    char32_t result = isAtEnd() ? 0 : readUtf8Char();

    // Restore position
    const_cast<Lexer*>(this)->pos_ = savedPos;
    const_cast<Lexer*>(this)->line_ = savedLine;
    const_cast<Lexer*>(this)->column_ = savedColumn;

    return result;
}

void Lexer::advance() {
    if (isAtEnd()) {
        return;
    }

    char32_t ch = readUtf8Char();
    size_t charLen = utf8CharLength(source_[pos_]);
    pos_ += charLen;
    updatePosition(ch);
}

void Lexer::advanceBy(size_t count) {
    for (size_t i = 0; i < count && !isAtEnd(); ++i) {
        advance();
    }
}

bool Lexer::isAtEnd() const {
    return pos_ >= source_.length();
}

Position Lexer::getPosition() const {
    return Position(line_, column_, pos_);
}

void Lexer::updatePosition(char32_t ch) {
    if (ch == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
}

void Lexer::addError(const std::string& message) {
    addError(message, getPosition());
}

void Lexer::addError(const std::string& message, const Position& pos) {
    errors_.emplace_back(message, pos);
}

Token Lexer::createToken(TokenType type, const Position& start, const Position& end) {
    std::string text = source_.substr(start.offset, end.offset - start.offset);
    return Token(type, text, Range(start, end));
}

Token Lexer::createToken(TokenType type, const Position& start, const Position& end, const std::string& text) {
    return Token(type, text, Range(start, end));
}

bool Lexer::isDigit(char32_t ch) const {
    return ch >= '0' && ch <= '9';
}

bool Lexer::isLetter(char32_t ch) const {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || isArabicLetter(ch);
}

bool Lexer::isOperatorChar(char32_t ch) const {
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '\\' ||
           ch == '=' || ch == '<' || ch == '>' || ch == '!' || ch == '&' ||
           ch == '|' || ch == '%' || ch == '^' || ch == '~';
}

bool Lexer::isPunctuationChar(char32_t ch) const {
    return ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}' ||
           ch == ',' || ch == ';' || ch == ':' || ch == '.';
}

char32_t Lexer::readUtf8Char() const {
    if (pos_ >= source_.length()) {
        return 0;
    }

    unsigned char first = static_cast<unsigned char>(source_[pos_]);

    // ASCII character
    if (first < 0x80) {
        return static_cast<char32_t>(first);
    }

    // Multi-byte UTF-8 character (simplified handling)
    // This is a basic implementation - full UTF-8 support would be more complex
    if ((first & 0xE0) == 0xC0) {
        // 2-byte character
        if (pos_ + 1 < source_.length()) {
            unsigned char second = static_cast<unsigned char>(source_[pos_ + 1]);
            return ((first & 0x1F) << 6) | (second & 0x3F);
        }
    } else if ((first & 0xF0) == 0xE0) {
        // 3-byte character
        if (pos_ + 2 < source_.length()) {
            unsigned char second = static_cast<unsigned char>(source_[pos_ + 1]);
            unsigned char third = static_cast<unsigned char>(source_[pos_ + 2]);
            return ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
        }
    }

    return static_cast<char32_t>(first);
}

size_t Lexer::utf8CharLength(char ch) const {
    unsigned char uch = static_cast<unsigned char>(ch);
    if (uch < 0x80) return 1;
    if ((uch & 0xE0) == 0xC0) return 2;
    if ((uch & 0xF0) == 0xE0) return 3;
    if ((uch & 0xF8) == 0xF0) return 4;
    return 1; // Invalid UTF-8, treat as single byte
}

std::string Lexer::utf32ToString(char32_t ch) const {
    if (ch < 0x80) {
        return std::string(1, static_cast<char>(ch));
    }

    // Basic UTF-8 encoding (simplified)
    std::string result;
    if (ch < 0x800) {
        result += static_cast<char>(0xC0 | (ch >> 6));
        result += static_cast<char>(0x80 | (ch & 0x3F));
    } else if (ch < 0x10000) {
        result += static_cast<char>(0xE0 | (ch >> 12));
        result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (ch & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | (ch >> 18));
        result += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (ch & 0x3F));
    }

    return result;
}

TokenType Lexer::classifyIdentifier(const std::string& text) const {
    if (isKeyword(text)) {
        return TokenType::Keyword;
    }
    if (isKeyword1(text)) {
        return TokenType::Keyword1;
    }
    if (isKeyword2(text)) {
        return TokenType::Keyword2;
    }
    return TokenType::Identifier;
}

} // namespace analysis
} // namespace als
