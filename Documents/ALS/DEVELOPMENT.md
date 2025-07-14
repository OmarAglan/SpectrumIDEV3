# ALS Project - Development Guide

**Last Updated**: July 14, 2025  
**Development Status**: Infrastructure Complete, Language Analysis Needed

## 🚀 Getting Started

This guide provides comprehensive instructions for setting up the development environment, building the project, and contributing to the Alif Language Server (ALS).

### Current Development Focus

#### ✅ **Ready for Development**
- **ALS Core Infrastructure**: Complete and production-ready
- **SpectrumIDE LSP Client**: Fully implemented and integrated
- **Build System**: CMake configuration with all dependencies
- **Testing Framework**: Comprehensive test suite with 100% pass rate

#### 🚧 **Primary Development Need**
- **Language Analysis Engine**: Implement Alif lexer, parser, and semantic analysis
- **LSP Feature Providers**: Add completion, hover, diagnostics, and definition providers
- **Integration Testing**: End-to-end testing with SpectrumIDE

## 📋 Prerequisites

### System Requirements
- **Operating System**: Windows 10+, Ubuntu 20.04+, macOS 11+
- **CPU**: x64 architecture (ARM64 support planned)
- **Memory**: Minimum 8GB RAM (16GB recommended for large projects)
- **Storage**: 2GB free space for development environment

### Required Tools
- **C++ Compiler**: 
  - GCC 11+ or Clang 14+ (Linux/macOS)
  - MSVC 2022 or Clang 14+ (Windows)
- **Build System**: CMake 3.20+
- **Version Control**: Git 2.30+

### Optional Tools
- **IDE**: Visual Studio Code, CLion, or Visual Studio 2022
- **Debugger**: GDB, LLDB, or Visual Studio Debugger
- **Profiler**: Valgrind, Intel VTune, or Visual Studio Profiler

## 🏗️ Project Setup

### 1. Clone and Build

```bash
# Navigate to project directory
cd D:/dev/SpectrumIDEV3/als

# Build the project
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
ctest --output-on-failure
```

### 2. Development Environment

```bash
# Set up development tools (optional)
cd D:/dev/SpectrumIDEV3/als
./scripts/setup-dev.sh  # Linux/macOS
./scripts/setup-dev.bat # Windows
```

## 📁 Project Structure & Development Status

### ALS Server (`als/`)

#### **Core Infrastructure** ✅ **COMPLETE - PRODUCTION READY**
```
als/src/core/
├── ✅ LspServer.cpp           - Main server orchestrator
├── ✅ JsonRpcProtocol.cpp     - JSON-RPC 2.0 implementation
├── ✅ RequestDispatcher.cpp   - Method routing with middleware
├── ✅ ThreadPool.cpp          - Task management with prioritization
├── ✅ ServerConfig.cpp        - Configuration management
└── ✅ Utils.cpp               - Utility functions (placeholder)
```

**Development Status**: Complete implementation, ready for language features

#### **Language Analysis** ⚠️ **NEEDS IMPLEMENTATION**
```
als/src/analysis/              ⚠️ EMPTY - Primary development focus
├── ⚠️ Lexer.h/cpp             - Port from Source/TextEditor/AlifLexer.cpp
├── ⚠️ Parser.h/cpp            - Recursive descent parser for Alif
├── ⚠️ AST/                    - Abstract Syntax Tree nodes
├── ⚠️ SemanticAnalyzer.h/cpp  - Symbol resolution and type checking
├── ⚠️ SymbolTable.h/cpp       - Cross-file symbol management
└── ⚠️ Diagnostics.h/cpp       - Error and warning generation
```

**Development Priority**: High - Critical for functional language server

#### **LSP Features** ⚠️ **NEEDS IMPLEMENTATION**
```
als/src/features/              ⚠️ EMPTY - Depends on language analysis
├── ⚠️ CompletionProvider.cpp  - Replace AlifComplete functionality
├── ⚠️ HoverProvider.cpp       - Symbol information display
├── ⚠️ DiagnosticsProvider.cpp - Real-time error reporting
├── ⚠️ DefinitionProvider.cpp  - Go-to-definition navigation
└── ⚠️ ReferencesProvider.cpp  - Find symbol references
```

**Development Priority**: High - Required for LSP functionality

#### **Workspace Management** ⚠️ **NEEDS IMPLEMENTATION**
```
als/src/workspace/             ⚠️ EMPTY - Medium priority
├── ⚠️ Workspace.cpp           - Project-wide state management
├── ⚠️ Document.cpp            - File representation and tracking
└── ⚠️ WorkspaceIndex.cpp      - Symbol indexing across files
```

**Development Priority**: Medium - Required for multi-file projects

### SpectrumIDE Integration (`Source/`)

#### **LSP Client** ✅ **COMPLETE - PRODUCTION READY**
```
Source/LspClient/
├── ✅ SpectrumLspClient.h/cpp - Main orchestrator (singleton)
├── ✅ LspProcess.h/cpp        - ALS server process management
├── ✅ LspProtocol.h/cpp       - JSON-RPC communication
├── ✅ LspFeatureManager.h/cpp - Feature coordination
├── ✅ DocumentManager.h/cpp   - Document synchronization
└── ✅ ErrorManager.h/cpp      - Error handling & graceful degradation
```

**Development Status**: Complete, ready for ALS server language features

#### **Legacy Components** ⚠️ **TO BE REPLACED**
```
Source/TextEditor/
├── ⚠️ AlifLexer.h/cpp         - Legacy lexer (source for ALS port)
├── ⚠️ AlifComplete.h/cpp      - Legacy completion (to be replaced)
├── ✅ SPEditor.h/cpp          - Editor (integrated with LSP client)
└── ✅ SPHighlighter.h/cpp     - Syntax highlighter (ready for LSP)
```

## 🔧 Build System

### CMake Configuration
```cmake
# Main targets
als                 # Main ALS server executable
als_tests          # Test suite executable

# Build options
-DALS_BUILD_TESTS=ON     # Enable test building (default: ON)
-DCMAKE_BUILD_TYPE=Debug # Debug/Release/RelWithDebInfo
```

### Dependencies
- **nlohmann/json**: JSON parsing and serialization
- **Custom Logging**: Production logging system (als/src/logging/)
- **Catch2**: Testing framework (tests only)

## 🧪 Testing

### Test Structure
```
als/tests/
├── ✅ test_main.cpp              - Test runner
├── ✅ test_jsonrpc_protocol.cpp  - JSON-RPC protocol tests
├── ✅ test_threadpool.cpp        - Threading and concurrency tests
├── ✅ test_request_dispatcher.cpp - Request routing tests
├── ✅ test_lsp_messages.cpp      - LSP message handling tests
└── ✅ test_protocol_compliance.cpp - Protocol compliance tests
```

### Running Tests
```bash
# Build and run all tests
cd als/build
ctest --output-on-failure

# Run specific test
./tests/als_tests --gtest_filter="ThreadPoolTest.*"

# Test with verbose output
ctest -V
```

### Test Results (Current)
```
6/6 tests passed (100% pass rate)
- BasicTestRunner: ✅ Passed
- JsonRpcProtocolTests: ✅ Passed  
- LspMessageTests: ✅ Passed
- ProtocolComplianceTests: ✅ Passed
- ThreadPoolTests: ✅ Passed
- RequestDispatcherTests: ✅ Passed
```

## 🔌 API Reference

### Core Server APIs ✅ **IMPLEMENTED**

#### LspServer Class
```cpp
class LspServer {
public:
    explicit LspServer(const ServerConfig& config);
    ~LspServer();
    
    // Main server operations
    void run();                          // Start the main event loop
    void shutdown();                     // Graceful shutdown
    void exit(int exit_code = 0);       // Force exit
    
    // Configuration and monitoring
    void updateConfig(const ServerConfig& config);
    const ServerConfig& getConfig() const;
    ServerStats getStats() const;
};
```

#### JsonRpcProtocol Class
```cpp
class JsonRpcProtocol {
public:
    JsonRpcProtocol();
    ~JsonRpcProtocol();
    
    // Message handling
    std::optional<JsonRpcMessage> readMessage();
    void writeResponse(const JsonRpcResponse& response);
    void writeNotification(const JsonRpcNotification& notification);
    void writeError(const JsonRpcError& error);
    
    // Protocol state
    bool isConnected() const;
    void disconnect();
};
```

#### ThreadPool Class
```cpp
class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count = 4);
    ~ThreadPool();
    
    // Task submission
    template<typename F, typename... Args>
    auto submit(TaskPriority priority, F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    // Pool management
    void resize(size_t new_size);
    void shutdown();
    ThreadPoolStats getStats() const;
};
```

### SpectrumIDE LSP Client APIs ✅ **IMPLEMENTED**

#### SpectrumLspClient Class
```cpp
class SpectrumLspClient : public QObject {
public:
    // Singleton access
    static SpectrumLspClient* instance();
    
    // Server lifecycle
    bool initialize(const QString& alsServerPath, const QString& workspaceRoot);
    bool start();
    void stop();
    
    // LSP feature requests
    QFuture<CompletionResult> requestCompletion(const QString& uri, int line, int character);
    QFuture<HoverResult> requestHover(const QString& uri, int line, int character);
    QFuture<DefinitionResult> requestDefinition(const QString& uri, int line, int character);
    
    // Document synchronization
    void didOpenDocument(const QString& uri, const QString& content);
    void didChangeDocument(const QString& uri, const QString& content);
    void didCloseDocument(const QString& uri);
    
    // Configuration and monitoring
    bool isFeatureEnabled(const QString& feature) const;
    void setFeatureEnabled(const QString& feature, bool enabled);
    ConnectionState getConnectionState() const;
};
```

### Planned APIs ⚠️ **NOT YET IMPLEMENTED**

#### Lexer Class (Planned)
```cpp
class Lexer {
public:
    explicit Lexer(const std::string& source);
    
    // Tokenization
    std::vector<Token> tokenize();
    Token nextToken();
    bool hasMoreTokens() const;
    
    // Error handling
    std::vector<LexerError> getErrors() const;
    void reset(const std::string& new_source);
};
```

#### CompletionProvider Class (Planned)
```cpp
class CompletionProvider {
public:
    CompletionProvider(const SymbolTable& symbols, const TypeSystem& types);
    
    // LSP completion
    CompletionList getCompletions(const Document& doc, Position pos);
    CompletionItem resolveCompletion(const CompletionItem& item);
    
    // Context analysis
    CompletionContext analyzeContext(const Document& doc, Position pos);
};
```

## 🎯 Development Priorities

### **1. Implement Language Analysis Engine** (High Priority)
**Objective**: Create functional Alif language analysis

**Tasks**:
- Port `AlifLexer.cpp` logic to `als/src/analysis/Lexer.cpp`
- Implement recursive descent parser for Alif syntax
- Build AST representation with visitor pattern
- Add semantic analysis with symbol resolution

**Estimated Effort**: 40-50 hours

### **2. Implement LSP Feature Providers** (High Priority)
**Objective**: Add LSP language features

**Tasks**:
- Create `CompletionProvider` to replace `AlifComplete`
- Implement `HoverProvider` for symbol information
- Add `DiagnosticsProvider` for error reporting
- Create `DefinitionProvider` for navigation

**Estimated Effort**: 20-30 hours

### **3. Integration Testing** (Medium Priority)
**Objective**: Validate end-to-end functionality

**Tasks**:
- Test ALS ↔ SpectrumIDE communication
- Validate document synchronization
- Test LSP feature requests and responses
- Verify error handling and recovery

**Estimated Effort**: 10-15 hours

## 🔄 Development Workflow

### 1. **Language Analysis Development**
```bash
# Focus area: als/src/analysis/
# Start with: Lexer.cpp (port from AlifLexer.cpp)
# Then: Parser.cpp, AST/, SemanticAnalyzer.cpp
```

### 2. **Feature Provider Development**
```bash
# Focus area: als/src/features/
# Start with: CompletionProvider.cpp
# Then: HoverProvider.cpp, DiagnosticsProvider.cpp
```

### 3. **Integration and Testing**
```bash
# Test with SpectrumIDE
# Validate LSP communication
# Performance testing and optimization
```

## 📊 Code Quality Standards

### **Testing Requirements**
- Unit tests for all new components
- Integration tests for LSP features
- Performance benchmarks for critical paths
- Test coverage >90% for core functionality

### **Code Standards**
- Modern C++23 features and best practices
- RAII principles for resource management
- Comprehensive error handling
- Thread-safe design for concurrent access

### **Documentation Requirements**
- API documentation for public interfaces
- Implementation comments for complex logic
- Usage examples for key features
- Architecture decision records

The development environment is ready, and the foundation is solid. The primary focus should be implementing the language analysis engine to unlock the full potential of the ALS architecture.
