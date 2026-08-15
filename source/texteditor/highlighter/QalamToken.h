#pragma once

#include <QString>

// ==================== Constants & Enums ====================

namespace StateMasks {
const int Normal = 0x0;
const int String = 0x1;
const int TypeMask = 0xFF;
const int DelimMask = 0xFF00;

const int Single = 0x100;      // '
const int Double = 0x200;      // "
}

enum StateType {
    Normal = 0,
    String = 1
};

enum class TokenType {
    None = 0,
    Keyword,
    BuiltinFunc,
    Number,
    String,
    Comment,
    Function,
    Operator,
    Identifier,
    Whitespace,
    Error,
    Preprocessor,
    Separator,
    BooleanLiteral
};

struct QalamToken {
    TokenType type;
    int start;
    int length;
    QString value;

    QalamToken(TokenType t = TokenType::None, int s = 0, int l = 0, const QString& v = "")
        : type(t), start(s), length(l), value(v) {}
};
