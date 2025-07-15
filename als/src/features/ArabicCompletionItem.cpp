#include "ArabicCompletionItem.h"
#include <algorithm>

namespace als {
namespace features {

// ParameterInfo implementation
nlohmann::json ParameterInfo::toJson() const {
    return nlohmann::json{
        {"name", name},
        {"type", type},
        {"arabicDescription", arabicDescription},
        {"isOptional", isOptional},
        {"defaultValue", defaultValue}
    };
}

ParameterInfo ParameterInfo::fromJson(const nlohmann::json& json) {
    ParameterInfo param;
    param.name = json.value("name", "");
    param.type = json.value("type", "");
    param.arabicDescription = json.value("arabicDescription", "");
    param.isOptional = json.value("isOptional", false);
    param.defaultValue = json.value("defaultValue", "");
    return param;
}

// ArabicCompletionItem implementation
ArabicCompletionItem::ArabicCompletionItem(const std::string& label, CompletionItemKind kind)
    : label(label), arabicName(label), kind(kind) {
    // Set default values
    insertText = label;
    filterText = label;
    sortText = label;
}

nlohmann::json ArabicCompletionItem::toJson() const {
    nlohmann::json json;
    
    // Basic LSP completion item fields
    json["label"] = label;
    json["kind"] = static_cast<int>(kind);
    json["insertText"] = insertText;
    json["filterText"] = filterText;
    json["sortText"] = sortText;
    
    // Arabic-specific fields
    json["arabicName"] = arabicName;
    json["englishName"] = englishName;
    json["arabicDescription"] = arabicDescription;
    json["arabicDetailedDesc"] = arabicDetailedDesc;
    json["usageExample"] = usageExample;
    json["arabicExample"] = arabicExample;
    
    // Function-specific fields
    if (!parameters.empty()) {
        nlohmann::json paramsJson = nlohmann::json::array();
        for (const auto& param : parameters) {
            paramsJson.push_back(param.toJson());
        }
        json["parameters"] = paramsJson;
    }
    
    json["returnType"] = returnType;
    json["arabicReturnDesc"] = arabicReturnDesc;
    
    // Metadata
    json["priority"] = priority;
    json["contexts"] = contexts;
    json["tags"] = tags;
    json["category"] = category;
    
    return json;
}

ArabicCompletionItem ArabicCompletionItem::fromJson(const nlohmann::json& json) {
    ArabicCompletionItem item;
    
    // Basic fields
    item.label = json.value("label", "");
    item.kind = static_cast<CompletionItemKind>(json.value("kind", 1));
    item.insertText = json.value("insertText", item.label);
    item.filterText = json.value("filterText", item.label);
    item.sortText = json.value("sortText", item.label);
    
    // Arabic-specific fields
    item.arabicName = json.value("arabicName", item.label);
    item.englishName = json.value("englishName", "");
    item.arabicDescription = json.value("arabicDescription", "");
    item.arabicDetailedDesc = json.value("arabicDetailedDesc", "");
    item.usageExample = json.value("usageExample", "");
    item.arabicExample = json.value("arabicExample", "");
    
    // Function-specific fields
    if (json.contains("parameters") && json["parameters"].is_array()) {
        for (const auto& paramJson : json["parameters"]) {
            item.parameters.push_back(ParameterInfo::fromJson(paramJson));
        }
    }
    
    item.returnType = json.value("returnType", "");
    item.arabicReturnDesc = json.value("arabicReturnDesc", "");
    
    // Metadata
    item.priority = json.value("priority", 50);
    item.contexts = json.value("contexts", std::vector<std::string>{});
    item.tags = json.value("tags", std::vector<std::string>{});
    item.category = json.value("category", "");
    
    return item;
}

bool ArabicCompletionItem::isApplicableInContext(const std::string& context) const {
    if (contexts.empty()) return true; // Applicable everywhere if no contexts specified
    return std::find(contexts.begin(), contexts.end(), context) != contexts.end();
}

bool ArabicCompletionItem::hasTag(const std::string& tag) const {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

std::string ArabicCompletionItem::getDisplayText() const {
    return arabicName.empty() ? label : arabicName;
}

std::string ArabicCompletionItem::getDetailText() const {
    std::string detail = arabicDescription;
    if (!returnType.empty()) {
        detail += " → " + returnType;
    }
    return detail;
}

// CodeSnippet implementation
nlohmann::json CodeSnippet::toJson() const {
    return nlohmann::json{
        {"name", name},
        {"description", description},
        {"template", template_},
        {"placeholders", placeholders},
        {"category", category},
        {"priority", priority},
        {"contexts", contexts}
    };
}

CodeSnippet CodeSnippet::fromJson(const nlohmann::json& json) {
    CodeSnippet snippet;
    snippet.name = json.value("name", "");
    snippet.description = json.value("description", "");
    snippet.template_ = json.value("template", "");
    snippet.placeholders = json.value("placeholders", std::vector<std::string>{});
    snippet.category = json.value("category", "");
    snippet.priority = json.value("priority", 50);
    snippet.contexts = json.value("contexts", std::vector<std::string>{});
    return snippet;
}

ArabicCompletionItem CodeSnippet::toCompletionItem() const {
    ArabicCompletionItem item(name, CompletionItemKind::Snippet);
    item.arabicName = name;
    item.arabicDescription = description;
    item.insertText = template_;
    item.category = category;
    item.priority = priority;
    item.contexts = contexts;
    item.tags.push_back("snippet");
    return item;
}

// CompletionContext implementation
nlohmann::json CompletionContext::toJson() const {
    return nlohmann::json{
        {"type", static_cast<int>(type)},
        {"currentScope", currentScope},
        {"availableVariables", availableVariables},
        {"availableFunctions", availableFunctions},
        {"availableClasses", availableClasses},
        {"cursorLine", cursorLine},
        {"cursorColumn", cursorColumn},
        {"currentWord", currentWord}
    };
}

CompletionContext CompletionContext::fromJson(const nlohmann::json& json) {
    CompletionContext context;
    context.type = static_cast<Type>(json.value("type", 0));
    context.currentScope = json.value("currentScope", "");
    context.availableVariables = json.value("availableVariables", std::vector<std::string>{});
    context.availableFunctions = json.value("availableFunctions", std::vector<std::string>{});
    context.availableClasses = json.value("availableClasses", std::vector<std::string>{});
    context.cursorLine = json.value("cursorLine", 0);
    context.cursorColumn = json.value("cursorColumn", 0);
    context.currentWord = json.value("currentWord", "");
    return context;
}

// CompletionRequest implementation
nlohmann::json CompletionRequest::toJson() const {
    return nlohmann::json{
        {"uri", uri},
        {"line", line},
        {"character", character},
        {"context", context.toJson()},
        {"triggerCharacter", triggerCharacter},
        {"isRetrigger", isRetrigger}
    };
}

CompletionRequest CompletionRequest::fromJson(const nlohmann::json& json) {
    CompletionRequest request;
    request.uri = json.value("uri", "");
    request.line = json.value("line", 0);
    request.character = json.value("character", 0);
    if (json.contains("context")) {
        request.context = CompletionContext::fromJson(json["context"]);
    }
    request.triggerCharacter = json.value("triggerCharacter", "");
    request.isRetrigger = json.value("isRetrigger", false);
    return request;
}

// CompletionResponse implementation
nlohmann::json CompletionResponse::toJson() const {
    nlohmann::json itemsJson = nlohmann::json::array();
    for (const auto& item : items) {
        itemsJson.push_back(item.toJson());
    }
    
    return nlohmann::json{
        {"items", itemsJson},
        {"isIncomplete", isIncomplete},
        {"contextInfo", contextInfo}
    };
}

CompletionResponse CompletionResponse::fromJson(const nlohmann::json& json) {
    CompletionResponse response;
    
    if (json.contains("items") && json["items"].is_array()) {
        for (const auto& itemJson : json["items"]) {
            response.items.push_back(ArabicCompletionItem::fromJson(itemJson));
        }
    }
    
    response.isIncomplete = json.value("isIncomplete", false);
    response.contextInfo = json.value("contextInfo", "");
    return response;
}

} // namespace features
} // namespace als
