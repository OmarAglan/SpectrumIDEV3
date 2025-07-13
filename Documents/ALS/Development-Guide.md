# Alif Language Server - Development Guide

## Getting Started

This guide provides comprehensive instructions for setting up the development environment, building the project, and contributing to the Alif Language Server (ALS).

## Prerequisites

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
- **Package Manager**: vcpkg (recommended) or Conan

### Optional Tools
- **IDE**: Visual Studio Code, CLion, or Visual Studio 2022
- **Debugger**: GDB, LLDB, or Visual Studio Debugger
- **Profiler**: Valgrind, Intel VTune, or Visual Studio Profiler
- **Static Analysis**: Clang-Tidy, PVS-Studio, or SonarQube

## Project Setup

### 1. Clone the Repository

```bash
# Clone the main repository
git clone https://github.com/Shad7ows/SpectrumIDEV3.git
cd SpectrumIDEV3

# Create ALS directory structure
mkdir -p als/src/{core,workspace,analysis,features}
mkdir -p als/{build,tests,docs,third_party}
```

### 2. Install Dependencies

#### Using vcpkg (Recommended)
```bash
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # Linux/macOS
# ./bootstrap-vcpkg.bat  # Windows

# Install required packages
./vcpkg install nlohmann-json spdlog fmt catch2 benchmark
```

#### Using System Package Manager
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libnlohmann-json3-dev libspdlog-dev libfmt-dev

# macOS (with Homebrew)
brew install cmake nlohmann-json spdlog fmt catch2

# Windows (with Chocolatey)
choco install cmake git visualstudio2022buildtools
```

### 3. Configure Build System

Create `als/CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.20)
project(AlifLanguageServer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find packages
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

# Add executable
add_executable(als
    src/main.cpp
    src/core/LspServer.cpp
    src/core/JsonRpcProtocol.cpp
    src/core/ThreadPool.cpp
    # Add more source files as they are created
)

# Link libraries
target_link_libraries(als PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    fmt::fmt
)

# Include directories
target_include_directories(als PRIVATE src)

# Compiler-specific options
if(MSVC)
    target_compile_options(als PRIVATE /W4 /WX)
else()
    target_compile_options(als PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Debug/Release configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(als PRIVATE DEBUG=1)
    target_compile_options(als PRIVATE -g -O0)
else()
    target_compile_definitions(als PRIVATE NDEBUG=1)
    target_compile_options(als PRIVATE -O3)
endif()
```

### 4. Build the Project

```bash
cd als
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Debug
# cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build . --config Debug

# Run
./als  # Linux/macOS
# ./Debug/als.exe  # Windows
```

## Development Workflow

### 1. Code Organization

Follow the established directory structure:
```
als/
├── src/
│   ├── core/           # Server core and communication
│   ├── workspace/      # Document and workspace management
│   ├── analysis/       # Language analysis engine
│   ├── features/       # LSP feature implementations
│   └── main.cpp        # Entry point
├── tests/              # Unit and integration tests
├── docs/               # Additional documentation
├── third_party/        # External dependencies
└── CMakeLists.txt      # Build configuration
```

### 2. Coding Standards

#### C++ Style Guidelines
- **Standard**: Follow Google C++ Style Guide with modifications
- **Naming**: 
  - Classes: `PascalCase` (e.g., `LspServer`)
  - Functions/Methods: `camelCase` (e.g., `processRequest`)
  - Variables: `snake_case` (e.g., `request_id`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)
- **Files**: 
  - Headers: `.h` extension
  - Implementation: `.cpp` extension
  - One class per file pair

#### Code Quality
- **Comments**: Use Doxygen-style comments for public APIs
- **Error Handling**: Use exceptions for exceptional cases, return codes for expected failures
- **Memory Management**: Prefer smart pointers over raw pointers
- **Thread Safety**: Document thread safety guarantees for all public APIs

#### Example Code Style
```cpp
/**
 * @brief Processes an LSP request and generates a response.
 * 
 * @param request The incoming LSP request
 * @param callback Function to call with the response
 * @throws std::invalid_argument if request is malformed
 */
void LspServer::processRequest(
    const JsonRpcRequest& request,
    ResponseCallback callback
) {
    try {
        auto response = dispatcher_->dispatch(request);
        callback(std::move(response));
    } catch (const std::exception& e) {
        logger_->error("Failed to process request: {}", e.what());
        callback(createErrorResponse(request.id, e.what()));
    }
}
```

### 3. Testing Strategy

#### Unit Testing
Use Catch2 framework for unit tests:
```cpp
#include <catch2/catch_test_macros.hpp>
#include "core/JsonRpcProtocol.h"

TEST_CASE("JsonRpcProtocol parses valid messages", "[protocol]") {
    JsonRpcProtocol protocol(std::cin, std::cout);
    
    SECTION("Simple request") {
        std::string json = R"({"jsonrpc":"2.0","id":1,"method":"test"})";
        auto message = protocol.parseMessage(json);
        
        REQUIRE(message.has_value());
        REQUIRE(message->isRequest());
        REQUIRE(message->asRequest().method == "test");
    }
}
```

#### Integration Testing
Test complete workflows:
```cpp
TEST_CASE("Complete completion workflow", "[integration]") {
    // Setup test workspace
    TestWorkspace workspace("test_project");
    workspace.addFile("main.alif", "دالة اختبار():\n    ");
    
    // Create server
    LspServer server(TestConfig{});
    server.initialize(workspace.getRoot());
    
    // Test completion
    auto completions = server.getCompletions("main.alif", Position{1, 4});
    REQUIRE(!completions.empty());
    REQUIRE(std::any_of(completions.begin(), completions.end(),
        [](const auto& item) { return item.label == "اطبع"; }));
}
```

### 4. Debugging and Profiling

#### Debugging Setup
```bash
# Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Run with debugger
gdb ./als
(gdb) set args < test_input.json
(gdb) run

# Or with VS Code
# Create .vscode/launch.json with appropriate configuration
```

#### Performance Profiling
```bash
# Build with profiling
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Profile with perf (Linux)
perf record -g ./als < large_test_input.json
perf report

# Profile with Instruments (macOS)
instruments -t "Time Profiler" ./als

# Profile with Visual Studio (Windows)
# Use built-in profiler in Visual Studio
```

### 5. Continuous Integration

#### GitHub Actions Workflow
Create `.github/workflows/ci.yml`:
```yaml
name: CI

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        build_type: [Debug, Release]
    
    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        # Platform-specific dependency installation
    
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
    
    - name: Build
      run: cmake --build build --config ${{ matrix.build_type }}
    
    - name: Test
      run: ctest --test-dir build --config ${{ matrix.build_type }}
```

## Contributing Guidelines

### 1. Development Process

#### Branch Strategy
- **main**: Stable, production-ready code
- **develop**: Integration branch for features
- **feature/**: Individual feature development
- **hotfix/**: Critical bug fixes

#### Workflow
1. Create feature branch from `develop`
2. Implement feature with tests
3. Ensure all tests pass and code coverage is maintained
4. Create pull request to `develop`
5. Code review and approval
6. Merge to `develop`
7. Regular merges from `develop` to `main`

### 2. Pull Request Guidelines

#### PR Template
```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Manual testing completed

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No new warnings introduced
```

#### Review Criteria
- **Functionality**: Does the code work as intended?
- **Design**: Is the code well-designed and maintainable?
- **Testing**: Are there adequate tests?
- **Performance**: Are there any performance implications?
- **Security**: Are there any security concerns?

### 3. Documentation Requirements

#### Code Documentation
- All public APIs must have Doxygen comments
- Complex algorithms should have explanatory comments
- README files for each major component

#### Architecture Documentation
- Update architecture diagrams for significant changes
- Document design decisions and trade-offs
- Maintain API reference documentation

## Troubleshooting

### Common Build Issues

#### CMake Configuration Errors
```bash
# Clear CMake cache
rm -rf build/
mkdir build && cd build

# Verbose configuration
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
```

#### Dependency Issues
```bash
# Check vcpkg integration
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Manual dependency paths
cmake .. -Dnlohmann_json_DIR=/path/to/json/cmake
```

#### Compiler Errors
```bash
# Check C++ standard support
cmake .. -DCMAKE_CXX_STANDARD=20  # Fallback to C++20 if needed

# Enable verbose compiler output
make VERBOSE=1  # Unix Makefiles
cmake --build . --verbose  # Other generators
```

### Runtime Issues

#### LSP Communication Problems
- Check stdin/stdout are not being used by other code
- Verify JSON-RPC message format
- Enable debug logging to trace message flow

#### Performance Issues
- Profile with appropriate tools for your platform
- Check for memory leaks with Valgrind or AddressSanitizer
- Monitor thread pool utilization

#### Memory Issues
```bash
# Build with AddressSanitizer
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address -g"

# Build with ThreadSanitizer
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
```

## Resources

### Documentation
- [Language Server Protocol Specification](https://microsoft.github.io/language-server-protocol/)
- [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification)
- [CMake Documentation](https://cmake.org/documentation/)

### Tools and Libraries
- [nlohmann/json](https://github.com/nlohmann/json) - JSON library
- [spdlog](https://github.com/gabime/spdlog) - Logging library
- [Catch2](https://github.com/catchorg/Catch2) - Testing framework
- [vcpkg](https://github.com/Microsoft/vcpkg) - Package manager

### Community
- [LSP Community](https://langserver.org/) - Language Server Protocol community
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) - C++ best practices

This development guide provides the foundation for contributing to the Alif Language Server project while maintaining high code quality and consistency.
