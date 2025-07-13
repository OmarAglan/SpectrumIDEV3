# Alif Language Server - Project Structure

## Overview

This document outlines the detailed project structure for the Alif Language Server (ALS), including directory organization, file naming conventions, and module dependencies.

## Root Directory Structure

```
SpectrumIDEV3/
├── als/                          # Alif Language Server root
│   ├── src/                      # Source code
│   ├── include/                  # Public headers
│   ├── tests/                    # Test suite
│   ├── docs/                     # Additional documentation
│   ├── third_party/              # External dependencies
│   ├── scripts/                  # Build and utility scripts
│   ├── examples/                 # Example configurations and usage
│   ├── build/                    # Build output (git-ignored)
│   ├── CMakeLists.txt            # Main CMake configuration
│   ├── README.md                 # Project overview
│   └── .clang-format             # Code formatting rules
│
├── Documents/ALS/                # Project documentation
│   ├── README.md                 # Main documentation index
│   ├── Architecture.md           # System architecture
│   ├── Roadmap.md               # Development roadmap
│   ├── LSP-Server-Design.md     # LSP implementation details
│   ├── API-Reference.md         # API documentation
│   ├── Development-Guide.md     # Development setup
│   ├── Project-Structure.md     # This document
│   └── Technical-Specification.md # Technical requirements
│
└── Source/TextEditor/            # Current Spectrum IDE components
    ├── AlifLexer.h/cpp          # Current lexer (to be replaced)
    ├── AlifComplete.h/cpp       # Current completion (to be replaced)
    ├── SPEditor.h/cpp           # Editor component
    └── SPHighlighter.h/cpp      # Syntax highlighter
```

## Source Code Organization

### Core Server Components (`src/core/`)

```
src/core/
├── LspServer.h/cpp              # Main server orchestrator
├── JsonRpcProtocol.h/cpp        # JSON-RPC communication layer
├── RequestDispatcher.h/cpp      # Request routing and handling
├── ThreadPool.h/cpp             # Worker thread management
├── ServerConfig.h/cpp           # Configuration management
├── Logger.h/cpp                 # Logging infrastructure
└── Utils.h/cpp                  # Common utilities
```

**Dependencies:**
- `JsonRpcProtocol` → `nlohmann/json`, `spdlog`
- `ThreadPool` → `<thread>`, `<future>`, `<queue>`
- `LspServer` → All core components

### Workspace Management (`src/workspace/`)

```
src/workspace/
├── Workspace.h/cpp              # Project-wide state management
├── Document.h/cpp               # Individual file representation
├── WorkspaceIndex.h/cpp         # Symbol indexing and search
├── DependencyGraph.h/cpp        # File dependency tracking
├── FileWatcher.h/cpp            # File system monitoring
└── ProjectScanner.h/cpp         # Project structure discovery
```

**Dependencies:**
- `Workspace` → `Document`, `WorkspaceIndex`, `DependencyGraph`
- `FileWatcher` → Platform-specific file system APIs
- `ProjectScanner` → `<filesystem>`, `<regex>`

### Language Analysis Engine (`src/analysis/`)

```
src/analysis/
├── Lexer.h/cpp                  # Enhanced tokenization
├── Parser.h/cpp                 # Recursive descent parser
├── AST/                         # Abstract Syntax Tree
│   ├── ASTNode.h                # Base AST node
│   ├── Statements.h/cpp         # Statement nodes
│   ├── Expressions.h/cpp        # Expression nodes
│   ├── Declarations.h/cpp       # Declaration nodes
│   └── Visitors.h/cpp           # AST visitor patterns
├── SemanticAnalyzer.h/cpp       # Semantic analysis
├── SymbolTable.h/cpp            # Symbol resolution
├── TypeSystem.h/cpp             # Type inference and checking
└── Diagnostics.h/cpp            # Error and warning generation
```

**Dependencies:**
- `Parser` → `Lexer`, `AST/*`
- `SemanticAnalyzer` → `AST/*`, `SymbolTable`, `TypeSystem`
- `Diagnostics` → All analysis components

### LSP Feature Implementations (`src/features/`)

```
src/features/
├── CompletionProvider.h/cpp     # Code completion
├── HoverProvider.h/cpp          # Hover information
├── DefinitionProvider.h/cpp     # Go to definition
├── ReferencesProvider.h/cpp     # Find references
├── DocumentSymbolProvider.h/cpp # Document outline
├── WorkspaceSymbolProvider.h/cpp # Workspace symbol search
├── CodeActionProvider.h/cpp     # Quick fixes and refactoring
├── DiagnosticsProvider.h/cpp    # Error and warning reporting
├── FormattingProvider.h/cpp     # Code formatting
└── RenameProvider.h/cpp         # Symbol renaming
```

**Dependencies:**
- All providers → `workspace/*`, `analysis/*`
- `CompletionProvider` → `SymbolTable`, `TypeSystem`
- `CodeActionProvider` → `Diagnostics`, `AST/*`

### Platform Abstraction (`src/platform/`)

```
src/platform/
├── FileSystem.h/cpp             # Cross-platform file operations
├── Process.h/cpp                # Process management
├── Threading.h/cpp              # Threading utilities
└── Memory.h/cpp                 # Memory management helpers
```

**Dependencies:**
- Platform-specific system APIs
- Standard library threading and filesystem

## Header Organization

### Public Headers (`include/als/`)

```
include/als/
├── Server.h                     # Main server interface
├── Types.h                      # Common type definitions
├── Config.h                     # Configuration structures
└── Version.h                    # Version information
```

### Internal Headers
- All implementation headers remain in `src/` subdirectories
- Use forward declarations to minimize header dependencies
- Separate interface from implementation where possible

## Test Structure (`tests/`)

```
tests/
├── unit/                        # Unit tests
│   ├── core/                    # Core component tests
│   ├── workspace/               # Workspace tests
│   ├── analysis/                # Analysis engine tests
│   └── features/                # Feature provider tests
├── integration/                 # Integration tests
│   ├── lsp_protocol/            # LSP protocol tests
│   ├── workspace_management/    # Multi-file scenarios
│   └── performance/             # Performance benchmarks
├── fixtures/                    # Test data and fixtures
│   ├── alif_projects/           # Sample Alif projects
│   ├── lsp_messages/            # Sample LSP messages
│   └── expected_outputs/        # Expected test results
├── mocks/                       # Mock objects and stubs
└── CMakeLists.txt               # Test build configuration
```

### Test Naming Conventions
- Unit tests: `Test<ComponentName>.cpp`
- Integration tests: `Integration<FeatureName>.cpp`
- Performance tests: `Benchmark<ComponentName>.cpp`

## Build System Organization

### CMake Structure

```
CMakeLists.txt                   # Root CMake file
├── cmake/                       # CMake modules and utilities
│   ├── FindDependencies.cmake   # Dependency finding
│   ├── CompilerOptions.cmake    # Compiler configuration
│   ├── Testing.cmake            # Test configuration
│   └── Packaging.cmake          # Package generation
├── src/CMakeLists.txt           # Source build rules
└── tests/CMakeLists.txt         # Test build rules
```

### Build Targets
- `als` - Main executable
- `als_lib` - Core library (for testing)
- `als_tests` - Test executable
- `als_benchmarks` - Performance benchmarks

## Third-Party Dependencies

### Required Dependencies (`third_party/`)

```
third_party/
├── nlohmann_json/               # JSON parsing (header-only)
├── spdlog/                      # Logging library
├── fmt/                         # String formatting
└── README.md                    # Dependency documentation
```

### Optional Dependencies
- `catch2` - Testing framework (test-only)
- `benchmark` - Performance benchmarking (test-only)
- `clang-tidy` - Static analysis (development-only)

### Dependency Management
- Use vcpkg for cross-platform package management
- Fallback to system packages where available
- Git submodules for header-only libraries

## Configuration and Data Files

### Configuration Files

```
examples/
├── server_config.json           # Server configuration example
├── workspace_settings.json      # Workspace settings example
└── logging_config.json          # Logging configuration example
```

### Data Files

```
data/
├── alif_keywords.json           # Language keywords
├── builtin_functions.json       # Built-in function signatures
├── error_messages.json          # Localized error messages
└── completion_snippets.json     # Code completion snippets
```

## Documentation Structure

### Generated Documentation

```
docs/
├── api/                         # Generated API docs (Doxygen)
├── coverage/                    # Code coverage reports
├── benchmarks/                  # Performance benchmark results
└── diagrams/                    # Architecture diagrams
```

### Documentation Generation
- Use Doxygen for API documentation
- Generate coverage reports with gcov/lcov
- Create architecture diagrams with PlantUML or Mermaid

## Development Tools Configuration

### Code Quality Tools

```
.clang-format                    # Code formatting rules
.clang-tidy                      # Static analysis configuration
.gitignore                       # Git ignore patterns
.editorconfig                    # Editor configuration
```

### CI/CD Configuration

```
.github/
├── workflows/
│   ├── ci.yml                   # Continuous integration
│   ├── release.yml              # Release automation
│   └── docs.yml                 # Documentation generation
└── ISSUE_TEMPLATE/              # Issue templates
    ├── bug_report.md
    ├── feature_request.md
    └── performance_issue.md
```

## Module Dependencies

### Dependency Graph

```
main.cpp
    └── LspServer
        ├── JsonRpcProtocol
        ├── RequestDispatcher
        │   └── Feature Providers
        │       ├── CompletionProvider
        │       ├── HoverProvider
        │       └── ...
        ├── ThreadPool
        └── Workspace
            ├── Document
            ├── WorkspaceIndex
            └── Analysis Engine
                ├── Lexer
                ├── Parser
                │   └── AST
                ├── SemanticAnalyzer
                │   ├── SymbolTable
                │   └── TypeSystem
                └── Diagnostics
```

### Circular Dependency Prevention
- Use forward declarations in headers
- Implement dependency injection where needed
- Apply the dependency inversion principle
- Use interfaces to break tight coupling

## File Naming Conventions

### Source Files
- Headers: `PascalCase.h` (e.g., `LspServer.h`)
- Implementation: `PascalCase.cpp` (e.g., `LspServer.cpp`)
- Test files: `Test<ComponentName>.cpp`

### Directory Names
- Use `snake_case` for directories (e.g., `third_party`)
- Use descriptive names that reflect component purpose
- Keep directory names short but meaningful

### Configuration Files
- Use `snake_case.json` for JSON configuration files
- Use `kebab-case.yml` for YAML files
- Include file format in extension where ambiguous

This project structure provides a solid foundation for the Alif Language Server while maintaining clear separation of concerns, testability, and maintainability.
