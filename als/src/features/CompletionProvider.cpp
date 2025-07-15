/**
 * @file CompletionProvider.cpp
 * @brief Implementation of LSP completion provider for Alif language
 */

#include "CompletionProvider.h"
#include <algorithm>
#include <sstream>
#include <set>

namespace als {
namespace features {

CompletionProvider::CompletionProvider() : lexer_("") {
}

CompletionProvider::~CompletionProvider() = default;

std::vector<CompletionItem> CompletionProvider::provideCompletions(const LegacyCompletionContext& context) {
    std::vector<CompletionItem> completions;
    
    // Don't provide completions if we're in a comment or string
    if (!context.tokens.empty()) {
        // Find token at current position
        for (const auto& token : context.tokens) {
            if (token.range.start.line <= context.line && 
                token.range.end.line >= context.line &&
                token.range.start.column <= context.character &&
                token.range.end.column >= context.character) {
                
                if (token.type == analysis::TokenType::Comment ||
                    token.type == analysis::TokenType::String) {
                    return completions; // No completions in comments/strings
                }
                break;
            }
        }
    }
    
    // Provide different types of completions
    auto keywordCompletions = provideKeywordCompletions(context);
    auto builtinCompletions = provideBuiltinCompletions(context);
    auto identifierCompletions = provideIdentifierCompletions(context);
    
    // Combine all completions
    completions.insert(completions.end(), keywordCompletions.begin(), keywordCompletions.end());
    completions.insert(completions.end(), builtinCompletions.begin(), builtinCompletions.end());
    completions.insert(completions.end(), identifierCompletions.begin(), identifierCompletions.end());
    
    // Filter by current word prefix
    if (!context.currentWord.empty()) {
        completions = filterCompletions(completions, context.currentWord);
    }
    
    return completions;
}

nlohmann::json CompletionProvider::toJson(const std::vector<CompletionItem>& items) {
    nlohmann::json result = nlohmann::json::object();
    result["isIncomplete"] = false;
    result["items"] = nlohmann::json::array();
    
    for (const auto& item : items) {
        result["items"].push_back(completionItemToJson(item));
    }
    
    return result;
}

LegacyCompletionContext CompletionProvider::createContext(const std::string& documentUri,
                                                         const std::string& documentContent,
                                                         size_t line, size_t character) {
    LegacyCompletionContext context;
    context.documentUri = documentUri;
    context.documentContent = documentContent;
    context.line = line;
    context.character = character;
    
    // Tokenize document
    lexer_.reset(documentContent);
    context.tokens = lexer_.tokenize();
    
    // Get current word being typed
    context.currentWord = getCurrentWord(documentContent, line, character);
    
    // Get previous token for context
    context.previousToken = getPreviousToken(context.tokens, line, character);
    
    return context;
}

CompletionContext CompletionProvider::createArabicContext(const std::string& documentUri,
                                                         const std::string& documentContent,
                                                         size_t line, size_t character) {
    CompletionContext context;
    context.cursorLine = static_cast<int>(line);
    context.cursorColumn = static_cast<int>(character);
    context.currentWord = getCurrentWord(documentContent, line, character);

    // TODO: Add more sophisticated context analysis
    // For now, set basic context
    context.type = CompletionContext::Type::Global;

    // TODO: Analyze document content to populate:
    // - availableVariables
    // - availableFunctions
    // - availableClasses
    // - currentScope

    return context;
}

std::vector<CompletionItem> CompletionProvider::provideKeywordCompletions(const LegacyCompletionContext& context) {
    (void)context; // Mark as intentionally unused for now
    return getKeywordCompletions();
}

std::vector<CompletionItem> CompletionProvider::provideBuiltinCompletions(const LegacyCompletionContext& context) {
    (void)context; // Mark as intentionally unused for now
    return getBuiltinCompletions();
}

std::vector<CompletionItem> CompletionProvider::provideIdentifierCompletions(const LegacyCompletionContext& context) {
    std::vector<CompletionItem> completions;
    
    // Extract identifiers from tokens
    std::set<std::string> identifiers;
    for (const auto& token : context.tokens) {
        if (token.type == analysis::TokenType::Identifier && 
            token.text != context.currentWord) {
            identifiers.insert(token.text);
        }
    }
    
    // Convert to completion items
    for (const auto& identifier : identifiers) {
        completions.emplace_back(identifier, CompletionItemKind::Variable, 
                                "Variable", "User-defined identifier");
    }
    
    return completions;
}

std::string CompletionProvider::getCurrentWord(const std::string& content, size_t line, size_t character) {
    std::istringstream stream(content);
    std::string currentLine;
    
    // Get the specific line
    for (size_t i = 0; i <= line && std::getline(stream, currentLine); ++i) {
        if (i == line) {
            break;
        }
    }
    
    if (character > currentLine.length()) {
        return "";
    }
    
    // Find word boundaries
    size_t start = character;
    size_t end = character;
    
    // Move start backward to find word start
    while (start > 0) {
        char ch = currentLine[start - 1];
        if (!std::isalnum(ch) && ch != '_') {
            break;
        }
        start--;
    }
    
    // Move end forward to find word end
    while (end < currentLine.length()) {
        char ch = currentLine[end];
        if (!std::isalnum(ch) && ch != '_') {
            break;
        }
        end++;
    }
    
    return currentLine.substr(start, end - start);
}

analysis::Token CompletionProvider::getPreviousToken(const std::vector<analysis::Token>& tokens, 
                                                     size_t line, size_t character) {
    analysis::Token previousToken;
    
    for (const auto& token : tokens) {
        if (token.range.end.line < line || 
            (token.range.end.line == line && token.range.end.column < character)) {
            previousToken = token;
        } else {
            break;
        }
    }
    
    return previousToken;
}

bool CompletionProvider::shouldTriggerCompletion(const LegacyCompletionContext& context) {
    (void)context; // Mark as intentionally unused for now
    return true; // Always trigger for now
}

std::vector<CompletionItem> CompletionProvider::filterCompletions(const std::vector<CompletionItem>& items, 
                                                                 const std::string& prefix) {
    std::vector<CompletionItem> filtered;
    
    for (const auto& item : items) {
        if (item.label.find(prefix) == 0) { // Starts with prefix
            filtered.push_back(item);
        }
    }
    
    return filtered;
}

std::vector<CompletionItem> CompletionProvider::getKeywordCompletions() {
    static std::vector<CompletionItem> keywords = {
        // Control flow keywords
        {"اذا", CompletionItemKind::Keyword, "if statement", "Conditional statement"},
        {"إذا", CompletionItemKind::Keyword, "if statement", "Conditional statement"},
        {"والا", CompletionItemKind::Keyword, "else statement", "Else clause"},
        {"وإلا", CompletionItemKind::Keyword, "else statement", "Else clause"},
        {"اواذا", CompletionItemKind::Keyword, "elif statement", "Else-if clause"},
        {"أوإذا", CompletionItemKind::Keyword, "elif statement", "Else-if clause"},
        {"بينما", CompletionItemKind::Keyword, "while loop", "While loop statement"},
        {"لاجل", CompletionItemKind::Keyword, "for loop", "For loop statement"},
        {"لأجل", CompletionItemKind::Keyword, "for loop", "For loop statement"},
        
        // Function and class keywords
        {"دالة", CompletionItemKind::Keyword, "function", "Function definition"},
        {"صنف", CompletionItemKind::Keyword, "class", "Class definition"},
        {"ارجع", CompletionItemKind::Keyword, "return", "Return statement"},
        
        // Other keywords
        {"في", CompletionItemKind::Keyword, "in", "Membership operator"},
        {"من", CompletionItemKind::Keyword, "from", "Import from"},
        {"استورد", CompletionItemKind::Keyword, "import", "Import statement"},
        {"حاول", CompletionItemKind::Keyword, "try", "Try statement"},
        {"خلل", CompletionItemKind::Keyword, "except", "Exception handler"},
        {"نهاية", CompletionItemKind::Keyword, "finally", "Finally clause"},
    };
    
    return keywords;
}

std::vector<CompletionItem> CompletionProvider::getBuiltinCompletions() {
    static std::vector<CompletionItem> builtins = {
        {"اطبع", CompletionItemKind::Function, "print function", "Print output to console"},
        {"ادخل", CompletionItemKind::Function, "input function", "Get user input"},
        {"مدى", CompletionItemKind::Function, "range function", "Generate range of numbers"},
    };
    
    return builtins;
}

CompletionItemKind CompletionProvider::tokenTypeToCompletionKind(analysis::TokenType type) {
    switch (type) {
        case analysis::TokenType::Keyword:
        case analysis::TokenType::Keyword1:
        case analysis::TokenType::Keyword2:
            return CompletionItemKind::Keyword;
        case analysis::TokenType::Identifier:
            return CompletionItemKind::Variable;
        default:
            return CompletionItemKind::Text;
    }
}

nlohmann::json CompletionProvider::completionItemToJson(const CompletionItem& item) {
    nlohmann::json json = nlohmann::json::object();
    json["label"] = item.label;
    json["kind"] = static_cast<int>(item.kind);
    json["detail"] = item.detail;
    json["documentation"] = item.documentation;
    json["insertText"] = item.insertText;
    json["preselect"] = item.preselect;
    
    return json;
}

// Arabic Completion Implementation
std::vector<ArabicCompletionItem> CompletionProvider::provideArabicCompletions(const CompletionContext& context) {
    // Initialize the Arabic completion database
    ArabicCompletionDatabase::initialize();

    // Get all available completions
    std::vector<ArabicCompletionItem> allCompletions = ArabicCompletionDatabase::getAllCompletions();

    // Add code snippets as completion items
    auto snippets = ArabicCompletionDatabase::getBuiltinSnippets();
    for (const auto& snippet : snippets) {
        allCompletions.push_back(snippet.toCompletionItem());
    }

    // Analyze context and filter completions
    std::vector<ArabicCompletionItem> contextualCompletions = getContextualArabicCompletions(context);

    // Merge contextual completions with built-in ones
    allCompletions.insert(allCompletions.end(), contextualCompletions.begin(), contextualCompletions.end());

    // Filter by current word
    std::vector<ArabicCompletionItem> filteredCompletions = filterArabicCompletions(allCompletions, context.currentWord);

    // Sort by priority and relevance
    std::sort(filteredCompletions.begin(), filteredCompletions.end(),
             [this, &context](const ArabicCompletionItem& a, const ArabicCompletionItem& b) {
                 int priorityA = calculateCompletionPriority(a, context);
                 int priorityB = calculateCompletionPriority(b, context);
                 if (priorityA != priorityB) return priorityA > priorityB;
                 return a.priority > b.priority;
             });

    // Limit results to prevent overwhelming the user
    if (filteredCompletions.size() > 50) {
        filteredCompletions.resize(50);
    }

    return filteredCompletions;
}

std::vector<ArabicCompletionItem> CompletionProvider::getContextualArabicCompletions(const CompletionContext& context) {
    std::vector<ArabicCompletionItem> contextualCompletions;

    // Analyze the current context to provide relevant completions
    CompletionContext::Type contextType = analyzeCompletionContext(context);

    std::string contextString;
    switch (contextType) {
        case CompletionContext::Type::Global:
            contextString = "global";
            break;
        case CompletionContext::Type::FunctionBody:
            contextString = "function";
            break;
        case CompletionContext::Type::ClassBody:
            contextString = "class";
            break;
        case CompletionContext::Type::IfCondition:
            contextString = "condition";
            break;
        case CompletionContext::Type::LoopBody:
            contextString = "loop";
            break;
        default:
            contextString = "global";
            break;
    }

    // Get completions for this context
    contextualCompletions = ArabicCompletionDatabase::getCompletionsForContext(contextString);

    // TODO: Add variable and function completions from current scope
    // This would require more sophisticated analysis of the document

    return contextualCompletions;
}

std::vector<ArabicCompletionItem> CompletionProvider::filterArabicCompletions(
    const std::vector<ArabicCompletionItem>& items, const std::string& prefix) {

    if (prefix.empty()) {
        return items;
    }

    std::vector<ArabicCompletionItem> filtered;

    for (const auto& item : items) {
        // Check if item matches the prefix
        bool matches = false;

        // Check Arabic name
        if (item.arabicName.find(prefix) == 0) {
            matches = true;
        }
        // Check label
        else if (item.label.find(prefix) == 0) {
            matches = true;
        }
        // Check filter text
        else if (!item.filterText.empty() && item.filterText.find(prefix) == 0) {
            matches = true;
        }
        // Fuzzy matching for Arabic text
        else {
            // Simple fuzzy matching - can be improved
            std::string lowerPrefix = prefix;
            std::string lowerLabel = item.arabicName;
            std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);
            std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::tolower);

            if (lowerLabel.find(lowerPrefix) != std::string::npos) {
                matches = true;
            }
        }

        if (matches) {
            filtered.push_back(item);
        }
    }

    return filtered;
}

CompletionContext::Type CompletionProvider::analyzeCompletionContext(const CompletionContext& context) {
    // Simple context analysis based on current scope
    // TODO: Implement more sophisticated analysis using document parsing

    // For now, use the type that's already set in the context
    return context.type;
}

int CompletionProvider::calculateCompletionPriority(const ArabicCompletionItem& item, const CompletionContext& context) {
    int priority = item.priority;

    // Boost priority based on context relevance
    CompletionContext::Type contextType = analyzeCompletionContext(context);

    // Boost items that are applicable in current context
    std::string contextString;
    switch (contextType) {
        case CompletionContext::Type::FunctionBody:
            contextString = "function";
            break;
        case CompletionContext::Type::ClassBody:
            contextString = "class";
            break;
        case CompletionContext::Type::Global:
            contextString = "global";
            break;
        default:
            contextString = "global";
            break;
    }

    if (item.isApplicableInContext(contextString)) {
        priority += 20;
    }

    // Boost based on prefix match quality
    if (!context.currentWord.empty()) {
        if (item.arabicName.find(context.currentWord) == 0) {
            priority += 30; // Exact prefix match
        } else if (item.arabicName.find(context.currentWord) != std::string::npos) {
            priority += 10; // Contains match
        }
    }

    // Boost frequently used items (keywords, basic functions)
    if (item.hasTag("basic") || item.hasTag("beginner")) {
        priority += 15;
    }

    return priority;
}

} // namespace features
} // namespace als
