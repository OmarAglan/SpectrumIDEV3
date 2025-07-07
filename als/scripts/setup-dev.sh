#!/bin/bash
# Alif Language Server - Development Environment Setup Script
# This script sets up the development environment for ALS

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Detect OS
OS="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    OS="windows"
fi

print_header "Alif Language Server - Development Setup"
print_info "Detected OS: $OS"
echo ""

# Check system requirements
print_header "Checking System Requirements"

# Check CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)
    
    if [[ $CMAKE_MAJOR -gt 3 ]] || [[ $CMAKE_MAJOR -eq 3 && $CMAKE_MINOR -ge 20 ]]; then
        print_success "CMake $CMAKE_VERSION (✓ >= 3.20)"
    else
        print_error "CMake $CMAKE_VERSION (✗ < 3.20 required)"
        echo "Please upgrade CMake to version 3.20 or higher"
        exit 1
    fi
else
    print_error "CMake not found"
    echo "Please install CMake 3.20 or higher"
    exit 1
fi

# Check C++ compiler
COMPILER_FOUND=0
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ -dumpversion)
    GCC_MAJOR=$(echo $GCC_VERSION | cut -d'.' -f1)
    
    if [[ $GCC_MAJOR -ge 11 ]]; then
        print_success "GCC $GCC_VERSION (✓ >= 11)"
        COMPILER_FOUND=1
    else
        print_warning "GCC $GCC_VERSION (⚠ < 11, may not support C++23)"
    fi
fi

if command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+' | head -n1)
    CLANG_MAJOR=$(echo $CLANG_VERSION | cut -d'.' -f1)
    
    if [[ $CLANG_MAJOR -ge 14 ]]; then
        print_success "Clang $CLANG_VERSION (✓ >= 14)"
        COMPILER_FOUND=1
    else
        print_warning "Clang $CLANG_VERSION (⚠ < 14, may not support C++23)"
    fi
fi

if [[ $COMPILER_FOUND -eq 0 ]]; then
    print_error "No suitable C++ compiler found"
    echo "Please install GCC 11+ or Clang 14+"
    exit 1
fi

# Check Git
if command -v git &> /dev/null; then
    GIT_VERSION=$(git --version | cut -d' ' -f3)
    print_success "Git $GIT_VERSION"
else
    print_warning "Git not found (recommended for development)"
fi

# Check optional tools
print_header "Checking Optional Development Tools"

# Check clang-format
if command -v clang-format &> /dev/null; then
    CLANG_FORMAT_VERSION=$(clang-format --version | grep -o '[0-9]\+\.[0-9]\+' | head -n1)
    print_success "clang-format $CLANG_FORMAT_VERSION (code formatting)"
else
    print_warning "clang-format not found (recommended for code formatting)"
fi

# Check clang-tidy
if command -v clang-tidy &> /dev/null; then
    print_success "clang-tidy found (static analysis)"
else
    print_warning "clang-tidy not found (recommended for static analysis)"
fi

# Check cppcheck
if command -v cppcheck &> /dev/null; then
    print_success "cppcheck found (static analysis)"
else
    print_warning "cppcheck not found (optional static analysis)"
fi

# Check valgrind (Linux only)
if [[ "$OS" == "linux" ]]; then
    if command -v valgrind &> /dev/null; then
        print_success "valgrind found (memory debugging)"
    else
        print_warning "valgrind not found (recommended for memory debugging)"
    fi
fi

# Check ninja build system
if command -v ninja &> /dev/null; then
    print_success "ninja found (faster builds)"
else
    print_warning "ninja not found (optional, provides faster builds)"
fi

# Setup development environment
print_header "Setting Up Development Environment"

# Create compile_commands.json symlink for IDE support
if [[ -f "build/compile_commands.json" ]]; then
    if [[ ! -L "compile_commands.json" ]]; then
        ln -sf build/compile_commands.json compile_commands.json
        print_success "Created compile_commands.json symlink for IDE support"
    fi
else
    print_info "compile_commands.json will be created after first build"
fi

# Setup git hooks (if git is available and we're in a git repo)
if command -v git &> /dev/null && git rev-parse --git-dir > /dev/null 2>&1; then
    print_info "Setting up git hooks..."
    
    # Pre-commit hook for formatting
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash
# Pre-commit hook for code formatting

if command -v clang-format &> /dev/null; then
    echo "Running clang-format..."
    find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
    git add -u
fi
EOF
    chmod +x .git/hooks/pre-commit
    print_success "Created pre-commit hook for code formatting"
fi

# Create VS Code configuration
print_header "Creating IDE Configuration"

mkdir -p .vscode

# VS Code settings
cat > .vscode/settings.json << 'EOF'
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.cppStandard": "c++23",
    "C_Cpp.default.compilerPath": "/usr/bin/g++",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.generator": "Unix Makefiles",
    "files.associations": {
        "*.h": "cpp",
        "*.cpp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.clang_format_style": "file"
}
EOF

# VS Code tasks
cat > .vscode/tasks.json << 'EOF'
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Debug",
            "type": "shell",
            "command": "./scripts/build.sh",
            "args": ["Debug"],
            "group": "build",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        },
        {
            "label": "Build Release",
            "type": "shell",
            "command": "./scripts/build.sh",
            "args": ["Release"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        },
        {
            "label": "Run Tests",
            "type": "shell",
            "command": "./scripts/build.sh",
            "args": ["Debug", "test"],
            "group": "test",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        }
    ]
}
EOF

print_success "Created VS Code configuration"

print_header "Development Environment Setup Complete"
print_success "Your development environment is ready!"
echo ""
echo "Next steps:"
echo "  1. Build the project: ./scripts/build.sh"
echo "  2. Run tests: ./scripts/build.sh Debug test"
echo "  3. Open in VS Code: code ."
echo ""
echo "Development workflow:"
echo "  • Use ./scripts/build.sh for building"
echo "  • Code formatting is handled by clang-format"
echo "  • Git pre-commit hook will format code automatically"
echo "  • Use VS Code tasks (Ctrl+Shift+P -> Tasks: Run Task)"
echo ""
