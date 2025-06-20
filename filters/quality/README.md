# RapidSift Quality Filter

A high-performance C++ system for filtering low-quality text documents using various heuristic rules. This is part of the RapidSift project for processing large-scale text corpora.

## Features

### 🔍 Comprehensive Quality Filtering

- **Length-based filters**: Remove documents that are too short or too long
- **Gibberish detection**: Identify non-linguistic text, random characters, and excessive repetition
- **Repetition analysis**: Detect boilerplate content, repeated lines, and low unique word counts
- **Format validation**: Filter out HTML/markup heavy content, code, and broken formatting
- **Metadata filtering**: Block suspicious domains, detect machine-generated content

### ⚡ High Performance

- **Parallel processing**: Multi-threaded filtering with OpenMP
- **Optimized algorithms**: Efficient text analysis with minimal memory overhead
- **Streaming support**: Process large datasets without loading everything into memory
- **Configurable thresholds**: Fine-tune filtering criteria for your specific use case

### 🛠️ Easy to Use

- **Command-line interface**: Simple CLI with comprehensive options
- **Multiple formats**: Support for text files and JSON documents
- **Detailed reporting**: Statistics and analysis of filtering results
- **Flexible configuration**: JSON-based configuration files

## Quick Start

### Build and Test

```bash
# Build the system with sample data
./build.sh all

# Or step by step:
./build.sh deps     # Install dependencies
./build.sh data     # Create test data
./build.sh build    # Build the project
./build.sh test     # Run tests
```

### Basic Usage

```bash
# Filter a text file (one document per line)
./build/quality_filter -i documents.txt -o filtered.txt

# Filter JSON documents
./build/quality_filter -i documents.json -o filtered.json -f json

# Analyze without filtering (see what would be removed)
./build/quality_filter -i documents.txt --analyze --verbose

# Custom filtering thresholds
./build/quality_filter -i documents.txt -o filtered.txt \
    --min-words 10 --max-words 50000 \
    --min-entropy 2.5 --max-non-alpha 0.2
```

## Quality Filters

### 1. Length Filter

Removes documents that are too short or too long based on word/character counts.

**Default thresholds:**
- Minimum words: 5
- Maximum words: 1,000,000
- Minimum characters: 20
- Maximum characters: 10,000,000

**Example rejections:**
- `"Hi"` (too short)
- Extremely long concatenated dumps

### 2. Gibberish Filter

Detects non-linguistic text using multiple heuristics:

- **Character composition**: Excessive non-alphabetic characters, digits, or symbols
- **Entropy analysis**: Very low character entropy indicating repetitive patterns
- **Linguistic patterns**: Unreasonable vowel/consonant ratios
- **Pattern matching**: Common gibberish patterns (long random strings, excessive repetition)

**Example rejections:**
- `"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"` (excessive repetition)
- `"XYZ123ABC789XYZ123ABC789XYZ123ABC789"` (non-linguistic pattern)
- `"$#@!%^&*()_+{}|:<>?"` (excessive symbols)

### 3. Repetition Filter

Identifies content with high repetition or boilerplate characteristics:

- **Line repetition**: Documents where many lines are identical
- **N-gram repetition**: Excessive repeated phrases or sentences
- **Lexical diversity**: Low unique word ratios
- **Boilerplate detection**: Common template patterns and repeated structural elements

**Example rejections:**
- Documents with >30% repeated lines
- Content with <30% unique words
- Template-like structures with repeated patterns

### 4. Format Filter

Detects formatting anomalies and unwanted content types:

- **HTML/markup**: Content that's mostly HTML tags rather than text
- **Code detection**: Programming code mixed with text
- **List-heavy content**: Documents that are primarily bullet lists
- **Broken formatting**: Poor line wrapping or structure

**Example rejections:**
- `"<div><p><span>Content</span></p></div>"` (mostly HTML)
- `"function test() { return 42; }"` (code content)
- Documents with >60% bullet list items

### 5. Metadata Filter

Uses URL, domain, and metadata signals for filtering:

- **Domain blocking**: Configurable blocklists of spam/low-quality domains
- **URL analysis**: Suspicious patterns in URLs
- **Machine translation**: Detection of auto-translated content
- **Content type validation**: Filtering non-text content types

**Example rejections:**
- Documents from blocked domains
- URLs with spam keywords
- Content with translation indicators

## Configuration

### Command Line Options

```bash
# Length filtering
--min-words N        # Minimum word count (default: 5)
--max-words N        # Maximum word count (default: 1000000)
--min-chars N        # Minimum character count (default: 20)
--max-chars N        # Maximum character count (default: 10000000)

# Gibberish detection
--max-non-alpha F    # Maximum non-alphabetic ratio (default: 0.3)
--min-entropy F      # Minimum character entropy (default: 2.0)

# Processing
--threads N          # Number of threads (default: auto)
--verbose            # Detailed output
--analyze            # Analysis mode (don't filter, just report)
--benchmark          # Run performance benchmarks
```

### Configuration Files

Create custom configurations in JSON format:

```json
{
  "length_config": {
    "min_words": 10,
    "max_words": 100000,
    "min_chars": 50,
    "max_chars": 1000000,
    "strict_mode": false
  },
  "gibberish_config": {
    "max_non_alpha_ratio": 0.2,
    "max_digit_ratio": 0.3,
    "min_entropy": 2.5,
    "max_consecutive_chars": 30
  },
  "repetition_config": {
    "max_line_repetition_ratio": 0.2,
    "min_unique_word_ratio": 0.4,
    "max_boilerplate_ratio": 0.6
  }
}
```

## Input/Output Formats

### Text Format

Simple text files with one document per line:

```
This is the first document.
This is the second document with more content.
Short.
This is a longer document with multiple sentences and more comprehensive content.
```

### JSON Format

Structured documents with metadata:

```json
[
  {
    "id": "doc_001",
    "text": "Document content here...",
    "url": "https://example.com/article",
    "domain": "example.com",
    "content_type": "text/html",
    "metadata": {
      "author": "John Doe",
      "date": "2024-01-01"
    }
  }
]
```

## Performance

### Benchmarks

On a typical dataset of 1M documents:

- **Processing speed**: ~10,000-50,000 documents/second (depending on filters enabled)
- **Memory usage**: ~100MB for filtering logic + document storage
- **Parallel scaling**: Near-linear speedup with additional CPU cores

### Optimization Tips

1. **Use appropriate thread count**: Usually optimal at number of CPU cores
2. **Enable specific filters**: Disable unused filters for better performance
3. **Batch processing**: Process large files in chunks for memory efficiency
4. **SSD storage**: I/O performance matters for large datasets

## Examples

### Real-world Filtering Pipeline

```bash
# Step 1: Analyze your dataset
./build/quality_filter -i raw_crawl.txt --analyze --stats analysis_stats.json

# Step 2: Apply conservative filtering
./build/quality_filter -i raw_crawl.txt -o filtered_conservative.txt \
    --min-words 10 --max-words 50000 --min-entropy 2.0

# Step 3: Apply aggressive filtering for high-quality subset
./build/quality_filter -i filtered_conservative.txt -o filtered_aggressive.txt \
    --min-words 25 --max-words 10000 --min-entropy 3.0 --max-non-alpha 0.15
```

### Integration with Other Tools

```bash
# Combine with deduplication
./build/quality_filter -i documents.txt -o quality_filtered.txt
../dedup/build/dedup_filter -i quality_filtered.txt -o final_output.txt

# Process streaming data
cat large_dataset.txt | ./build/quality_filter -i /dev/stdin -o filtered_stream.txt
```

## Development

### Building from Source

Requirements:
- C++17 compatible compiler (GCC 7+, Clang 5+)
- CMake 3.12+
- OpenMP
- xxHash library
- nlohmann/json library

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake pkg-config libxxhash-dev libomp-dev nlohmann-json3-dev

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Testing

```bash
# Run all tests
./build.sh test

# Run specific test categories
./build/tests/length_filter_test
./build/tests/gibberish_filter_test
./build/tests/integration_test
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is part of RapidSift and follows the same licensing terms.

## Acknowledgments

Inspired by quality filtering approaches from:
- FineWeb (Hugging Face)
- C4 (Google)
- RefinedWeb (Various research papers)
- Common Crawl filtering pipelines 