# RapidSift Content Filter

A comprehensive C++ content filtering system for removing unsafe, toxic, and privacy-violating content from text datasets. This module provides source-level filtering, toxicity detection, and PII removal capabilities for large-scale text processing pipelines.

## Features

### 🛡️ Domain & Source Filtering
- **Domain Blocklists**: Filter content based on source domain reputation
- **URL Pattern Analysis**: Detect suspicious URL structures and patterns
- **IP Address Blocking**: Block content from IP-based URLs
- **URL Shortener Detection**: Identify and filter URL shortening services
- **Domain Reputation Scoring**: Calculate domain trustworthiness scores

### 🚫 Toxicity & Offensive Content Detection
- **Hate Speech Detection**: Identify hate speech and discriminatory content
- **Profanity Filtering**: Remove content with excessive profanity
- **Harassment Detection**: Flag targeted harassment and bullying
- **Violence Detection**: Identify graphic violence descriptions
- **NSFW Content Filtering**: Remove explicit sexual or adult content
- **Context-Aware Analysis**: Distinguish between problematic and educational content

### 🔒 Privacy & PII Protection
- **Email Address Removal**: Detect and anonymize email addresses
- **Phone Number Sanitization**: Remove or mask phone numbers
- **SSN Detection**: Identify and remove social security numbers
- **Credit Card Protection**: Detect and remove credit card numbers
- **Address Anonymization**: Remove physical addresses
- **IP Address Filtering**: Remove IP addresses from content
- **Named Entity Recognition**: Identify person names and organizations

## Quick Start

### Building

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake pkg-config libxxhash-dev libre2-dev libomp-dev libcurl4-openssl-dev nlohmann-json3-dev

# Or use the automated installer
./build.sh --install-deps

# Build the project
./build.sh

# Build with tests
./build.sh --test --verbose
```

### Basic Usage

```bash
# Basic content filtering
./build/content_filter -i input.txt -o output.txt

# Remove PII and filter domains
./build/content_filter -i data.txt -o clean_data.txt \
    --remove-emails --remove-phones --remove-ssn \
    --blocked-domains spam.com,malware.net \
    --sanitize-mode

# Process JSON format with full privacy protection
./build/content_filter -i data.json -f json -o filtered.json \
    --remove-emails --remove-phones --remove-addresses \
    --use-placeholders --toxicity-threshold 0.8

# Analyze content without filtering
./build/content_filter -i data.txt --analyze --verbose
```

## Configuration

### Command Line Options

#### Input/Output
- `-i, --input FILE`: Input file path
- `-o, --output FILE`: Output file path  
- `-f, --format FORMAT`: File format (text|json)
- `--config FILE`: Load configuration from JSON file

#### Domain Filtering
- `--blocked-domains LIST`: Comma-separated blocked domains
- `--allowed-domains LIST`: Comma-separated allowed domains
- `--block-ip-urls`: Block URLs with IP addresses
- `--domain-list FILE`: Load domain blocklist from file

#### Toxicity Filtering
- `--toxicity-threshold FLOAT`: Toxicity rejection threshold (0-1)
- `--hate-threshold FLOAT`: Hate speech threshold (0-1)
- `--nsfw-threshold FLOAT`: NSFW content threshold (0-1)
- `--enable-context`: Enable context-aware detection

#### PII Protection
- `--remove-emails`: Remove email addresses
- `--remove-phones`: Remove phone numbers
- `--remove-ssn`: Remove social security numbers
- `--remove-addresses`: Remove physical addresses
- `--remove-names`: Remove person names
- `--use-placeholders`: Use placeholders instead of removing

#### Processing Modes
- `--sanitize-mode`: Sanitize content instead of rejecting
- `--strict-mode`: Strict filtering mode
- `--privacy-mode`: Focus on privacy protection

#### Performance
- `-t, --threads N`: Number of threads (0 = auto)
- `--verbose`: Verbose output
- `--analyze`: Analyze without filtering
- `--benchmark`: Run benchmark mode

### Configuration Files

#### Domain Blocklist (`config/blocked_domains.txt`)
```
# One domain per line, comments start with #
spam-site.com
malware-host.net
phishing-example.org
```

#### PII Patterns (`config/pii_patterns.json`)
```json
{
  "email_patterns": ["\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Z|a-z]{2,}\\b"],
  "phone_patterns": ["\\b\\d{3}-\\d{3}-\\d{4}\\b"],
  "safe_domains": ["example.com", "test.com"],
  "placeholders": {
    "email": "[EMAIL]",
    "phone": "[PHONE]"
  }
}
```

## Programming Interface

### Basic Usage

```cpp
#include "rapidsift/content_processor.hpp"

using namespace rapidsift::content;

// Create documents
std::vector<Document> documents = {
    Document("doc1", "Contact us at user@company.com", "https://example.com"),
    Document("doc2", "Toxic content here", "https://spam-site.com")
};

// Configure filtering
ContentConfig config;
config.pii_config.remove_emails = true;
config.domain_config.blocked_domains = {"spam-site.com"};
config.sanitize_mode = true;

// Process documents
ContentProcessor processor(config);
auto filtered_docs = processor.sanitize_documents(documents);

// Check statistics
const auto& stats = processor.get_stats();
std::cout << "Processed: " << stats.total_processed << std::endl;
std::cout << "Kept: " << stats.kept << std::endl;
std::cout << "Rejected: " << stats.rejected << std::endl;
```

### Advanced Configuration

```cpp
// Create comprehensive configuration
ContentConfig config;

// Domain filtering
config.domain_config.blocked_domains = {"spam.com", "malware.net"};
config.domain_config.block_ip_addresses = true;
config.domain_config.check_url_shorteners = true;

// Toxicity detection
config.toxicity_config.toxicity_threshold = 0.7;
config.toxicity_config.hate_speech_threshold = 0.8;
config.toxicity_config.context_aware = true;

// PII protection
config.pii_config.remove_emails = true;
config.pii_config.remove_phones = true;
config.pii_config.use_placeholders = true;
config.pii_config.anonymize_instead_of_remove = true;

// Processing settings
config.sanitize_mode = true;
config.parallel_processing = true;
config.num_threads = 8;
```

### Individual Filter Usage

```cpp
// Use individual filters
DomainFilter domain_filter;
domain_filter.add_blocked_domain("spam.com");

ToxicityFilter toxicity_filter;
toxicity_filter.add_hate_keyword("hate_term");

PIIFilter pii_filter;
pii_filter.add_custom_pattern("\\b\\d{3}-\\d{2}-\\d{4}\\b", PIIType::SSN);

// Evaluate documents
auto domain_decision = domain_filter.evaluate(document);
auto toxicity_decision = toxicity_filter.evaluate(document);
auto pii_decision = pii_filter.evaluate(document);
```

## Performance

### Benchmarks
- **Processing Speed**: 10,000-50,000 documents/second (depending on content and filters)
- **Memory Usage**: ~100MB base + 2x document size during processing
- **CPU Scaling**: Near-linear scaling with CPU cores
- **Throughput**: 1-10 GB/hour text processing (typical web content)

### Optimization Tips
1. **Use parallel processing** for large datasets
2. **Tune filter thresholds** based on your quality requirements
3. **Precompile domain lists** for faster startup
4. **Batch process** large files to reduce memory usage
5. **Use sanitization mode** instead of rejection when possible

## Data Sources & Research

This implementation is inspired by and follows best practices from:

### Research Papers & Projects
- **FineWeb**: HuggingFace's 15T token filtered dataset
- **Gopher**: DeepMind's 280B parameter model training pipeline
- **C4**: Google's Colossal Clean Crawled Corpus
- **Common Crawl**: Web crawl filtering methodologies

### Domain Filtering
- Based on FineWeb's URL filtering approach
- Incorporates domain reputation scoring
- Uses pattern matching for suspicious URLs
- Implements IP address and shortener detection

### Toxicity Detection
- Keyword-based filtering with context awareness
- Inspired by Perspective API methodology
- Implements severity-based thresholding
- Supports medical/educational content exceptions

### PII Protection
- Regex-based pattern matching for common PII types
- Luhn algorithm validation for credit cards
- Named entity recognition for person names
- Context-aware false positive reduction

## Integration Examples

### Pipeline Integration
```cpp
// Integrate with existing pipeline
class DataPipeline {
    ContentProcessor content_filter_;
    
public:
    std::vector<Document> process_batch(const std::vector<Document>& docs) {
        // Content filtering step
        auto filtered_docs = content_filter_.sanitize_documents(docs);
        
        // Continue with other processing steps
        return filtered_docs;
    }
};
```

### Streaming Processing
```cpp
// Stream processing for large datasets
ContentProcessor processor(config);

std::ifstream input("large_dataset.jsonl");
std::ofstream output("filtered_dataset.jsonl");

std::string line;
while (std::getline(input, line)) {
    Document doc = parse_json_line(line);
    
    if (processor.should_keep_document(doc)) {
        output << serialize_document(doc) << std::endl;
    }
}
```

## Safety & Ethics

### Content Policy Compliance
- Implements responsible AI content filtering
- Follows industry best practices for safety
- Provides transparency in filtering decisions
- Supports audit trails and reporting

### Privacy Protection
- GDPR compliance through PII removal
- Supports data anonymization workflows  
- Prevents personal information leakage
- Configurable privacy protection levels

### Bias Mitigation
- Context-aware toxicity detection
- Medical/educational content exceptions
- Configurable sensitivity levels
- Support for custom keyword lists

## Contributing

### Development Setup
```bash
git clone <repository>
cd filters/content
./build.sh --install-deps --test
```

### Adding New Filters
1. Inherit from `ContentFilter` base class
2. Implement `evaluate()` method
3. Add to `ContentProcessor` initialization
4. Update configuration structures
5. Add tests and documentation

### Testing
```bash
# Run all tests
./build.sh --test

# Manual testing
./build/content_filter -i test_data.txt --analyze --verbose
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- HuggingFace team for FineWeb methodology
- Google for C4 dataset filtering approaches  
- DeepMind for Gopher pipeline insights
- OpenAI for responsible AI practices
- Common Crawl for web-scale filtering techniques 