#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>

class LanguageDefinition {
public:
    QSet<QString> keywordSet{};
    QSet<QString> builtinSet{};
    QSet<QString> magicSet{};
    QSet<QString> preprocessorSet{};
    QRegularExpression numberPattern{};
    QRegularExpression hexPattern{};

    // Lists for iteration (autocomplete, etc.)
    QStringList keywordList{};
    QStringList builtinList{};
    QStringList preprocessorList{};

    LanguageDefinition();

    // Singleton accessor -- single source of truth for all language data
    static const LanguageDefinition& instance();

private:
    // Try to load definitions from a JSON resource file.
    // Returns true on success, false if the file is missing or malformed.
    bool loadFromJson(const QString &resourcePath);

    // Populate sets from the already-filled lists.
    void buildSets();

    // Hardcoded fallback if JSON is unavailable.
    void loadDefaults();
};
