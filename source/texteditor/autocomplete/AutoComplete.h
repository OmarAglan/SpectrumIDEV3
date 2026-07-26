#pragma once

#include <QString>

enum CompletionType {
    Keyword,
    Snippet,
    Function,
    Variable,
    Type,
    Value,
    Preprocessor
};

struct CompletionItem {
    QString label;
    QString completion;
    QString description;
    CompletionType type{CompletionType::Value};
    bool snippet{};
    int startLine{};
    int startCharacter{};
    int endLine{};
    int endCharacter{};
};

