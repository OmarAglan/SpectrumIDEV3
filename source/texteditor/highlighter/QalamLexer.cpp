#include "QalamLexer.h"



// ==================== Helpers ====================

static QPair<int, QString> checkStringStart(QStringView text, int pos) {
    if (pos >= text.length()) return {-1, ""};
    QChar ch = text[pos];
    if (ch == '"' || ch == '\'') {
        return {0, QString(ch)};
    }
    return {-1, ""};
}

static QRegularExpressionMatch matchAt(
    const QRegularExpression &pattern,
    QStringView text,
    int offset)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    return pattern.matchView(
        text,
        offset,
        QRegularExpression::NormalMatch,
        QRegularExpression::AnchorAtOffsetMatchOption);
#else
    return pattern.match(
        text,
        offset,
        QRegularExpression::NormalMatch,
        QRegularExpression::AnchorAtOffsetMatchOption);
#endif
}

// ==================== Normal State ====================

QalamToken NormalState::readToken(QStringView text, int& pos, const LanguageDefinition& langDef) {
    if (pos >= text.length()) return QalamToken(TokenType::None, pos, 0);

    QChar ch = text[pos];

    // 1. Whitespace
    if (ch.isSpace()) {
        int start = pos;
        while (pos < text.length() && text[pos].isSpace()) pos++;
        return QalamToken(TokenType::Whitespace, start, pos - start);
    }

    // 2. Comments (Baa uses // for single-line comments)
    if (pos + 1 < text.length() && ch == '/' && text[pos + 1] == '/') {
        int start = pos;
        pos = text.length();
        return QalamToken(TokenType::Comment, start, pos - start);
    }

    // 3. Preprocessor directives (start with #)
    if (ch == '#') {
        int start = pos;
        pos++; // Skip the #

        // Read the directive name (Arabic letters and underscores after #)
        while (pos < text.length() && (text[pos].isLetter() || text[pos] == '_')) {
            pos++;
        }

        QString directive = text.mid(start, pos - start).toString();

        if (langDef.preprocessorSet.contains(directive)) {
            return QalamToken(TokenType::Preprocessor, start, pos - start, directive);
        }

        return QalamToken(TokenType::Operator, start, pos - start, directive);
    }

    // 4. Strings
    auto strCheck = checkStringStart(text, pos);
    if (strCheck.first != -1) {
        int start = pos;
        QString quote = strCheck.second;
        
        pos += quote.length(); 

        int delimId = (quote == "\"") ? StateMasks::Double : StateMasks::Single;
        pendingState = std::make_unique<StringState>(quote, delimId);
        return QalamToken(TokenType::String, start, pos - start);
    }

    // 5. Identifiers & Keywords
    if (ch.isLetter() || ch == '_') {
        int start = pos;
        while (pos < text.length() && (text[pos].isLetterOrNumber() || text[pos] == '_')) pos++;
        QStringView wordView = text.mid(start, pos - start);
        QString word = wordView.toString();

        if (word == "صواب" || word == "خطأ") return QalamToken(TokenType::BooleanLiteral, start, pos - start, word);
        if (langDef.keywordSet.contains(word)) return QalamToken(TokenType::Keyword, start, pos - start, word);
        if (langDef.builtinSet.contains(word)) return QalamToken(TokenType::BuiltinFunc, start, pos - start, word);
        if (langDef.preprocessorSet.contains(word)) return QalamToken(TokenType::Preprocessor, start, pos - start, word);

        // Check for function pattern 'func('
        int next = pos;
        while(next < text.length() && text[next].isSpace()) next++;
        if (next < text.length() && text[next] == '(') {
            return QalamToken(TokenType::Function, start, pos - start, word);
        }

        return QalamToken(TokenType::Identifier, start, pos - start, word);
    }

    // 6. Numbers (Integers Only (§3.1))
    if (ch.isDigit() || ch == u'٠' || ch == u'١' || ch == u'٢' || ch == u'٣' || ch == u'٤' || ch == u'٥' || ch == u'٦' || ch == u'٧' || ch == u'٨' || ch == u'٩') {
        int start = pos;
        if (ch == '0' && pos + 1 < text.length() && text.mid(pos, 2).compare(u"0x", Qt::CaseInsensitive) == 0) {
            auto m = matchAt(langDef.hexPattern, text, start);
            if (m.hasMatch()) { pos += m.capturedLength(); return QalamToken(TokenType::Number, start, m.capturedLength()); }
        }
        auto m = matchAt(langDef.numberPattern, text, start);
        if (m.hasMatch()) { pos += m.capturedLength(); return QalamToken(TokenType::Number, start, m.capturedLength()); }
        pos++; return QalamToken(TokenType::Number, start, 1);
    }

    // 7. Separators
    if (ch == '.' || ch == u'؛') {
        pos++;
        return QalamToken(TokenType::Separator, pos - 1, 1, QString(ch));
    }

    // 8. Multi-character operators (look-ahead for second character)
    if (pos + 1 < text.length()) {
        QChar next = text[pos + 1];
        // Two-character operators: ==, !=, <=, >=, &&, ||, ++, --, +=, -=, *=, /=, %=, <<, >>
        if ((ch == '=' and next == '=') or
            (ch == '!' and next == '=') or
            (ch == '<' and next == '=') or
            (ch == '>' and next == '=') or
            (ch == '&' and next == '&') or
            (ch == '|' and next == '|') or
            (ch == '+' and next == '+') or
            (ch == '-' and next == '-') or
            (ch == '+' and next == '=') or
            (ch == '-' and next == '=') or
            (ch == '*' and next == '=') or
            (ch == '/' and next == '=') or
            (ch == '%' and next == '=') or
            (ch == '<' and next == '<') or
            (ch == '>' and next == '>')) {
            int start = pos;
            pos += 2;
            return QalamToken(TokenType::Operator, start, 2, text.mid(start, 2).toString());
        }
    }

    // 9. Single-character operators (fallback)
    pos++;
    return QalamToken(TokenType::Operator, pos - 1, 1, QString(ch));
}

std::unique_ptr<LexerState> NormalState::nextState() const {
    if (pendingState) return std::move(pendingState);
    return std::make_unique<NormalState>();
}

std::unique_ptr<LexerState> NormalState::clone() const {
    auto c = std::make_unique<NormalState>();
    if (pendingState) c->pendingState = pendingState->clone();
    return c;
}

// ==================== String States ====================

StringState::StringState(const QString& delim, int id) : delimiter(delim), delimId(id) {}

QalamToken StringState::readToken(QStringView text, int& pos, const LanguageDefinition&) {
    int start = pos;
    while (pos < text.length()) {
        if (text[pos] == '\\') { pos = qMin(pos + 2, static_cast<int>(text.length())); continue; }
        if (text.mid(pos).startsWith(delimiter)) {
            pos += delimiter.length();
            m_terminated = true;
            return QalamToken(TokenType::String, start, pos - start);
        }
        pos++;
    }
    m_terminated = false;
    return QalamToken(TokenType::String, start, pos - start);
}

std::unique_ptr<LexerState> StringState::nextState() const {
    if (m_terminated) {
        return std::make_unique<NormalState>();
    }
    return std::make_unique<StringState>(delimiter, delimId);
}

std::unique_ptr<LexerState> StringState::clone() const {
    return std::make_unique<StringState>(delimiter, delimId);
}

// ==================== Lexer ====================

QalamLexer::QalamLexer() : langDef(LanguageDefinition::instance()) { finalState = StateMasks::Normal; }

QVector<QalamToken> QalamLexer::tokenize(QStringView text, int initialState) {
    QVector<QalamToken> tokens;
    int pos = 0;

    std::unique_ptr<LexerState> currentState;

    int type = initialState & StateMasks::TypeMask;
    int dType = initialState & StateMasks::DelimMask;

    QString delim = "\"";
    if (dType == StateMasks::Single) delim = "'";

    if (type == StateMasks::String) {
        currentState = std::make_unique<StringState>(delim, dType);
    } else {
        currentState = std::make_unique<NormalState>();
    }

    while (pos < text.length()) {
        QalamToken token = currentState->readToken(text, pos, langDef);

        if (token.length > 0) tokens.append(token);
        else if (pos < text.length()) pos++;

        currentState = currentState->nextState();
    }

    finalState = currentState->getStateId();
    return tokens;
}
