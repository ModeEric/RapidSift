#!/bin/bash

# RapidSift Content Filter Build Script
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}RapidSift Content Filter Build Script${NC}"
echo "====================================="

# Parse command line arguments
BUILD_TYPE="Release"
INSTALL_DEPS=false
RUN_TESTS=false
CLEAN_BUILD=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --install-deps)
            INSTALL_DEPS=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --clean)
            CLEAN_BUILD=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug         Build in debug mode"
            echo "  --install-deps  Install system dependencies"
            echo "  --test          Run tests after building"
            echo "  --clean         Clean build before building"
            echo "  --verbose       Verbose output"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option $1"
            exit 1
            ;;
    esac
done

# Function to detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    else
        echo "unknown"
    fi
}

OS=$(detect_os)
echo -e "${YELLOW}Detected OS: $OS${NC}"

# Install dependencies
install_dependencies() {
    echo -e "${YELLOW}Installing dependencies...${NC}"
    
    if [[ "$OS" == "linux" ]]; then
        # Ubuntu/Debian
        if command -v apt-get &> /dev/null; then
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                pkg-config \
                libxxhash-dev \
                libre2-dev \
                libomp-dev \
                libcurl4-openssl-dev \
                nlohmann-json3-dev
        # CentOS/RHEL/Fedora
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y \
                gcc-c++ \
                cmake \
                pkgconfig \
                xxhash-devel \
                re2-devel \
                libomp-devel \
                libcurl-devel \
                json-devel
        elif command -v yum &> /dev/null; then
            sudo yum install -y \
                gcc-c++ \
                cmake \
                pkgconfig \
                xxhash-devel \
                re2-devel \
                libcurl-devel
        fi
    elif [[ "$OS" == "macos" ]]; then
        # macOS with Homebrew
        if command -v brew &> /dev/null; then
            brew install \
                cmake \
                pkg-config \
                xxhash \
                re2 \
                libomp \
                curl \
                nlohmann-json
        else
            echo -e "${RED}Homebrew not found. Please install Homebrew first.${NC}"
            exit 1
        fi
    fi
    
    echo -e "${GREEN}Dependencies installed successfully${NC}"
}

# Install dependencies if requested
if [[ "$INSTALL_DEPS" == true ]]; then
    install_dependencies
fi

# Check for required tools
echo -e "${YELLOW}Checking build tools...${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}CMake not found. Please install CMake.${NC}"
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo -e "${RED}pkg-config not found. Please install pkg-config.${NC}"
    exit 1
fi

echo -e "${GREEN}Build tools check passed${NC}"

# Create build directory
BUILD_DIR="build"
if [[ "$CLEAN_BUILD" == true ]] && [[ -d "$BUILD_DIR" ]]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure CMake
echo -e "${YELLOW}Configuring build...${NC}"
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

if [[ "$VERBOSE" == true ]]; then
    CMAKE_ARGS+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
fi

cmake "${CMAKE_ARGS[@]}" ..

# Build
echo -e "${YELLOW}Building...${NC}"
CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
make -j"$CORES"

echo -e "${GREEN}Build completed successfully${NC}"

# Run tests if requested
if [[ "$RUN_TESTS" == true ]]; then
    echo -e "${YELLOW}Running tests...${NC}"
    if [[ -f "content_filter" ]]; then
        # Create test data
        echo "Creating test data..."
        cat > test_input.txt << 'EOF'
This is a safe document about technology.
Contact john.doe@example.com for more information or call 555-123-4567.
Visit our malicious site at https://spam-site.com/malware
This content has some offensive language that should be filtered.
My personal info: SSN 123-45-6789, CC 4532-1234-5678-9012
EOF
        
        echo "Running content filter on test data..."
        ./content_filter -i test_input.txt -o test_output.txt --remove-emails --remove-phones --remove-ssn --blocked-domains spam-site.com --sanitize-mode --verbose
        
        if [[ -f "test_output.txt" ]]; then
            echo -e "${GREEN}Test completed. Output saved to test_output.txt${NC}"
            echo "Filtered content:"
            cat test_output.txt
        else
            echo -e "${RED}Test failed - no output file generated${NC}"
        fi
        
        # Cleanup
        rm -f test_input.txt test_output.txt
    else
        echo -e "${RED}content_filter executable not found${NC}"
        exit 1
    fi
fi

# Run example if available
if [[ -f "examples/content_example" ]]; then
    echo -e "${YELLOW}Running example...${NC}"
    ./examples/content_example
fi

cd ..

echo -e "${GREEN}All done!${NC}"
echo ""
echo "Build artifacts are in the 'build' directory:"
echo "  - content_filter: Main executable"
echo "  - examples/content_example: Example program"
echo ""
echo "Usage examples:"
echo "  ./build/content_filter -i input.txt -o output.txt --remove-emails --blocked-domains spam.com"
echo "  ./build/content_filter -i data.json -f json --sanitize-mode --verbose"
echo "  ./build/examples/content_example" 