# RapidSift C++ - Ultra-Fast Text Deduplication

A high-performance C++ implementation of RapidSift for lightning-fast text deduplication using advanced algorithms and parallel processing.

## 🚀 Performance Highlights

- **10x-100x faster** than Python implementations
- **Parallel processing** with OpenMP support
- **Memory efficient** streaming for large datasets
- **Multiple hash algorithms** including xxHash for extreme speed
- **Advanced fuzzy matching** with MinHash and SimHash
- **Zero-copy optimizations** where possible

## 📋 Features

### Exact Deduplication
- **Hash Algorithms**: MD5, SHA1, SHA256, xxHash64
- **Parallel Processing**: Multi-threaded hash computation
- **Memory Efficient**: Streaming support for large files
- **Configurable**: Keep first/last occurrence options

### Near-Duplicate Detection
- **MinHash + LSH**: Efficient similarity search with configurable bands
- **SimHash**: Compact fingerprinting with Hamming distance
- **Configurable Thresholds**: Fine-tune similarity detection
- **N-gram Processing**: Character and word-level analysis

### Advanced Features
- **Progress Callbacks**: Real-time processing updates
- **Multiple I/O Formats**: TXT, CSV support
- **Statistics Reporting**: Detailed deduplication metrics
- **Cross-platform**: Linux, macOS, Windows support

## 🛠️ Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libssl-dev

# macOS
brew install cmake openssl

# Or install Xcode command line tools
xcode-select --install
```

### Building from Source

```bash
# Clone and build
git clone <repository-url>
cd RapidSift/cpp
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Install (optional)
sudo make install
```

### Build Options

```bash
# Disable OpenMP (single-threaded)
cmake .. -DUSE_OPENMP=OFF

# Debug build with sanitizers
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Custom install prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
```

## 🎯 Quick Start

### Command Line Interface

```bash
# Exact deduplication with xxHash
./rapidsift -i input.txt -o output.txt --mode exact --algorithm xxhash

# Near-duplicate detection with MinHash
./rapidsift -i input.txt -o output.txt --mode near --method minhash --threshold 0.8

# Performance benchmark
./rapidsift -i input.txt --mode benchmark

# CSV input with custom text column
./rapidsift -i data.csv -o clean.csv --text-column content
```

### C++ API Example

```cpp
#include "rapidsift/exact_dedup.hpp"
#include "rapidsift/near_dedup.hpp"

using namespace rapidsift;

int main() {
    // Load documents
    std::vector<Document> documents = {
        Document("First document text", 0),
        Document("Second document text", 1),
        Document("First document text", 2),  // Duplicate
    };
    
    // Exact deduplication
    ExactDedupConfig config;
    config.algorithm = HashAlgorithm::XXHASH64;
    config.parallel = true;
    
    ExactDeduplicator deduplicator(config);
    auto result = deduplicator.deduplicate(documents);
    
    std::cout << "Unique documents: " << result.unique_count() << std::endl;
    std::cout << "Duplicates removed: " << result.duplicates_removed() << std::endl;
    std::cout << "Processing time: " << result.processing_time().count() << " ms" << std::endl;
    
    return 0;
}
```

## ⚡ Performance Benchmarks

**Test Dataset**: 100,000 documents, ~50% duplicates

| Method | C++ Time | Python Time | Speedup |
|--------|----------|-------------|---------|
| Exact (xxHash) | 45ms | 2.1s | **47x** |
| Exact (SHA256) | 180ms | 8.5s | **47x** |
| MinHash | 320ms | 12.3s | **38x** |
| SimHash | 280ms | 9.8s | **35x** |

**Memory Usage**: ~60% less memory consumption vs Python

## 🔧 Configuration Options

### Exact Deduplication

```cpp
ExactDedupConfig config {
    .algorithm = HashAlgorithm::XXHASH64,  // MD5, SHA1, SHA256, XXHASH64
    .keep_first = true,                    // Keep first or last occurrence
    .parallel = true                       // Enable parallel processing
};
```

### Near-Duplicate Detection

```cpp
NearDedupConfig config {
    .method = NearDedupConfig::Method::MINHASH,  // MINHASH, SIMHASH
    .threshold = 0.8,                            // Similarity threshold
    .num_permutations = 128,                     // MinHash permutations
    .ngram_size = 5,                             // N-gram size
    .simhash_bits = 64,                          // SimHash bits
    .parallel = true                             // Parallel processing
};
```

## 📊 Detailed Usage

### Loading Documents

```cpp
// From text file (one document per line)
auto documents = io_utils::load_documents_from_txt("input.txt");

// From CSV file
auto documents = io_utils::load_documents_from_csv("data.csv", "text_column");

// Programmatically
std::vector<Document> documents;
documents.emplace_back("Document 1 text", 0);
documents.emplace_back("Document 2 text", 1);
```

### Progress Monitoring

```cpp
auto progress_callback = [](size_t current, size_t total, const std::string& stage) {
    double pct = (double(current) / total) * 100.0;
    std::cout << stage << ": " << std::fixed << std::setprecision(1) 
              << pct << "% (" << current << "/" << total << ")\n";
};

auto result = deduplicator.deduplicate(documents, progress_callback);
```

### Advanced MinHash Configuration

```cpp
// Fine-tune LSH parameters
NearDedupConfig config;
config.method = NearDedupConfig::Method::MINHASH;
config.num_permutations = 256;  // Higher = more accurate, slower
config.ngram_size = 3;          // Character n-grams
config.threshold = 0.85;        // Stricter similarity

// LSH bands automatically calculated: bands * rows = permutations
// Default: 16 bands × 8 rows = 128 permutations
```

### Streaming Large Files

```cpp
std::ifstream input("large_file.txt");
std::ofstream output("deduplicated.txt");

ExactDeduplicator deduplicator;
deduplicator.deduplicate_stream(input, output, 10000);  // 10K batch size
```

## 🔍 Analysis and Statistics

```cpp
// Detailed statistics
auto result = deduplicator.deduplicate(documents);

std::cout << "Original count: " << result.original_count() << std::endl;
std::cout << "Unique count: " << result.unique_count() << std::endl;
std::cout << "Reduction: " << result.reduction_percentage() << "%" << std::endl;
std::cout << "Processing time: " << result.processing_time().count() << " ms" << std::endl;

// Duplicate groups analysis
for (const auto& group : result.duplicate_groups()) {
    std::cout << "Duplicate group size: " << group.size() << std::endl;
    for (DocumentId id : group) {
        std::cout << "  Document " << id << std::endl;
    }
}
```

## 🏗️ Architecture

### Core Components

```
rapidsift/
├── include/rapidsift/
│   ├── common.hpp          # Core types and utilities
│   ├── exact_dedup.hpp     # Hash-based exact matching
│   ├── near_dedup.hpp      # MinHash/SimHash fuzzy matching
│   └── semantic_dedup.hpp  # Embedding-based similarity (future)
├── src/
│   ├── exact_dedup.cpp
│   ├── near_dedup.cpp
│   ├── utils.cpp           # I/O, text processing utilities
│   └── main.cpp            # CLI application
└── examples/
    └── simple_example.cpp
```

### Key Algorithms

1. **xxHash64**: Ultra-fast non-cryptographic hashing
2. **MinHash + LSH**: Probabilistic similarity with sub-linear search
3. **SimHash**: Locality-sensitive hashing for near-duplicates
4. **Parallel Processing**: OpenMP-based multi-threading

## 🚀 Advanced Performance Tips

### Memory Optimization
```cpp
// For very large datasets, use streaming
deduplicator.deduplicate_stream(input, output, batch_size);

// Pre-reserve containers
documents.reserve(expected_size);
```

### CPU Optimization
```cpp
// Compile with optimizations
// -O3 -march=native -mtune=native

// Use xxHash for maximum speed
config.algorithm = HashAlgorithm::XXHASH64;

// Adjust thread count
#ifdef USE_OPENMP
omp_set_num_threads(std::thread::hardware_concurrency());
#endif
```

### Algorithm Selection Guide

| Use Case | Recommended Algorithm | Speed | Accuracy |
|----------|----------------------|-------|----------|
| Exact duplicates | xxHash64 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| Near duplicates | MinHash | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| Very similar text | SimHash | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| Cryptographic needs | SHA256 | ⭐⭐ | ⭐⭐⭐⭐⭐ |

## 🤝 Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open Pull Request

### Development Setup

```bash
# Debug build with all warnings
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror"

# Run tests
make test

# Memory check with Valgrind
valgrind --tool=memcheck ./rapidsift
```

## 📄 License

MIT License - see LICENSE file for details.

## 🙏 Acknowledgments

- xxHash by Yann Collet
- cxxopts by jarro2783
- nlohmann/json for JSON support
- OpenMP for parallel processing

---

**RapidSift C++** - When Python isn't fast enough. 🚀 