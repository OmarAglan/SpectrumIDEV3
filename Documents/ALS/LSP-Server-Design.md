# Alif Language Server - LSP Server Design

## Overview

This document provides detailed design specifications for the Language Server Protocol (LSP) implementation in the Alif Language Server (ALS). It covers the protocol compliance, message handling, feature implementation, and integration patterns.

## LSP Protocol Compliance

### Supported LSP Version
- **Target Version**: LSP 3.17 (latest stable)
- **Minimum Version**: LSP 3.16 (for broader client compatibility)
- **Protocol Transport**: JSON-RPC 2.0 over stdio

### Server Capabilities

#### Core Capabilities
```json
{
  "textDocumentSync": {
    "openClose": true,
    "change": 2,  // Incremental
    "willSave": false,
    "willSaveWaitUntil": false,
    "save": {
      "includeText": false
    }
  },
  "completionProvider": {
    "resolveProvider": true,
    "triggerCharacters": [".", "(", "[", "{", " "],
    "allCommitCharacters": ["\t", "\n", " "]
  },
  "hoverProvider": true,
  "definitionProvider": true,
  "referencesProvider": true,
  "documentSymbolProvider": true,
  "workspaceSymbolProvider": true,
  "codeActionProvider": {
    "codeActionKinds": [
      "quickfix",
      "refactor",
      "refactor.extract",
      "refactor.inline",
      "refactor.rewrite",
      "source",
      "source.organizeImports"
    ]
  },
  "diagnosticProvider": {
    "interFileDependencies": true,
    "workspaceDiagnostics": true
  }
}
```

#### Advanced Capabilities
```json
{
  "renameProvider": {
    "prepareProvider": true
  },
  "foldingRangeProvider": true,
  "selectionRangeProvider": true,
  "semanticTokensProvider": {
    "legend": {
      "tokenTypes": ["keyword", "string", "comment", "number", "operator", "function", "variable", "class"],
      "tokenModifiers": ["declaration", "definition", "readonly", "static", "deprecated"]
    },
    "range": true,
    "full": {
      "delta": true
    }
  },
  "inlayHintProvider": {
    "resolveProvider": false
  }
}
```

## Message Flow Architecture

### Request-Response Pattern

```
Client                    ALS Server
  │                          │
  ├─ initialize ────────────▶ │
  │                          ├─ Process capabilities
  │                          ├─ Initialize workspace
  │ ◄──────── InitializeResult │
  │                          │
  ├─ initialized ───────────▶ │
  │                          ├─ Start background indexing
  │                          │
  ├─ textDocument/didOpen ──▶ │
  │                          ├─ Parse document
  │                          ├─ Analyze semantics
  │ ◄─ textDocument/publishDiagnostics
  │                          │
  ├─ textDocument/completion ▶ │
  │                          ├─ Analyze context
  │                          ├─ Generate suggestions
  │ ◄──────── CompletionList │
```

### Notification Flow

```
Client                    ALS Server
  │                          │
  ├─ textDocument/didChange ▶ │
  │                          ├─ Update document
  │                          ├─ Schedule reparse (debounced)
  │                          │
  │                          ├─ Background: Parse & analyze
  │ ◄─ textDocument/publishDiagnostics
  │                          │
  ├─ textDocument/didSave ──▶ │
  │                          ├─ Trigger project-wide analysis
  │                          │
  ├─ textDocument/didClose ─▶ │
  │                          ├─ Clean up document state
```

## Core LSP Feature Implementation

### 1. Text Document Synchronization

#### Document State Management
```cpp
class Document {
private:
    std::string uri_;
    std::string content_;
    int version_;
    std::shared_ptr<AST> ast_;
    std::shared_ptr<SymbolTable> symbols_;
    std::vector<Diagnostic> diagnostics_;
    std::chrono::steady_clock::time_point last_modified_;
    
public:
    void updateContent(const std::string& content, int version);
    void applyIncrementalChanges(const std::vector<TextDocumentContentChangeEvent>& changes);
    bool needsReparse() const;
    void scheduleReparse();
};
```

#### Change Processing Strategy
- **Incremental Updates**: Apply TextDocumentContentChangeEvent efficiently
- **Debouncing**: Delay parsing for 250ms to batch rapid changes
- **Version Tracking**: Ensure consistency between client and server state
- **Conflict Resolution**: Handle out-of-order change notifications

### 2. Code Completion (textDocument/completion)

#### Completion Context Analysis
```cpp
struct CompletionContext {
    Position position;
    std::string trigger_character;
    CompletionTriggerKind trigger_kind;
    
    // Derived context
    std::shared_ptr<ASTNode> containing_node;
    std::vector<Symbol> visible_symbols;
    std::string partial_identifier;
    bool in_string_literal;
    bool in_comment;
};
```

#### Completion Providers
- **Keyword Completion**: Alif language keywords based on context
- **Symbol Completion**: Variables, functions, classes in scope
- **Member Completion**: Object/class member access (obj.member)
- **Import Completion**: Module and package names
- **Snippet Completion**: Code templates with placeholders

#### Performance Optimizations
- **Symbol Indexing**: Pre-built symbol index for fast lookups
- **Context Caching**: Cache completion contexts for repeated requests
- **Fuzzy Matching**: Efficient fuzzy string matching for filtering
- **Result Limiting**: Return top 50 results, support pagination

### 3. Hover Information (textDocument/hover)

#### Hover Content Generation
```cpp
struct HoverInfo {
    std::string symbol_name;
    std::string symbol_type;
    std::string documentation;
    std::vector<std::string> signatures;
    Range definition_range;
    std::string file_path;
};
```

#### Information Sources
- **Symbol Definitions**: Function signatures, variable types
- **Documentation**: Extracted from comments and docstrings
- **Type Information**: Inferred or declared types
- **Usage Examples**: Common usage patterns
- **Related Symbols**: Links to related definitions

### 4. Go to Definition (textDocument/definition)

#### Definition Resolution Strategy
1. **Symbol Identification**: Identify symbol at cursor position
2. **Scope Resolution**: Find symbol in appropriate scope chain
3. **Cross-File Resolution**: Handle imports and module references
4. **Multiple Definitions**: Handle overloaded functions/methods
5. **Fallback Strategies**: Fuzzy matching for partial matches

#### Definition Types
- **Variable Definitions**: Local, parameter, global variables
- **Function Definitions**: Function and method declarations
- **Class Definitions**: Class and interface declarations
- **Import Definitions**: Module and package sources
- **Built-in Definitions**: Standard library symbols

### 5. Find References (textDocument/references)

#### Reference Search Algorithm
```cpp
class ReferenceSearcher {
public:
    std::vector<Location> findReferences(
        const Symbol& symbol,
        bool include_declaration,
        const WorkspaceFolder& scope
    );
    
private:
    void searchInDocument(const Document& doc, const Symbol& symbol);
    void searchInWorkspace(const Symbol& symbol);
    bool isSymbolReference(const ASTNode& node, const Symbol& symbol);
};
```

#### Search Optimizations
- **Index-Based Search**: Use pre-built symbol index
- **Incremental Search**: Search recently modified files first
- **Parallel Search**: Multi-threaded search across files
- **Result Deduplication**: Remove duplicate references
- **Context Filtering**: Filter false positives using AST context

### 6. Diagnostics (textDocument/publishDiagnostics)

#### Diagnostic Categories
```cpp
enum class DiagnosticCategory {
    SYNTAX_ERROR,      // Parser errors
    SEMANTIC_ERROR,    // Undefined symbols, type errors
    WARNING,           // Potential issues, unused variables
    STYLE,             // Code style violations
    PERFORMANCE,       // Performance recommendations
    SECURITY           // Security-related issues
};
```

#### Diagnostic Generation Pipeline
1. **Syntax Diagnostics**: Generated during parsing phase
2. **Semantic Diagnostics**: Generated during semantic analysis
3. **Linting Diagnostics**: Generated by configurable rules
4. **Cross-File Diagnostics**: Generated during project analysis
5. **Diagnostic Aggregation**: Combine and prioritize diagnostics

## Advanced LSP Features

### 1. Code Actions (textDocument/codeAction)

#### Quick Fix Actions
- **Import Missing Symbol**: Add import statements automatically
- **Fix Syntax Errors**: Suggest corrections for common syntax mistakes
- **Remove Unused Variables**: Clean up unused declarations
- **Fix Type Errors**: Suggest type corrections and conversions

#### Refactoring Actions
- **Extract Method**: Extract selected code into a new function
- **Rename Symbol**: Rename symbol across all references
- **Organize Imports**: Sort and clean up import statements
- **Convert Syntax**: Convert between different syntax forms

### 2. Semantic Tokens (textDocument/semanticTokens)

#### Token Classification
```cpp
enum class SemanticTokenType {
    KEYWORD,
    STRING,
    COMMENT,
    NUMBER,
    OPERATOR,
    FUNCTION,
    VARIABLE,
    CLASS,
    PARAMETER,
    PROPERTY,
    ENUM_MEMBER,
    DECORATOR
};

enum class SemanticTokenModifier {
    DECLARATION,
    DEFINITION,
    READONLY,
    STATIC,
    DEPRECATED,
    ABSTRACT,
    ASYNC,
    MODIFICATION
};
```

#### Token Generation Strategy
- **AST-Based Classification**: Use AST nodes for accurate classification
- **Incremental Updates**: Send delta updates for changed regions
- **Performance Optimization**: Cache token information per document
- **Multi-Line Tokens**: Handle tokens spanning multiple lines

### 3. Workspace Symbols (workspace/symbol)

#### Symbol Indexing
```cpp
class WorkspaceIndex {
private:
    std::unordered_map<std::string, std::vector<SymbolInformation>> symbol_index_;
    std::unordered_map<std::string, std::set<std::string>> file_dependencies_;
    
public:
    void indexDocument(const Document& document);
    void removeDocument(const std::string& uri);
    std::vector<SymbolInformation> searchSymbols(const std::string& query);
    void updateDependencies(const std::string& uri, const std::set<std::string>& deps);
};
```

#### Search Features
- **Fuzzy Search**: Flexible symbol name matching
- **Category Filtering**: Filter by symbol type (function, class, etc.)
- **Scope Filtering**: Search within specific modules or packages
- **Ranking**: Rank results by relevance and usage frequency

## Error Handling & Resilience

### Protocol Error Handling
- **Malformed Requests**: Return appropriate JSON-RPC error responses
- **Invalid Parameters**: Validate parameters and provide helpful error messages
- **Server Errors**: Log errors and return generic error responses to client
- **Timeout Handling**: Cancel long-running operations on client request

### Recovery Strategies
- **Partial Failures**: Continue processing other requests when one fails
- **State Corruption**: Detect and recover from corrupted internal state
- **Resource Exhaustion**: Implement resource limits and cleanup strategies
- **Client Disconnection**: Clean up resources when client disconnects

## Performance Considerations

### Response Time Targets
- **Completion**: < 200ms for 95% of requests
- **Hover**: < 100ms for 95% of requests
- **Definition**: < 150ms for 95% of requests
- **References**: < 500ms for 95% of requests (may be longer for large projects)
- **Diagnostics**: < 1000ms for document analysis

### Memory Management
- **Document Caching**: LRU cache with configurable size limits
- **AST Sharing**: Share AST nodes between documents when possible
- **Symbol Table Optimization**: Efficient symbol storage and lookup
- **Garbage Collection**: Periodic cleanup of unused resources

### Scalability Features
- **Lazy Loading**: Load and analyze files on-demand
- **Background Processing**: Use background threads for non-critical tasks
- **Incremental Analysis**: Only reanalyze changed code sections
- **Resource Monitoring**: Monitor and limit CPU and memory usage

## Configuration & Customization

### Server Configuration
```json
{
  "alif.server.maxCachedDocuments": 100,
  "alif.server.completionTimeout": 200,
  "alif.server.diagnosticsDelay": 250,
  "alif.server.maxWorkerThreads": 4,
  "alif.server.logLevel": "info"
}
```

### Language-Specific Settings
```json
{
  "alif.analysis.enableSemanticAnalysis": true,
  "alif.analysis.enableTypeInference": true,
  "alif.completion.enableSnippets": true,
  "alif.completion.maxSuggestions": 50,
  "alif.diagnostics.enableLinting": true,
  "alif.diagnostics.lintingRules": ["unused-variable", "undefined-symbol"]
}
```

This design provides a comprehensive foundation for implementing a world-class LSP server that delivers excellent developer experience while maintaining high performance and reliability.
