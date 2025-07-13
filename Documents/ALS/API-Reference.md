# Alif Language Server - API Reference

## Overview

This document provides comprehensive API reference for the internal components of the Alif Language Server (ALS). It covers the main classes, interfaces, and data structures used throughout the system.

## Core Server Components

### LspServer Class

The main server orchestrator that manages the entire LSP server lifecycle.

```cpp
class LspServer {
public:
    // Constructor & Destructor
    explicit LspServer(const ServerConfig& config);
    ~LspServer();

    // Main server operations
    void run();                          // Start the main event loop
    void shutdown();                     // Graceful shutdown
    void exit(int exit_code = 0);       // Force exit

    // Configuration
    void updateConfig(const ServerConfig& config);
    const ServerConfig& getConfig() const;

    // Statistics and monitoring
    ServerStats getStats() const;
    void resetStats();

private:
    std::unique_ptr<JsonRpcProtocol> protocol_;
    std::unique_ptr<RequestDispatcher> dispatcher_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<Workspace> workspace_;
    ServerConfig config_;
    ServerStats stats_;
    std::atomic<bool> running_;
};
```

### JsonRpcProtocol Class

Handles JSON-RPC 2.0 protocol communication over stdio.

```cpp
class JsonRpcProtocol {
public:
    // Message handling
    std::optional<JsonRpcMessage> readMessage();
    void writeMessage(const JsonRpcResponse& response);
    void writeNotification(const JsonRpcNotification& notification);

    // Error handling
    void writeError(const JsonRpcError& error);
    void writeParseError(const std::string& message);

    // Protocol state
    bool isConnected() const;
    void disconnect();

private:
    std::istream& input_stream_;
    std::ostream& output_stream_;
    std::mutex write_mutex_;
    
    std::optional<std::string> readContentLengthHeader();
    std::string readJsonPayload(size_t content_length);
    void writeRawMessage(const std::string& json_content);
};
```

### RequestDispatcher Class ✅ **IMPLEMENTED**

Routes LSP requests to appropriate handlers and manages request lifecycle.

```cpp
class RequestDispatcher {
public:
    using RequestHandler = std::function<void(const RequestContext& context)>;
    using NotificationHandler = std::function<void(const JsonRpcNotification&)>;

    // Constructor
    RequestDispatcher(JsonRpcProtocol& protocol, ThreadPool& thread_pool);

    // Handler registration
    void registerRequestHandler(const std::string& method, RequestHandler handler);
    void registerNotificationHandler(const std::string& method, NotificationHandler handler);

    // Request processing
    void dispatch(const JsonRpcMessage& message);
    void cancelRequest(const JsonRpcId& request_id);
    void cancelAllRequests();

    // Middleware support
    void addMiddleware(std::unique_ptr<RequestMiddleware> middleware);

    // Statistics and monitoring
    DispatcherStats getStats() const;
    void resetStats();
    bool hasRequestHandler(const std::string& method) const;
    bool hasNotificationHandler(const std::string& method) const;

private:
    JsonRpcProtocol& protocol_;
    ThreadPool& thread_pool_;
    std::unordered_map<std::string, RequestHandler> request_handlers_;
    std::unordered_map<std::string, NotificationHandler> notification_handlers_;
    std::vector<std::unique_ptr<RequestMiddleware>> middleware_;
    std::unordered_map<JsonRpcId, std::shared_ptr<std::atomic<bool>>> active_requests_;
};
```

## Threading & Task Management

### ThreadPool Class ✅ **IMPLEMENTED**

Manages worker threads and task execution with prioritization and cancellation support.

```cpp
class ThreadPool {
public:
    enum class TaskPriority {
        LOW = 0,      // Background tasks (indexing, cleanup)
        NORMAL = 1,   // Regular LSP requests
        HIGH = 2,     // User-interactive requests (completion, hover)
        URGENT = 3    // Critical requests (shutdown, cancellation)
    };

    struct TaskStats {
        size_t submitted = 0;
        size_t completed = 0;
        size_t cancelled = 0;
        size_t failed = 0;
        std::chrono::milliseconds total_execution_time{0};
        std::chrono::milliseconds average_execution_time{0};
    };

    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency(),
                       size_t max_queue_size = 1000);
    ~ThreadPool();

    // Task submission
    template<typename F, typename... Args>
    auto submit(TaskPriority priority, F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>;

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>;

    template<typename F, typename... Args>
    auto submitCancellable(TaskPriority priority,
                          std::shared_ptr<std::atomic<bool>> cancellation_token,
                          F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>>;

    // Cancellation support
    std::shared_ptr<std::atomic<bool>> createCancellationToken();
    void cancelAllTasks();

    // Pool management
    bool waitForCompletion(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    void resize(size_t num_threads);

    // Status and monitoring
    size_t size() const;
    size_t activeThreads() const;
    size_t queuedTasks() const;
    TaskStats getStats() const;
    void resetStats();
    auto submit(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // Cancellation support
    std::shared_ptr<std::atomic<bool>> createCancellationToken();
    void cancelAllTasks();

    // Pool management
    void resize(size_t num_threads);
    size_t size() const;
    size_t activeThreads() const;
    size_t queuedTasks() const;

private:
    struct Task {
        std::function<void()> function;
        TaskPriority priority;
        std::chrono::steady_clock::time_point submit_time;
        std::shared_ptr<std::atomic<bool>> cancellation_token;
    };

    std::vector<std::thread> workers_;
    std::priority_queue<Task> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};
```

## Workspace & Document Management

### Workspace Class

Manages project-wide state, documents, and cross-file analysis.

```cpp
class Workspace {
public:
    // Initialization
    void initialize(const WorkspaceFolder& root_folder);
    void addWorkspaceFolder(const WorkspaceFolder& folder);
    void removeWorkspaceFolder(const std::string& uri);

    // Document management
    std::shared_ptr<Document> openDocument(const std::string& uri, const std::string& content);
    void updateDocument(const std::string& uri, const std::string& content, int version);
    void closeDocument(const std::string& uri);
    std::shared_ptr<Document> getDocument(const std::string& uri) const;

    // Project analysis
    void indexWorkspace();
    void reindexDocument(const std::string& uri);
    std::vector<SymbolInformation> findSymbols(const std::string& query) const;

    // Dependency management
    std::vector<std::string> getDependencies(const std::string& uri) const;
    std::vector<std::string> getDependents(const std::string& uri) const;

    // Configuration
    void updateConfiguration(const WorkspaceConfiguration& config);

private:
    std::vector<WorkspaceFolder> workspace_folders_;
    std::unordered_map<std::string, std::shared_ptr<Document>> documents_;
    std::unique_ptr<WorkspaceIndex> index_;
    std::unique_ptr<DependencyGraph> dependency_graph_;
    WorkspaceConfiguration config_;
    mutable std::shared_mutex documents_mutex_;
};
```

### Document Class

Represents a single source file with its content, AST, and analysis results.

```cpp
class Document {
public:
    // Construction
    Document(const std::string& uri, const std::string& content, int version);

    // Content management
    void updateContent(const std::string& content, int version);
    void applyIncrementalChanges(const std::vector<TextDocumentContentChangeEvent>& changes);
    
    // Analysis state
    void setAST(std::shared_ptr<AST> ast);
    void setSymbolTable(std::shared_ptr<SymbolTable> symbols);
    void setDiagnostics(std::vector<Diagnostic> diagnostics);

    // Accessors
    const std::string& getUri() const { return uri_; }
    const std::string& getContent() const { return content_; }
    int getVersion() const { return version_; }
    std::shared_ptr<AST> getAST() const { return ast_; }
    std::shared_ptr<SymbolTable> getSymbolTable() const { return symbols_; }
    const std::vector<Diagnostic>& getDiagnostics() const { return diagnostics_; }

    // State queries
    bool needsReparse() const;
    bool isAnalyzed() const;
    std::chrono::steady_clock::time_point getLastModified() const;

private:
    std::string uri_;
    std::string content_;
    int version_;
    std::shared_ptr<AST> ast_;
    std::shared_ptr<SymbolTable> symbols_;
    std::vector<Diagnostic> diagnostics_;
    std::chrono::steady_clock::time_point last_modified_;
    mutable std::shared_mutex content_mutex_;
};
```

## Language Analysis Components

### Lexer Class

Enhanced version of the existing AlifLexer with error recovery and position tracking.

```cpp
class Lexer {
public:
    // Tokenization
    std::vector<Token> tokenize(const std::string& text);
    std::vector<Token> tokenizeIncremental(
        const std::string& text,
        const std::vector<Token>& previous_tokens,
        const TextRange& changed_range
    );

    // Error handling
    std::vector<LexicalError> getErrors() const;
    void clearErrors();

    // Configuration
    void setErrorRecoveryEnabled(bool enabled);
    void setPositionTrackingEnabled(bool enabled);

private:
    std::vector<Token> tokens_;
    std::vector<LexicalError> errors_;
    size_t position_;
    int line_;
    int column_;
    bool error_recovery_enabled_;
    bool position_tracking_enabled_;

    // Tokenization methods
    Token scanNumber();
    Token scanString();
    Token scanIdentifier();
    Token scanOperator();
    Token scanComment();
    
    // Error recovery
    void recoverFromError();
    void skipToNextToken();
};
```

### Parser Class

Recursive descent parser with comprehensive error recovery for Alif syntax.

```cpp
class Parser {
public:
    // Parsing
    std::shared_ptr<AST> parse(const std::vector<Token>& tokens);
    std::shared_ptr<AST> parseIncremental(
        const std::vector<Token>& tokens,
        std::shared_ptr<AST> previous_ast,
        const TextRange& changed_range
    );

    // Error handling
    std::vector<SyntaxError> getErrors() const;
    void clearErrors();

    // Configuration
    void setErrorRecoveryEnabled(bool enabled);
    void setMaxErrorCount(size_t max_errors);

private:
    std::vector<Token> tokens_;
    size_t current_token_;
    std::vector<SyntaxError> errors_;
    bool error_recovery_enabled_;
    size_t max_error_count_;

    // Parsing methods
    std::shared_ptr<ASTNode> parseStatement();
    std::shared_ptr<ASTNode> parseExpression();
    std::shared_ptr<ASTNode> parseFunction();
    std::shared_ptr<ASTNode> parseClass();
    std::shared_ptr<ASTNode> parseImport();

    // Error recovery
    void reportError(const std::string& message);
    void synchronize();
    bool isAtSynchronizationPoint();
};
```

### AST Node Hierarchy

Base classes and specific node types for the Abstract Syntax Tree.

```cpp
// Base AST node
class ASTNode {
public:
    enum class NodeType {
        PROGRAM, STATEMENT, EXPRESSION, DECLARATION,
        FUNCTION_DEF, CLASS_DEF, VARIABLE_DEF,
        IF_STMT, WHILE_STMT, FOR_STMT, RETURN_STMT,
        BINARY_EXPR, UNARY_EXPR, CALL_EXPR, MEMBER_EXPR,
        IDENTIFIER, LITERAL
    };

    virtual ~ASTNode() = default;
    virtual NodeType getType() const = 0;
    virtual void accept(ASTVisitor& visitor) = 0;

    // Position information
    const SourceRange& getSourceRange() const { return source_range_; }
    void setSourceRange(const SourceRange& range) { source_range_ = range; }

    // Parent/child relationships
    ASTNode* getParent() const { return parent_; }
    void setParent(ASTNode* parent) { parent_ = parent; }

protected:
    SourceRange source_range_;
    ASTNode* parent_ = nullptr;
};

// Specific node types
class FunctionDefNode : public ASTNode {
public:
    NodeType getType() const override { return NodeType::FUNCTION_DEF; }
    void accept(ASTVisitor& visitor) override;

    const std::string& getName() const { return name_; }
    const std::vector<Parameter>& getParameters() const { return parameters_; }
    std::shared_ptr<ASTNode> getBody() const { return body_; }

private:
    std::string name_;
    std::vector<Parameter> parameters_;
    std::shared_ptr<ASTNode> body_;
};
```

### SemanticAnalyzer Class

Performs semantic analysis and builds symbol tables.

```cpp
class SemanticAnalyzer {
public:
    // Analysis
    std::shared_ptr<SymbolTable> analyze(std::shared_ptr<AST> ast);
    std::vector<SemanticError> getErrors() const;
    void clearErrors();

    // Configuration
    void setTypeInferenceEnabled(bool enabled);
    void setStrictModeEnabled(bool enabled);

private:
    std::shared_ptr<SymbolTable> symbol_table_;
    std::vector<SemanticError> errors_;
    std::stack<Scope*> scope_stack_;
    bool type_inference_enabled_;
    bool strict_mode_enabled_;

    // Analysis methods
    void analyzeNode(std::shared_ptr<ASTNode> node);
    void analyzeFunctionDef(std::shared_ptr<FunctionDefNode> node);
    void analyzeClassDef(std::shared_ptr<ClassDefNode> node);
    void analyzeVariableDef(std::shared_ptr<VariableDefNode> node);

    // Symbol resolution
    Symbol* resolveSymbol(const std::string& name);
    void declareSymbol(const std::string& name, const Symbol& symbol);
    
    // Type inference
    Type inferType(std::shared_ptr<ASTNode> node);
    bool isTypeCompatible(const Type& expected, const Type& actual);
};
```

## LSP Feature Implementations

### CompletionProvider Class

Provides intelligent code completion suggestions.

```cpp
class CompletionProvider {
public:
    // Completion generation
    CompletionList getCompletions(
        std::shared_ptr<Document> document,
        const Position& position,
        const CompletionContext& context
    );

    // Configuration
    void setMaxSuggestions(size_t max_suggestions);
    void setSnippetsEnabled(bool enabled);
    void setFuzzyMatchingEnabled(bool enabled);

private:
    size_t max_suggestions_;
    bool snippets_enabled_;
    bool fuzzy_matching_enabled_;

    // Completion providers
    std::vector<CompletionItem> getKeywordCompletions(const CompletionContext& context);
    std::vector<CompletionItem> getSymbolCompletions(const CompletionContext& context);
    std::vector<CompletionItem> getMemberCompletions(const CompletionContext& context);
    std::vector<CompletionItem> getSnippetCompletions(const CompletionContext& context);

    // Filtering and ranking
    std::vector<CompletionItem> filterCompletions(
        const std::vector<CompletionItem>& items,
        const std::string& prefix
    );
    void rankCompletions(std::vector<CompletionItem>& items);
};
```

### HoverProvider Class

Provides hover information for symbols and expressions.

```cpp
class HoverProvider {
public:
    // Hover information
    std::optional<Hover> getHover(
        std::shared_ptr<Document> document,
        const Position& position
    );

private:
    // Information extraction
    std::optional<Symbol> getSymbolAtPosition(
        std::shared_ptr<Document> document,
        const Position& position
    );
    
    std::string formatSymbolInfo(const Symbol& symbol);
    std::string formatTypeInfo(const Type& type);
    std::string formatDocumentation(const Symbol& symbol);
};
```

## Data Structures

### Core LSP Types

```cpp
// Position and range types
struct Position {
    int line;
    int character;
};

struct Range {
    Position start;
    Position end;
};

struct Location {
    std::string uri;
    Range range;
};

// Diagnostic types
enum class DiagnosticSeverity {
    ERROR = 1,
    WARNING = 2,
    INFORMATION = 3,
    HINT = 4
};

struct Diagnostic {
    Range range;
    DiagnosticSeverity severity;
    std::string code;
    std::string message;
    std::string source;
    std::vector<DiagnosticRelatedInformation> related_information;
};

// Symbol types
enum class SymbolKind {
    FILE = 1, MODULE = 2, NAMESPACE = 3, PACKAGE = 4,
    CLASS = 5, METHOD = 6, PROPERTY = 7, FIELD = 8,
    CONSTRUCTOR = 9, ENUM = 10, INTERFACE = 11,
    FUNCTION = 12, VARIABLE = 13, CONSTANT = 14,
    STRING = 15, NUMBER = 16, BOOLEAN = 17, ARRAY = 18
};

struct SymbolInformation {
    std::string name;
    SymbolKind kind;
    Location location;
    std::string container_name;
};
```

This API reference provides the foundation for implementing and extending the Alif Language Server with consistent interfaces and clear separation of concerns.
