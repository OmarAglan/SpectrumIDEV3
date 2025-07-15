#pragma once

#include "ArabicCompletionItem.h"
#include <vector>
#include <map>
#include <string>

namespace als {
namespace features {

/**
 * @brief Comprehensive database of Arabic completion items
 */
class ArabicCompletionDatabase {
public:
    /**
     * @brief Get all built-in completion items
     */
    static std::vector<ArabicCompletionItem> getAllCompletions();
    
    /**
     * @brief Get completions by category
     */
    static std::vector<ArabicCompletionItem> getCompletionsByCategory(const std::string& category);
    
    /**
     * @brief Get completions by context
     */
    static std::vector<ArabicCompletionItem> getCompletionsForContext(const std::string& context);
    
    /**
     * @brief Get built-in code snippets
     */
    static std::vector<CodeSnippet> getBuiltinSnippets();
    
    /**
     * @brief Initialize the database (load from files if needed)
     */
    static void initialize();
    
    /**
     * @brief Add custom completion item
     */
    static void addCustomCompletion(const ArabicCompletionItem& item);
    
    /**
     * @brief Get completion by label
     */
    static ArabicCompletionItem* findCompletion(const std::string& label);

private:
    // Core completion categories
    static std::vector<ArabicCompletionItem> getIOCompletions();
    static std::vector<ArabicCompletionItem> getControlFlowCompletions();
    static std::vector<ArabicCompletionItem> getDataTypeCompletions();
    static std::vector<ArabicCompletionItem> getMathCompletions();
    static std::vector<ArabicCompletionItem> getStringCompletions();
    static std::vector<ArabicCompletionItem> getArrayCompletions();
    static std::vector<ArabicCompletionItem> getFunctionCompletions();
    static std::vector<ArabicCompletionItem> getClassCompletions();
    static std::vector<ArabicCompletionItem> getErrorHandlingCompletions();
    static std::vector<ArabicCompletionItem> getFileIOCompletions();
    
    // Snippet categories
    static std::vector<CodeSnippet> getControlFlowSnippets();
    static std::vector<CodeSnippet> getFunctionSnippets();
    static std::vector<CodeSnippet> getClassSnippets();
    static std::vector<CodeSnippet> getCommonPatternSnippets();
    
    // Helper methods
    static ArabicCompletionItem createFunction(
        const std::string& arabicName,
        const std::string& englishName,
        const std::string& description,
        const std::string& detailedDesc,
        const std::vector<ParameterInfo>& params,
        const std::string& returnType = "",
        const std::string& returnDesc = "",
        int priority = 50
    );
    
    static ArabicCompletionItem createKeyword(
        const std::string& arabicName,
        const std::string& englishName,
        const std::string& description,
        const std::string& detailedDesc,
        const std::string& example,
        int priority = 50
    );
    
    static ParameterInfo createParam(
        const std::string& name,
        const std::string& type,
        const std::string& description,
        bool optional = false,
        const std::string& defaultValue = ""
    );
    
    // Static data
    static std::vector<ArabicCompletionItem> s_allCompletions;
    static std::vector<CodeSnippet> s_allSnippets;
    static std::map<std::string, std::vector<ArabicCompletionItem>> s_completionsByCategory;
    static bool s_initialized;
};

} // namespace features
} // namespace als
