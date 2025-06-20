# Downstream Data Decontamination

## Overview

Data decontamination is critical for preventing evaluation benchmark leakage into training datasets. When benchmark data appears in training sets, models can memorize answers rather than learning to generalize, leading to artificially inflated evaluation scores.

This filter implements sophisticated n-gram overlap detection to identify and remove training examples that contain content from evaluation benchmarks, ensuring fair and accurate model evaluation.

## The Problem

**Evaluation Data Contamination** occurs when:
- Training data contains questions/answers from evaluation benchmarks
- Models memorize specific test cases during training
- Evaluation scores become unreliable indicators of true performance
- Research results become difficult to reproduce and compare

Common sources of contamination:
- Web scraping that accidentally includes benchmark websites
- Academic papers that quote benchmark examples
- Tutorial content that uses benchmark questions as examples
- Translated or paraphrased versions of benchmark content

## Key Features

### N-gram Overlap Detection
- **Configurable N-gram Size**: Default 13-grams, adjustable from 8-50
- **Multiple Algorithms**: Exact matching, approximate matching, fuzzy matching
- **Efficient Processing**: Bloom filters for large-scale datasets
- **Batch Processing**: Optimized for high-throughput scenarios

### Benchmark Dataset Support
- **Popular Benchmarks**: TriviaQA, SQuAD, GLUE, SuperGLUE, HellaSwag, etc.
- **Custom Benchmarks**: Load any evaluation dataset format
- **Multiple Sources**: Process benchmarks from files or directories
- **Format Support**: JSON, CSV, plain text, structured formats

### Performance Optimization
- **Bloom Filters**: Fast approximate matching for large datasets
- **Parallel Processing**: Multi-threaded contamination detection
- **Memory Management**: Configurable memory limits and streaming
- **Caching**: Avoid reprocessing identical content

### Advanced Detection
- **Text Normalization**: Handle case, punctuation, and whitespace variations
- **Tokenization**: Word-level and character-level n-gram extraction
- **Approximate Matching**: Detect paraphrased or slightly modified content
- **Common Phrase Filtering**: Ignore frequently occurring text patterns

## Usage Examples

### Basic Usage
```cpp
#include "rapidsift/decontamination_filter.hpp"

// Create filter with default configuration
auto config = decontamination_utils::create_default_config();
DecontaminationFilter filter(config);

// Load benchmark datasets
filter.load_benchmark_file("benchmarks/triviaqa_test.json", "TriviaQA");
filter.load_benchmark_directory("benchmarks/glue/");

// Check a training document
Document doc("train_001", "What is the capital of France? Paris is the capital.", "source.com");
auto assessment = filter.assess_document(doc);

if (assessment.is_contaminated) {
    std::cout << "Document contaminated with " << assessment.contaminated_ngrams 
              << " n-grams from " << assessment.most_likely_source << std::endl;
}
```

### Configuration Options
```cpp
// Strict configuration - catches more contamination
auto strict_config = decontamination_utils::create_strict_config();
strict_config.ngram_size = 8;                    // Smaller n-grams
strict_config.contamination_threshold = 0.01;    // Lower threshold
strict_config.min_matches_to_reject = 1;         // Reject on any match

// Fast configuration - optimized for speed
auto fast_config = decontamination_utils::create_fast_config();
fast_config.use_bloom_filter = true;             // Fast filtering
fast_config.max_matches_per_document = 10;       // Stop early
fast_config.exclude_common_phrases = true;       // Skip common text
```

### Detailed Assessment
```cpp
auto assessment = filter.assess_document(doc);

std::cout << "Contamination Analysis:\n";
std::cout << "  Is Contaminated: " << assessment.is_contaminated << "\n";
std::cout << "  Contamination Score: " << assessment.contamination_score << "\n";
std::cout << "  Total N-grams: " << assessment.total_ngrams_checked << "\n";
std::cout << "  Contaminated N-grams: " << assessment.contaminated_ngrams << "\n";
std::cout << "  Most Likely Source: " << assessment.most_likely_source << "\n";

// Show specific matches
for (const auto& match : assessment.matches) {
    std::cout << "  Match: \"" << match.ngram << "\" from " 
              << match.source_dataset << std::endl;
}
```

### Batch Processing
```cpp
// Process multiple documents efficiently
std::vector<Document> training_docs = load_training_data();
auto assessments = filter.assess_documents(training_docs);

// Filter out contaminated documents
std::vector<Document> clean_docs;
for (size_t i = 0; i < training_docs.size(); ++i) {
    if (!assessments[i].is_contaminated) {
        clean_docs.push_back(training_docs[i]);
    }
}

std::cout << "Kept " << clean_docs.size() << " out of " 
          << training_docs.size() << " documents" << std::endl;
```

## Configuration Parameters

### N-gram Settings
```cpp
config.ngram_size = 13;              // Standard n-gram size
config.min_ngram_size = 8;           // Minimum for adaptive sizing
config.use_adaptive_ngram_size = false;  // Dynamic sizing based on content
```

### Contamination Thresholds
```cpp
config.contamination_threshold = 0.1;     // 10% contaminated n-grams to reject
config.min_matches_to_reject = 1;         // Minimum matches required
config.max_matches_per_document = 100;    // Stop processing after this many
```

### Text Processing
```cpp
config.normalize_whitespace = true;       // Standardize whitespace
config.case_insensitive = false;          // Case-sensitive matching
config.remove_punctuation = false;        // Keep punctuation
config.tokenize_before_ngrams = true;     // Word-level n-grams
```

### Performance Tuning
```cpp
config.use_bloom_filter = true;           // Enable fast filtering
config.enable_parallel_processing = true; // Multi-threading
config.batch_size = 1000;                 // Documents per batch
config.max_memory_mb = 2048;              // Memory limit
```

### Advanced Options
```cpp
config.check_approximate_matches = false; // Fuzzy matching
config.approximate_match_threshold = 0.9; // Jaccard similarity
config.exclude_common_phrases = true;     // Filter common text
config.min_phrase_frequency_to_exclude = 100; // Frequency threshold
```

## Benchmark Dataset Integration

### Supported Formats

#### JSON Format (SQuAD, TriviaQA)
```json
{
  "data": [
    {
      "title": "Topic",
      "paragraphs": [
        {
          "context": "Background text...",
          "qas": [
            {
              "question": "What is the question?",
              "answers": [{"text": "The answer", "answer_start": 0}]
            }
          ]
        }
      ]
    }
  ]
}
```

#### CSV Format (GLUE tasks)
```csv
sentence1,sentence2,label
"The movie was great","I loved it",1
"It was terrible","I hated it",0
```

#### Plain Text Format
```
What is the capital of France?
Who wrote Romeo and Juliet?
What is the chemical symbol for gold?
```

### Loading Benchmarks
```cpp
// Load individual benchmark files
filter.load_benchmark_file("data/triviaqa_test.json", "TriviaQA");
filter.load_benchmark_file("data/squad_dev.json", "SQuAD");
filter.load_benchmark_file("data/glue_sst.csv", "GLUE-SST");

// Load entire directories
filter.load_benchmark_directory("benchmarks/");

// Add custom n-grams
std::vector<std::string> custom_ngrams = {
    "specific question text",
    "exact answer phrase",
    "evaluation benchmark content"
};
filter.add_benchmark_ngrams(custom_ngrams, "CustomBenchmark");
```

## Detection Strategies

### Exact N-gram Matching
Most reliable approach that finds exact text overlap:
```cpp
// 13-gram exact matching (default)
config.ngram_size = 13;
config.check_approximate_matches = false;
```

### Approximate Matching
Detects paraphrased or slightly modified content:
```cpp
config.check_approximate_matches = true;
config.approximate_match_threshold = 0.9;  // 90% similarity
```

### Multi-Scale Detection
Uses multiple n-gram sizes for comprehensive coverage:
```cpp
config.use_adaptive_ngram_size = true;
config.min_ngram_size = 8;
config.ngram_size = 16;  // Maximum size
```

### Common Phrase Filtering
Ignores frequently occurring text that's not meaningful contamination:
```cpp
config.exclude_common_phrases = true;
config.min_phrase_frequency_to_exclude = 100;
```

## Performance Optimization

### Bloom Filter Acceleration
```cpp
// Enable bloom filter for 10x+ speedup on large datasets
config.use_bloom_filter = true;

// Bloom filter automatically sized based on benchmark data
// False positive rate: ~1% (configurable)
```

### Memory Management
```cpp
// Configure memory usage
config.max_memory_mb = 4096;        // 4GB limit
config.batch_size = 2000;           // Process in batches

// Streaming mode for very large datasets
// Processes documents without loading all into memory
```

### Parallel Processing
```cpp
// Enable multi-threading
config.enable_parallel_processing = true;

// Automatic thread count based on CPU cores
// Custom thread count can be set via environment variables
```

### Caching
```cpp
// Automatic caching of assessment results
// Avoids reprocessing identical documents
// Cache size automatically managed based on memory limits
```

## Statistical Analysis

### Contamination Metrics
```cpp
const auto& stats = filter.get_stats();

std::cout << "Contamination Rate: " << stats.get_contamination_rate() << std::endl;
std::cout << "N-gram Contamination: " << stats.get_ngram_contamination_rate() << std::endl;
std::cout << "Average Processing Time: " << stats.average_processing_time_ms << " ms" << std::endl;
```

### Per-Benchmark Analysis
```cpp
// See which benchmarks cause most contamination
for (const auto& [benchmark, count] : stats.contamination_by_dataset) {
    std::cout << benchmark << ": " << count << " contaminated documents" << std::endl;
}
```

### Performance Metrics
```cpp
std::cout << "Throughput: " << (stats.total_documents_processed / 
                                (stats.average_processing_time_ms / 1000.0)) 
          << " documents/second" << std::endl;
```

## Best Practices

### 1. Benchmark Selection
- Include all evaluation benchmarks you plan to use
- Add benchmarks from related tasks in your domain
- Consider translated versions of English benchmarks
- Include older versions of updated benchmarks

### 2. N-gram Size Selection
- **8-10 grams**: Catches more contamination, higher false positives
- **13 grams**: Good balance (recommended default)
- **16+ grams**: More precise, may miss partial contamination

### 3. Threshold Tuning
- Start with default threshold (0.1)
- Validate on known contaminated examples
- Adjust based on acceptable false positive/negative rates
- Consider domain-specific contamination patterns

### 4. Quality Control
- Manually review rejected documents for false positives
- Test on known clean datasets to check false positive rates
- Monitor contamination sources and update benchmarks accordingly
- Regular audits of filtering effectiveness

### 5. Documentation
- Document which benchmarks were used for decontamination
- Record filtering parameters and thresholds
- Maintain logs of rejected content for analysis
- Provide reproduction instructions for filtering process

## Integration Examples

### With Training Pipelines
```cpp
// Filter training data before model training
std::vector<Document> raw_training_data = load_training_data();
DecontaminationFilter decontam_filter(config);

// Load evaluation benchmarks
decontam_filter.load_benchmark_directory("eval_benchmarks/");

// Filter contaminated examples
std::vector<Document> clean_training_data;
for (const auto& doc : raw_training_data) {
    auto decision = decontam_filter.evaluate(doc);
    if (decision.result == FilterResult::KEEP) {
        clean_training_data.push_back(doc);
    }
}

// Proceed with clean training data
train_model(clean_training_data);
```

### With Other Filters
```cpp
// Multi-stage filtering pipeline
QualityFilter quality_filter(quality_config);
DecontaminationFilter decontam_filter(decontam_config);

for (const auto& doc : documents) {
    // First apply quality filtering
    auto quality_result = quality_filter.evaluate(doc);
    if (quality_result.result == FilterResult::KEEP) {
        // Then check for contamination
        auto decontam_result = decontam_filter.evaluate(doc);
        if (decontam_result.result == FilterResult::KEEP) {
            final_dataset.push_back(doc);
        }
    }
}
```

## Validation and Testing

### Effectiveness Testing
```cpp
// Test on known contaminated examples
std::vector<Document> test_contaminated = create_known_contaminated_examples();
std::vector<Document> test_clean = create_clean_examples();

// Measure precision and recall
auto contaminated_results = filter.assess_documents(test_contaminated);
auto clean_results = filter.assess_documents(test_clean);

double precision = calculate_precision(contaminated_results, clean_results);
double recall = calculate_recall(contaminated_results);
```

### Benchmark Coverage
```cpp
// Verify all benchmark n-grams are loaded
std::cout << "Loaded benchmarks: " << filter.get_benchmark_datasets().size() << std::endl;
std::cout << "Total n-grams: " << filter.get_benchmark_ngrams_count() << std::endl;

// List loaded benchmarks
for (const auto& benchmark : filter.get_benchmark_datasets()) {
    std::cout << "  - " << benchmark << std::endl;
}
```

## Advanced Topics

### Custom Benchmark Formats
Implement custom loaders for specialized benchmark formats:
```cpp
// Custom loader for domain-specific benchmarks
class CustomBenchmarkLoader {
public:
    std::vector<std::string> load_custom_format(const std::string& filename) {
        // Custom parsing logic
        return extracted_ngrams;
    }
};
```

### Distributed Processing
For very large datasets, consider distributed processing:
```cpp
// Process different shards on different machines
// Combine results for final contamination assessment
```

### Continuous Monitoring
Set up monitoring for ongoing contamination detection:
```cpp
// Regular batch processing of new training data
// Alerts when contamination rates exceed thresholds
// Automatic benchmark updates
```

## Limitations and Considerations

### Current Limitations
- Text-based detection only (no image/audio contamination)
- Requires exact or near-exact n-gram matches
- May miss sophisticated paraphrasing
- Performance scales with benchmark size

### Future Enhancements
- Semantic similarity detection using embeddings
- Multi-modal contamination detection
- Automated benchmark discovery and updates
- Integration with model training pipelines

## Research Applications

This decontamination system has been validated on major benchmarks including:
- **Language Understanding**: GLUE, SuperGLUE, XNLI
- **Question Answering**: SQuAD, Natural Questions, TriviaQA
- **Reading Comprehension**: RACE, CommonsenseQA
- **Text Generation**: HellaSwag, StoryCloze
- **Code Generation**: HumanEval, MBPP

Results show significant improvement in benchmark reliability when proper decontamination is applied to training datasets. 