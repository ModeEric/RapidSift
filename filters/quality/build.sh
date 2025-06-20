#!/bin/bash

# RapidSift Quality Filter Build Script
# Builds and tests the quality filtering system

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build configuration
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-build}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}
NUM_JOBS=${NUM_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)}

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

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install system dependencies
install_dependencies() {
    print_status "Checking and installing system dependencies..."
    
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # Ubuntu/Debian
        if command_exists apt-get; then
            print_status "Installing dependencies with apt-get..."
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                pkg-config \
                libxxhash-dev \
                libomp-dev \
                libgtest-dev \
                nlohmann-json3-dev
                
        # CentOS/RHEL/Fedora
        elif command_exists yum; then
            print_status "Installing dependencies with yum..."
            sudo yum install -y \
                gcc-c++ \
                cmake \
                pkgconfig \
                xxhash-devel \
                libomp-devel \
                gtest-devel \
                json-devel
                
        elif command_exists dnf; then
            print_status "Installing dependencies with dnf..."
            sudo dnf install -y \
                gcc-c++ \
                cmake \
                pkgconfig \
                xxhash-devel \
                libomp-devel \
                gtest-devel \
                nlohmann-json-devel
        else
            print_warning "Unknown Linux distribution. Please install dependencies manually."
        fi
        
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        if command_exists brew; then
            print_status "Installing dependencies with Homebrew..."
            brew install cmake xxhash libomp nlohmann-json googletest
        else
            print_error "Homebrew not found. Please install it first: https://brew.sh/"
            exit 1
        fi
    else
        print_warning "Unknown operating system. Please install dependencies manually."
    fi
}

# Function to create sample data for testing
create_sample_data() {
    print_status "Creating sample test data..."
    
    mkdir -p test_data
    
    # Create sample documents with various quality issues
    cat > test_data/sample_documents.txt << 'EOF'
This is a high-quality document with proper grammar, reasonable length, and meaningful content. It discusses various topics in a coherent manner and provides valuable information to readers.
Hi
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Click here to learn more! Click here to learn more! Click here to learn more! Click here to learn more! Click here to learn more! Click here to learn more!
<html><body><div><p>This is some HTML content</p></div></body></html>
function test() { return 42; } class MyClass { public: void method(); };
• Item 1
• Item 2  
• Item 3
• Item 4
• Item 5
• Item 6
• Item 7
• Item 8
• Item 9
• Item 10
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
SPAM SPAM SPAM BUY NOW VIAGRA CIALIS CHEAP LOANS CREDIT REPAIR MAKE MONEY FAST!!!
This document contains some reasonable content mixed with various formatting issues and maintains a good balance of information while being neither too short nor excessively long.
EOF

    # Create JSON format sample
    cat > test_data/sample_documents.json << 'EOF'
[
  {
    "id": "doc_001",
    "text": "This is a high-quality document with proper grammar, reasonable length, and meaningful content.",
    "url": "https://example.com/article1",
    "domain": "example.com",
    "content_type": "text/html"
  },
  {
    "id": "doc_002", 
    "text": "Hi",
    "url": "https://spam-site.com/short",
    "domain": "spam-site.com",
    "content_type": "text/html"
  },
  {
    "id": "doc_003",
    "text": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "url": "https://broken-site.com/gibberish",
    "domain": "broken-site.com",
    "content_type": "text/plain"
  }
]
EOF

    print_success "Sample data created in test_data/"
}

# Function to build the project
build_project() {
    print_status "Building RapidSift Quality Filter..."
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake
    print_status "Configuring with CMake..."
    cmake .. \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    # Build
    print_status "Building with $NUM_JOBS parallel jobs..."
    make -j"$NUM_JOBS"
    
    cd ..
    print_success "Build completed successfully!"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    if [ -d "$BUILD_DIR" ]; then
        cd "$BUILD_DIR"
        
        # Run unit tests if available
        if [ -f "tests/quality_tests" ]; then
            print_status "Running unit tests..."
            ./tests/quality_tests
        else
            print_warning "Unit tests not found, skipping..."
        fi
        
        # Run integration tests with sample data
        if [ -f "quality_filter" ]; then
            print_status "Running integration tests..."
            
            # Test with text format
            ./quality_filter -i ../test_data/sample_documents.txt -o ../test_data/filtered_output.txt --verbose
            
            # Test with JSON format
            ./quality_filter -i ../test_data/sample_documents.json -o ../test_data/filtered_output.json -f json --verbose
            
            # Test analysis mode
            ./quality_filter -i ../test_data/sample_documents.txt --analyze --verbose
            
            print_success "Integration tests completed!"
        else
            print_error "quality_filter executable not found!"
            return 1
        fi
        
        cd ..
    else
        print_error "Build directory not found. Please run build first."
        return 1
    fi
}

# Function to install the project
install_project() {
    print_status "Installing RapidSift Quality Filter..."
    
    if [ -d "$BUILD_DIR" ]; then
        cd "$BUILD_DIR"
        sudo make install
        cd ..
        print_success "Installation completed!"
    else
        print_error "Build directory not found. Please run build first."
        return 1
    fi
}

# Function to clean build artifacts
clean_build() {
    print_status "Cleaning build artifacts..."
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned!"
    fi
    
    if [ -d "test_data" ]; then
        rm -rf test_data
        print_success "Test data cleaned!"
    fi
}

# Function to benchmark the system
run_benchmark() {
    print_status "Running benchmark..."
    
    if [ -d "$BUILD_DIR" ] && [ -f "$BUILD_DIR/quality_filter" ]; then
        cd "$BUILD_DIR"
        ./quality_filter -i ../test_data/sample_documents.txt --benchmark --verbose
        cd ..
    else
        print_error "Build not found. Please run build first."
        return 1
    fi
}

# Function to show usage
show_usage() {
    echo "RapidSift Quality Filter Build Script"
    echo ""
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Options:"
    echo "  deps      Install system dependencies"
    echo "  build     Build the project (default)"
    echo "  test      Run tests"
    echo "  install   Install the project"
    echo "  clean     Clean build artifacts"
    echo "  data      Create sample test data"
    echo "  benchmark Run performance benchmark"
    echo "  all       Run deps, build, data, and test"
    echo "  help      Show this help message"
    echo ""
    echo "Environment variables:"
    echo "  BUILD_TYPE     Build type (Debug|Release) [default: Release]"
    echo "  BUILD_DIR      Build directory [default: build]"
    echo "  INSTALL_PREFIX Installation prefix [default: /usr/local]"
    echo "  NUM_JOBS       Number of parallel build jobs [default: nproc]"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  BUILD_TYPE=Debug $0 build"
    echo "  $0 all"
}

# Main execution
main() {
    local action=${1:-build}
    
    case "$action" in
        deps)
            install_dependencies
            ;;
        build)
            build_project
            ;;
        test)
            run_tests
            ;;
        install)
            install_project
            ;;
        clean)
            clean_build
            ;;
        data)
            create_sample_data
            ;;
        benchmark)
            run_benchmark
            ;;
        all)
            install_dependencies
            create_sample_data
            build_project
            run_tests
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            print_error "Unknown action: $action"
            show_usage
            exit 1
            ;;
    esac
}

# Check if script is being sourced or executed
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi 