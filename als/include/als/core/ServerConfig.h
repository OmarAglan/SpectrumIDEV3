/**
 * @file ServerConfig.h
 * @brief Server configuration management
 */

#pragma once

#include <string>
#include <unordered_map>

namespace als {
namespace core {

/**
 * @brief Server configuration class
 * 
 * Manages server configuration including:
 * - Performance settings
 * - Feature toggles
 * - Logging configuration
 * - Analysis options
 */
class ServerConfig {
public:
    /**
     * @brief Default constructor with default values
     */
    ServerConfig();
    
    /**
     * @brief Load configuration from file
     * @param filename Configuration file path
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Save configuration to file
     * @param filename Configuration file path
     * @return true if saved successfully
     */
    bool saveToFile(const std::string& filename) const;
    
    // Server settings
    int getMaxCachedDocuments() const { return maxCachedDocuments_; }
    void setMaxCachedDocuments(int value) { maxCachedDocuments_ = value; }
    
    int getCompletionTimeout() const { return completionTimeout_; }
    void setCompletionTimeout(int value) { completionTimeout_ = value; }
    
    int getDiagnosticsDelay() const { return diagnosticsDelay_; }
    void setDiagnosticsDelay(int value) { diagnosticsDelay_ = value; }
    
    int getMaxWorkerThreads() const { return maxWorkerThreads_; }
    void setMaxWorkerThreads(int value) { maxWorkerThreads_ = value; }
    
    const std::string& getLogLevel() const { return logLevel_; }
    void setLogLevel(const std::string& level) { logLevel_ = level; }
    
    // Analysis settings
    bool isSemanticAnalysisEnabled() const { return enableSemanticAnalysis_; }
    void setSemanticAnalysisEnabled(bool enabled) { enableSemanticAnalysis_ = enabled; }
    
    bool isTypeInferenceEnabled() const { return enableTypeInference_; }
    void setTypeInferenceEnabled(bool enabled) { enableTypeInference_ = enabled; }
    
    // Completion settings
    bool areSnippetsEnabled() const { return enableSnippets_; }
    void setSnippetsEnabled(bool enabled) { enableSnippets_ = enabled; }
    
    int getMaxSuggestions() const { return maxSuggestions_; }
    void setMaxSuggestions(int value) { maxSuggestions_ = value; }
    
    // Diagnostics settings
    bool isLintingEnabled() const { return enableLinting_; }
    void setLintingEnabled(bool enabled) { enableLinting_ = enabled; }

private:
    // Server configuration
    int maxCachedDocuments_;
    int completionTimeout_;
    int diagnosticsDelay_;
    int maxWorkerThreads_;
    std::string logLevel_;
    
    // Analysis configuration
    bool enableSemanticAnalysis_;
    bool enableTypeInference_;
    
    // Completion configuration
    bool enableSnippets_;
    int maxSuggestions_;
    
    // Diagnostics configuration
    bool enableLinting_;
    
    /**
     * @brief Set default configuration values
     */
    void setDefaults();
};

} // namespace core
} // namespace als
