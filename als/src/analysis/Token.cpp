/**
 * @file Token.cpp
 * @brief Implementation of token utility functions
 */

#include "Token.h"

namespace als {
namespace analysis {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Keyword:        return "Keyword";
        case TokenType::Keyword1:       return "Keyword1";
        case TokenType::Keyword2:       return "Keyword2";
        case TokenType::Identifier:     return "Identifier";
        case TokenType::Number:         return "Number";
        case TokenType::String:         return "String";
        case TokenType::Comment:        return "Comment";
        case TokenType::Whitespace:     return "Whitespace";
        case TokenType::Operator:       return "Operator";
        case TokenType::Punctuation:    return "Punctuation";
        case TokenType::EndOfFile:      return "EndOfFile";
        case TokenType::Invalid:        return "Invalid";
        case TokenType::FStringStart:   return "FStringStart";
        case TokenType::FStringMiddle:  return "FStringMiddle";
        case TokenType::FStringEnd:     return "FStringEnd";
        default:                        return "Unknown";
    }
}

bool isArabicLetter(char32_t ch) {
    // Arabic Unicode block ranges
    // Basic Arabic: U+0600-U+06FF
    // Arabic Supplement: U+0750-U+077F
    // Arabic Extended-A: U+08A0-U+08FF
    // Arabic Presentation Forms-A: U+FB50-U+FDFF
    // Arabic Presentation Forms-B: U+FE70-U+FEFF
    
    return (ch >= 0x0600 && ch <= 0x06FF) ||  // Basic Arabic
           (ch >= 0x0750 && ch <= 0x077F) ||  // Arabic Supplement
           (ch >= 0x08A0 && ch <= 0x08FF) ||  // Arabic Extended-A
           (ch >= 0xFB50 && ch <= 0xFDFF) ||  // Arabic Presentation Forms-A
           (ch >= 0xFE70 && ch <= 0xFEFF);    // Arabic Presentation Forms-B
}

bool isIdentifierStart(char32_t ch) {
    // ASCII letters
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
        return true;
    }
    
    // Underscore
    if (ch == '_') {
        return true;
    }
    
    // Arabic letters
    return isArabicLetter(ch);
}

bool isIdentifierContinue(char32_t ch) {
    // All identifier start characters
    if (isIdentifierStart(ch)) {
        return true;
    }
    
    // Digits
    if (ch >= '0' && ch <= '9') {
        return true;
    }
    
    // Arabic digits
    if (ch >= 0x0660 && ch <= 0x0669) {  // Arabic-Indic digits
        return true;
    }
    
    return false;
}

bool isArabicString(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    
    // Simple check - convert to UTF-32 and check each character
    // This is a simplified implementation
    for (size_t i = 0; i < str.length(); ) {
        unsigned char ch = static_cast<unsigned char>(str[i]);
        
        if (ch < 0x80) {
            // ASCII character
            if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')) {
                // Non-letter ASCII character found
                i++;
                continue;
            }
            return false; // ASCII letter found, not purely Arabic
        }
        
        // Multi-byte UTF-8 character
        char32_t codepoint = 0;
        size_t charLen = 1;
        
        if ((ch & 0xE0) == 0xC0) {
            // 2-byte character
            if (i + 1 < str.length()) {
                charLen = 2;
                codepoint = ((ch & 0x1F) << 6) | (static_cast<unsigned char>(str[i + 1]) & 0x3F);
            }
        } else if ((ch & 0xF0) == 0xE0) {
            // 3-byte character
            if (i + 2 < str.length()) {
                charLen = 3;
                codepoint = ((ch & 0x0F) << 12) | 
                           ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 6) |
                           (static_cast<unsigned char>(str[i + 2]) & 0x3F);
            }
        } else if ((ch & 0xF8) == 0xF0) {
            // 4-byte character
            if (i + 3 < str.length()) {
                charLen = 4;
                codepoint = ((ch & 0x07) << 18) |
                           ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 12) |
                           ((static_cast<unsigned char>(str[i + 2]) & 0x3F) << 6) |
                           (static_cast<unsigned char>(str[i + 3]) & 0x3F);
            }
        }
        
        if (codepoint != 0 && !isArabicLetter(codepoint)) {
            return false; // Non-Arabic character found
        }
        
        i += charLen;
    }
    
    return true; // All characters are Arabic
}

} // namespace analysis
} // namespace als
