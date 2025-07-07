# Alif Language Server (ALS)

A high-performance Language Server Protocol (LSP) implementation for the Alif programming language, built with modern C++23.

## 🚀 Features

- **Complete LSP Support**: Full implementation of Language Server Protocol v3.17
- **High Performance**: Asynchronous, multi-threaded architecture
- **Alif Language**: Native support for Arabic programming language constructs
- **Cross-Platform**: Windows, Linux, and macOS support
- **Modern C++**: Built with C++23 features and best practices

## 📋 Requirements

### Build Requirements
- **CMake**: 3.20 or higher
- **Compiler**: 
  - MSVC 2022+ (Windows)
  - GCC 11+ (Linux)
  - Clang 14+ (macOS)
- **C++ Standard**: C++23 (fallback to C++20)

### Runtime Requirements
- **Memory**: Minimum 512MB RAM
- **Disk Space**: 50MB for installation
- **OS**: Windows 10+, Ubuntu 20.04+, macOS 12+

## 🛠️ Building

### Quick Start

```bash
# Clone and build
git clone <repository-url>
cd als
./scripts/build.sh  # Linux/macOS
# or
scripts\build.bat   # Windows

# Run
./build/als
```

### Manual Build

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Install (optional)
cmake --install . --prefix /usr/local
```

### Build Options

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Enable testing
cmake .. -DALS_BUILD_TESTS=ON

# Use bundled dependencies
cmake .. -DALS_USE_BUNDLED_DEPS=ON

# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/als
```

## 🔧 Configuration

### Server Configuration

Create `als-config.json` in your workspace:

```json
{
  "server": {
    "maxCachedDocuments": 100,
    "completionTimeout": 200,
    "diagnosticsDelay": 250,
    "maxWorkerThreads": 4,
    "logLevel": "info"
  },
  "analysis": {
    "enableSemanticAnalysis": true,
    "enableTypeInference": true
  },
  "completion": {
    "enableSnippets": true,
    "maxSuggestions": 50
  }
}
```

### Editor Integration

#### Spectrum IDE
The ALS is designed to integrate seamlessly with Spectrum IDE, replacing the current `AlifLexer` and `AlifComplete` components.

#### VS Code
Install the Alif language extension from the marketplace.

#### Vim/Neovim
Use any LSP client plugin (e.g., `nvim-lspconfig`, `vim-lsp`).

## 📁 Project Structure

```
als/
├── src/                    # Source code
│   ├── core/              # Core server components
│   ├── features/          # LSP feature implementations
│   ├── analysis/          # Language analysis engine
│   └── workspace/         # Workspace management
├── include/als/           # Public headers
├── tests/                 # Test suite
├── scripts/               # Build and utility scripts
├── examples/              # Configuration examples
└── third_party/           # Bundled dependencies
```

## 🧪 Testing

```bash
# Build with tests
cmake .. -DALS_BUILD_TESTS=ON
cmake --build .

# Run tests
ctest --output-on-failure

# Run specific test
./tests/als_tests --gtest_filter="LexerTest.*"
```

## 📊 Performance

- **Startup Time**: < 100ms
- **Memory Usage**: ~50MB base + ~10MB per 1000 LOC
- **Completion Latency**: < 50ms for most cases
- **Large Files**: Supports files up to 10MB efficiently

## 🐛 Troubleshooting

### Common Issues

**Build fails with C++23 errors**
```bash
# Use C++20 fallback
cmake .. -DCMAKE_CXX_STANDARD=20
```

**Missing dependencies**
```bash
# Use bundled dependencies
cmake .. -DALS_USE_BUNDLED_DEPS=ON
```

**Server not starting**
```bash
# Check logs
./als --log-level=debug --log-file=als.log
```

### Debug Mode

```bash
# Build debug version
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Run with debugging
gdb ./als
# or
lldb ./als
```

## 📝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

See [Development Guide](../Documents/ALS/Development-Guide.md) for detailed contribution guidelines.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🔗 Links

- [Documentation](../Documents/ALS/)
- [Architecture](../Documents/ALS/Architecture.md)
- [Roadmap](../Documents/ALS/Roadmap.md)
- [API Reference](../Documents/ALS/API-Reference.md)
