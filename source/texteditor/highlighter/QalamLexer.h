#pragma once

#include "QalamSyntaxDefinition.h"

#include <QString>
#include <QSet>
#include <QTextCharFormat>
#include <memory>

#include "QalamToken.h"

// ==================== State Machine Interfaces ====================

class LexerState {
public:
    virtual ~LexerState() = default;
    virtual QalamToken readToken(QStringView text, int& pos, const LanguageDefinition& langDef) = 0;
    virtual std::unique_ptr<LexerState> nextState() const = 0;
    // Added clone for robust state copying (prototype pattern)
    virtual std::unique_ptr<LexerState> clone() const = 0;
    virtual int getStateId() const { return StateMasks::Normal; }
};

// 1. Normal Code State
class NormalState : public LexerState {
public:
    QalamToken readToken(QStringView text, int& pos, const LanguageDefinition& langDef) override;
    std::unique_ptr<LexerState> nextState() const override;
    std::unique_ptr<LexerState> clone() const override;

    mutable std::unique_ptr<LexerState> pendingState;
    int getStateId() const override { return StateMasks::Normal; }
};

// 2. String State (Single or Double)
class StringState : public LexerState {
    QString delimiter;
    int delimId;
    mutable bool m_terminated{false};
public:
    StringState(const QString& delim, int id);
    QalamToken readToken(QStringView text, int& pos, const LanguageDefinition& langDef) override;
    std::unique_ptr<LexerState> nextState() const override;
    std::unique_ptr<LexerState> clone() const override;
    int getStateId() const override { return StateMasks::String | delimId; }
};

// ==================== Lexer ====================

class QalamLexer {
public:
    QalamLexer();
    QVector<QalamToken> tokenize(QStringView text, int initialState);
    int getFinalState() const { return finalState; }

private:
    const LanguageDefinition& langDef;
    int finalState{};
};
