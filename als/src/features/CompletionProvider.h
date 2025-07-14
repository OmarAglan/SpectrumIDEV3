/**
 * @file CompletionProvider.h
 * @brief LSP completion provider for Alif language
 */

#pragma once

#include "../analysis/Lexer.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace als {
namespace features {

/**
 * @brief LSP completion item kinds
 */
enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};

/**
 * @brief Completion item structure
 */
struct CompletionItem {
    std::string label;              // Display text
    CompletionItemKind kind;        // Item kind
    std::string detail;             // Additional detail
    std::string documentation;      // Documentation string
    std::string insertText;         // Text to insert
    bool preselect;                 // Should be preselected
    
    CompletionItem(const std::string& lbl, CompletionItemKind k, 
                   const std::string& det = "", const std::string& doc = "",
                   const std::string& insert = "", bool presel = false)
        : label(lbl), kind(k), detail(det), documentation(doc), 
          insertText(insert.empty() ? lbl : insert), preselect(presel) {}
};

/**
 * @brief Completion context information
 */
struct CompletionContext {
    std::string documentUri;        // Document URI
    std::string documentContent;    // Full document content
    size_t line;                    // 0-based line number
    size_t character;               // 0-based character position
    std::string currentWord;        // Word being typed
    analysis::Token previousToken;  // Previous token for context
    std::vector<analysis::Token> tokens; // All document tokens
};

/**
 * @brief Completion provider for Alif language
 */
class CompletionProvider {
public:
    /**
     * @brief Constructor
     */
    CompletionProvider();
    
    /**
     * @brief Destructor
     */
    ~CompletionProvider();
    
    /**
     * @brief Provide completions for given context
     * @param context Completion context
     * @return Vector of completion items
     */
    std::vector<CompletionItem> provideCompletions(const CompletionContext& context);
    
    /**
     * @brief Convert completion items to LSP JSON format
     * @param items Completion items
     * @return JSON array of completion items
     */
    nlohmann::json toJson(const std::vector<CompletionItem>& items);
    
    /**
     * @brief Create completion context from LSP request parameters
     * @param documentUri Document URI
     * @param documentContent Document content
     * @param line Line number (0-based)
     * @param character Character position (0-based)
     * @return Completion context
     */
    CompletionContext createContext(const std::string& documentUri,
                                   const std::string& documentContent,
                                   size_t line, size_t character);

private:
    analysis::Lexer lexer_;
    
    // Completion providers for different contexts
    std::vector<CompletionItem> provideKeywordCompletions(const CompletionContext& context);
    std::vector<CompletionItem> provideBuiltinCompletions(const CompletionContext& context);
    std::vector<CompletionItem> provideIdentifierCompletions(const CompletionContext& context);
    
    // Helper methods
    std::string getCurrentWord(const std::string& content, size_t line, size_t character);
    analysis::Token getPreviousToken(const std::vector<analysis::Token>& tokens, size_t line, size_t character);
    bool shouldTriggerCompletion(const CompletionContext& context);
    std::vector<CompletionItem> filterCompletions(const std::vector<CompletionItem>& items, 
                                                  const std::string& prefix);
    
    // Static completion data
    static std::vector<CompletionItem> getKeywordCompletions();
    static std::vector<CompletionItem> getBuiltinCompletions();
    
    // Utility methods
    CompletionItemKind tokenTypeToCompletionKind(analysis::TokenType type);
    nlohmann::json completionItemToJson(const CompletionItem& item);
};

} // namespace features
} // namespace als
