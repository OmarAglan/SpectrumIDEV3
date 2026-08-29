#pragma once

#include <QString>

enum CompletionType {
    Keyword,
    Snippet,
    Function,
    Variable,
    Type,
    Value,
    Preprocessor,
    File,
    Folder
};

struct CompletionItem {
    QString label;
    QString completion;
    QString description;
    QString serverSortText;
    QString context;
    QString stableKey;
    CompletionType type{CompletionType::Value};
    bool snippet{};
    int startLine{};
    int startCharacter{};
    int endLine{};
    int endCharacter{};
};

