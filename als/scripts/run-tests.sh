#!/bin/bash
# Alif Language Server - Test Runner Script
# Usage: ./run-tests.sh [options]

set -e

# Default configuration
BUILD_TYPE="Debug"
VERBOSE=0
FILTER=""
PARALLEL=1
COVERAGE=0
VALGRIND=0
BUILD_DIR="build"

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

show_help() {
    echo "Alif Language Server - Test Runner"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -t, --type TYPE      Build type (Debug|Release, default: Debug)"
    echo "  -v, --verbose        Verbose test output"
    echo "  -f, --filter FILTER  Run only tests matching filter"
    echo "  -p, --parallel       Run tests in parallel"
    echo "  -c, --coverage       Generate code coverage report"
    echo "  -m, --valgrind       Run tests with valgrind (Linux only)"
    echo "  -h, --help           Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                           # Run all tests"
    echo "  $0 -v                        # Run with verbose output"
    echo "  $0 -f \"Lexer*\"               # Run only lexer tests"
    echo "  $0 -t Release -p             # Run release tests in parallel"
    echo "  $0 -c                        # Run with coverage"
    echo ""
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -f|--filter)
            FILTER="$2"
            shift 2
            ;;
        -p|--parallel)
            PARALLEL=1
            shift
            ;;
        -c|--coverage)
            COVERAGE=1
            BUILD_TYPE="Debug"  # Coverage requires debug build
            shift
            ;;
        -m|--valgrind)
            VALGRIND=1
            BUILD_TYPE="Debug"  # Valgrind requires debug build
            shift
            ;;
        -h|--help)
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

print_header "Alif Language Server - Test Runner"
echo "Build Type: $BUILD_TYPE"
echo "Verbose: $VERBOSE"
echo "Filter: ${FILTER:-'(all tests)'}"
echo "Parallel: $PARALLEL"
echo "Coverage: $COVERAGE"
echo "Valgrind: $VALGRIND"
echo ""

# Check if build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    print_error "Build directory not found: $BUILD_DIR"
    echo "Please run ./scripts/build.sh first"
    exit 1
fi

cd "$BUILD_DIR"

# Check if tests were built
if [[ ! -f "tests/als_tests" ]] && [[ ! -f "tests/${BUILD_TYPE}/als_tests.exe" ]]; then
    print_error "Test executable not found"
    echo "Please build with tests enabled: ./scripts/build.sh $BUILD_TYPE test"
    exit 1
fi

# Prepare test command
TEST_CMD="ctest"
TEST_ARGS=()

# Add build config for multi-config generators
TEST_ARGS+=("--build-config" "$BUILD_TYPE")

# Add verbose output if requested
if [[ $VERBOSE -eq 1 ]]; then
    TEST_ARGS+=("--verbose")
else
    TEST_ARGS+=("--output-on-failure")
fi

# Add filter if specified
if [[ -n "$FILTER" ]]; then
    TEST_ARGS+=("-R" "$FILTER")
fi

# Add parallel execution
if [[ $PARALLEL -eq 1 ]]; then
    JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    TEST_ARGS+=("--parallel" "$JOBS")
fi

# Setup coverage if requested
if [[ $COVERAGE -eq 1 ]]; then
    print_header "Setting up Code Coverage"
    
    # Check for gcov/lcov
    if ! command -v gcov &> /dev/null || ! command -v lcov &> /dev/null; then
        print_error "gcov and lcov are required for coverage analysis"
        echo "Please install: sudo apt-get install gcov lcov (Ubuntu/Debian)"
        exit 1
    fi
    
    # Reset coverage counters
    lcov --directory . --zerocounters
    print_success "Coverage counters reset"
fi

# Setup valgrind if requested
if [[ $VALGRIND -eq 1 ]]; then
    print_header "Setting up Valgrind"
    
    if ! command -v valgrind &> /dev/null; then
        print_error "valgrind not found"
        echo "Please install: sudo apt-get install valgrind (Ubuntu/Debian)"
        exit 1
    fi
    
    # Create valgrind suppressions file
    cat > valgrind.supp << 'EOF'
# Valgrind suppressions for ALS tests
{
   ignore_std_string_leaks
   Memcheck:Leak
   ...
   fun:*std*string*
}
EOF
    
    export CTEST_MEMORYCHECK_COMMAND=valgrind
    export CTEST_MEMORYCHECK_COMMAND_OPTIONS="--leak-check=full --show-leak-kinds=all --track-origins=yes --suppressions=valgrind.supp"
    TEST_CMD="ctest -T memcheck"
    print_success "Valgrind configured"
fi

# Run tests
print_header "Running Tests"
echo "Command: $TEST_CMD ${TEST_ARGS[*]}"
echo ""

if $TEST_CMD "${TEST_ARGS[@]}"; then
    print_success "All tests passed!"
    TEST_RESULT=0
else
    print_error "Some tests failed!"
    TEST_RESULT=1
fi

# Generate coverage report if requested
if [[ $COVERAGE -eq 1 ]]; then
    print_header "Generating Coverage Report"
    
    # Capture coverage data
    lcov --directory . --capture --output-file coverage.info
    
    # Remove system headers and test files from coverage
    lcov --remove coverage.info '/usr/*' '*/tests/*' '*/third_party/*' --output-file coverage_filtered.info
    
    # Generate HTML report
    mkdir -p coverage_html
    genhtml coverage_filtered.info --output-directory coverage_html
    
    print_success "Coverage report generated in: coverage_html/index.html"
    
    # Show coverage summary
    lcov --summary coverage_filtered.info
fi

# Show valgrind results if used
if [[ $VALGRIND -eq 1 ]]; then
    print_header "Valgrind Results"
    
    if [[ -f "Testing/Temporary/MemoryChecker.*.log" ]]; then
        echo "Memory check logs:"
        ls Testing/Temporary/MemoryChecker.*.log
        echo ""
        echo "To view detailed results:"
        echo "cat Testing/Temporary/MemoryChecker.*.log"
    fi
fi

cd ..

print_header "Test Summary"
if [[ $TEST_RESULT -eq 0 ]]; then
    print_success "All tests completed successfully!"
else
    print_error "Some tests failed. Check the output above for details."
fi

echo ""
echo "Additional commands:"
echo "  Run specific test: cd build && ./tests/als_tests --gtest_filter=\"TestName*\""
echo "  List all tests: cd build && ./tests/als_tests --gtest_list_tests"
if [[ $COVERAGE -eq 1 ]]; then
    echo "  View coverage: open build/coverage_html/index.html"
fi
echo ""

exit $TEST_RESULT
