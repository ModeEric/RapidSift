#!/bin/bash

# RapidSift Build Script
# Ultra-fast text deduplication in C++

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
CLEAN_BUILD=false
RUN_TESTS=false
INSTALL=false
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")
ENABLE_OPENMP=ON

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show help
show_help() {
    echo "RapidSift Build Script"
    echo "====================="
    echo
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -c, --clean         Clean build directory before building"
    echo "  -d, --debug         Build in debug mode (default: release)"
    echo "  -t, --test          Run tests after building"
    echo "  -i, --install       Install after building"
    echo "  -j, --jobs N        Number of parallel jobs (default: $PARALLEL_JOBS)"
    echo "  --no-openmp         Disable OpenMP support"
    echo "  --benchmark         Run performance benchmarks"
    echo
    echo "Examples:"
    echo "  $0                  # Basic release build"
    echo "  $0 -c -t           # Clean build and run tests"
    echo "  $0 -d -t           # Debug build with tests"
    echo "  $0 --benchmark     # Release build with benchmarks"
    echo
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        --no-openmp)
            ENABLE_OPENMP=OFF
            shift
            ;;
        --benchmark)
            RUN_TESTS=true
            BENCHMARK=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Print build configuration
print_status "RapidSift Build Configuration"
echo "=============================="
echo "Build Type:      $BUILD_TYPE"
echo "Clean Build:     $CLEAN_BUILD"
echo "Run Tests:       $RUN_TESTS"
echo "Install:         $INSTALL"
echo "Parallel Jobs:   $PARALLEL_JOBS"
echo "OpenMP:          $ENABLE_OPENMP"
echo ""

# Create build directory
BUILD_DIR="build"

if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    print_success "Build directory cleaned"
fi

if [ ! -d "$BUILD_DIR" ]; then
    print_status "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Check for required tools
print_status "Checking build requirements..."

if ! command -v cmake &> /dev/null; then
    print_error "CMake is required but not installed"
    exit 1
fi

if ! command -v make &> /dev/null && ! command -v ninja &> /dev/null; then
    print_error "Make or Ninja is required but not installed"
    exit 1
fi

print_success "Build requirements satisfied"

# Configure with CMake
print_status "Configuring build with CMake..."
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DUSE_OPENMP="$ENABLE_OPENMP" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..

if [ $? -ne 0 ]; then
    print_error "CMake configuration failed"
    exit 1
fi

print_success "CMake configuration completed"

# Build
print_status "Building RapidSift..."
make -j"$PARALLEL_JOBS"

if [ $? -ne 0 ]; then
    print_error "Build failed"
    exit 1
fi

print_success "Build completed successfully"

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    print_status "Running tests..."
    
    # Run individual test suites
    echo ""
    echo "Running individual test suites:"
    
    if [ -f "./test_utils" ]; then
        print_status "Running utilities tests..."
        ./test_utils
    fi
    
    if [ -f "./test_exact_dedup" ]; then
        print_status "Running exact deduplication tests..."
        ./test_exact_dedup
    fi
    
    if [ -f "./test_near_dedup" ]; then
        print_status "Running near-duplicate detection tests..."
        ./test_near_dedup
    fi
    
    if [ -f "./run_all_tests" ]; then
        print_status "Running comprehensive test suite..."
        ./run_all_tests
    fi
    
    # Run performance tests if benchmarking is enabled
    if [ "$BENCHMARK" = true ] && [ -f "./performance_test" ]; then
        print_status "Running performance benchmarks..."
        ./performance_test
    fi
    
    # Run CTest
    print_status "Running CTest..."
    ctest --output-on-failure --verbose
    
    if [ $? -eq 0 ]; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed"
        exit 1
    fi
fi

# Install if requested
if [ "$INSTALL" = true ]; then
    print_status "Installing RapidSift..."
    
    if [ "$EUID" -ne 0 ]; then
        print_warning "Installation may require root privileges"
        sudo make install
    else
        make install
    fi
    
    if [ $? -eq 0 ]; then
        print_success "Installation completed"
    else
        print_error "Installation failed"
        exit 1
    fi
fi

# Final summary
echo ""
print_success "RapidSift build process completed!"
echo "=================================="

if [ -f "./rapidsift" ]; then
    echo "Executable: $PWD/rapidsift"
    echo ""
    echo "Usage examples:"
    echo "  $PWD/rapidsift --help"
    echo "  $PWD/rapidsift --mode exact --input data.txt"
    echo "  $PWD/rapidsift --mode near --threshold 0.8 --input data.csv"
    echo ""
fi

if [ "$BUILD_TYPE" = "Release" ]; then
    echo "🚀 Release build optimized for maximum performance!"
else
    echo "🔧 Debug build ready for development and debugging!"
fi

if [ "$ENABLE_OPENMP" = "ON" ]; then
    echo "⚡ OpenMP enabled for parallel processing!"
fi

echo ""
print_success "Ready for ultra-fast text deduplication!" 