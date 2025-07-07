#!/bin/bash
# Alif Language Server - Linux/macOS Build Script
# Usage: ./build.sh [Debug|Release] [clean] [test] [install] [help]

set -e  # Exit on any error

# Default configuration
BUILD_TYPE="Release"
CLEAN_BUILD=0
RUN_TESTS=0
INSTALL_ALS=0
BUILD_DIR="build"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
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

show_help() {
    echo "Alif Language Server - Build Script"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  Debug|Release    Build configuration (default: Release)"
    echo "  clean           Clean build directory before building"
    echo "  test            Build and run tests"
    echo "  install         Install after building"
    echo "  help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                    # Release build"
    echo "  $0 Debug             # Debug build"
    echo "  $0 Release clean     # Clean release build"
    echo "  $0 Debug test        # Debug build with tests"
    echo "  $0 Release install   # Release build and install"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        Debug|debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        Release|release)
            BUILD_TYPE="Release"
            shift
            ;;
        clean)
            CLEAN_BUILD=1
            shift
            ;;
        test)
            RUN_TESTS=1
            shift
            ;;
        install)
            INSTALL_ALS=1
            shift
            ;;
        help|--help|-h)
            show_help
            exit 0
            ;;
        *)
            print_error "Unknown argument: $1"
            show_help
            exit 1
            ;;
    esac
done

print_header "Alif Language Server - Build Script"
echo "Build Type: $BUILD_TYPE"
echo "Clean Build: $CLEAN_BUILD"
echo "Run Tests: $RUN_TESTS"
echo "Install: $INSTALL_ALS"
echo "Jobs: $JOBS"
echo ""

# Check for required tools
if ! command -v cmake &> /dev/null; then
    print_error "CMake not found in PATH"
    echo "Please install CMake 3.20 or higher"
    exit 1
fi

# Check CMake version
CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
print_success "CMake Version: $CMAKE_VERSION"

# Check compiler
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    print_success "Compiler: $GCC_VERSION"
elif command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -n1)
    print_success "Compiler: $CLANG_VERSION"
else
    print_error "No suitable C++ compiler found (g++ or clang++)"
    exit 1
fi

# Clean build if requested
if [[ $CLEAN_BUILD -eq 1 ]]; then
    print_warning "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure project
print_header "Configuring Project"
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DALS_BUILD_TESTS="$RUN_TESTS" \
    -DCMAKE_INSTALL_PREFIX="$(pwd)/install" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

print_success "Configuration completed"

# Build project
print_header "Building Project"
cmake --build . --config "$BUILD_TYPE" --parallel "$JOBS"

print_success "Build completed"

# Run tests if requested
if [[ $RUN_TESTS -eq 1 ]]; then
    print_header "Running Tests"
    if ctest --output-on-failure --build-config "$BUILD_TYPE"; then
        print_success "All tests passed"
    else
        print_warning "Some tests failed"
    fi
fi

# Install if requested
if [[ $INSTALL_ALS -eq 1 ]]; then
    print_header "Installing"
    cmake --install . --config "$BUILD_TYPE"
    print_success "Installation completed in: $(pwd)/install"
fi

cd ..

print_header "Build Summary"
print_success "Build completed successfully!"
echo ""
echo "Executable: $BUILD_DIR/als"
if [[ $INSTALL_ALS -eq 1 ]]; then
    echo "Installed to: $BUILD_DIR/install"
fi
echo ""
echo "Next steps:"
echo "  Run server: ./$BUILD_DIR/als"
if [[ $RUN_TESTS -eq 0 ]]; then
    echo "  Run tests:  ./scripts/build.sh $BUILD_TYPE test"
fi
echo "  Clean build: ./scripts/build.sh clean"
echo ""
