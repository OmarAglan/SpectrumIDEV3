#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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
 * @brief Parameter information for function completions
 */
struct ParameterInfo {
    std::string name;                    // "النص"
    std::string type;                    // "نص"
    std::string arabicDescription;       // "النص المراد طباعته"
    bool isOptional = false;
    std::string defaultValue;
    
    // JSON serialization
    nlohmann::json toJson() const;
    static ParameterInfo fromJson(const nlohmann::json& json);
};

/**
 * @brief Enhanced completion item with rich Arabic metadata
 */
struct ArabicCompletionItem {
    // Basic completion data
    std::string label;                   // "اطبع"
    std::string arabicName;             // "اطبع"
    std::string englishName;            // "print"
    CompletionItemKind kind;            // Function, Keyword, Variable, etc.
    
    // Arabic-specific metadata
    std::string arabicDescription;      // "يطبع النص المحدد إلى وحدة التحكم"
    std::string arabicDetailedDesc;     // Extended Arabic explanation
    std::string usageExample;           // "اطبع(\"مرحبا بالعالم\")"
    std::string arabicExample;          // Full code example with Arabic comments
    
    // Function-specific data
    std::vector<ParameterInfo> parameters;
    std::string returnType;
    std::string arabicReturnDesc;
    
    // Context and priority
    int priority = 50;                  // 1-100 relevance score
    std::vector<std::string> contexts;  // ["global", "function", "class"]
    std::vector<std::string> tags;      // ["io", "basic", "beginner"]
    
    // Additional metadata
    std::string category;               // "control_flow", "io", "math", etc.
    std::string insertText;             // Text to insert (may differ from label)
    std::string filterText;             // Text used for filtering
    std::string sortText;               // Text used for sorting
    
    // Constructor
    ArabicCompletionItem() = default;
    ArabicCompletionItem(const std::string& label, CompletionItemKind kind);
    
    // Conversion methods
    nlohmann::json toJson() const;
    static ArabicCompletionItem fromJson(const nlohmann::json& json);
    
    // Utility methods
    bool isApplicableInContext(const std::string& context) const;
    bool hasTag(const std::string& tag) const;
    std::string getDisplayText() const;
    std::string getDetailText() const;
};

/**
 * @brief Code snippet template for advanced completions
 */
struct CodeSnippet {
    std::string name;                   // "حلقة للعد"
    std::string description;            // "حلقة for للعد من 1 إلى رقم محدد"
    std::string template_;              // Template with placeholders
    std::vector<std::string> placeholders; // ["${1:البداية}", "${2:النهاية}"]
    std::string category;               // "control_flow"
    int priority = 50;
    std::vector<std::string> contexts;  // Where this snippet is applicable
    
    // JSON serialization
    nlohmann::json toJson() const;
    static CodeSnippet fromJson(const nlohmann::json& json);
    
    // Convert to completion item
    ArabicCompletionItem toCompletionItem() const;
};

/**
 * @brief Context information for smart completions
 */
struct CompletionContext {
    enum Type {
        Global,           // Top-level code
        FunctionBody,     // Inside function
        ClassBody,        // Inside class
        IfCondition,      // Inside if condition
        LoopBody,         // Inside loop
        FunctionCall,     // Function parameters
        Assignment,       // Right side of assignment
        Import           // Import statement
    };
    
    Type type = Global;
    std::string currentScope;
    std::vector<std::string> availableVariables;
    std::vector<std::string> availableFunctions;
    std::vector<std::string> availableClasses;
    int cursorLine = 0;
    int cursorColumn = 0;
    std::string currentWord;            // Word being typed
    
    // JSON serialization
    nlohmann::json toJson() const;
    static CompletionContext fromJson(const nlohmann::json& json);
};

/**
 * @brief Completion request with context
 */
struct CompletionRequest {
    std::string uri;
    int line;
    int character;
    CompletionContext context;
    std::string triggerCharacter;       // Character that triggered completion
    bool isRetrigger = false;           // Is this a re-trigger?
    
    nlohmann::json toJson() const;
    static CompletionRequest fromJson(const nlohmann::json& json);
};

/**
 * @brief Completion response with rich items
 */
struct CompletionResponse {
    std::vector<ArabicCompletionItem> items;
    bool isIncomplete = false;          // More items available?
    std::string contextInfo;            // Additional context information
    
    nlohmann::json toJson() const;
    static CompletionResponse fromJson(const nlohmann::json& json);
};

} // namespace features
} // namespace als
