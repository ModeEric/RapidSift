#pragma once

#include "common.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <fstream>

namespace rapidsift {
namespace dedup {

// N-gram size for decontamination
constexpr size_t DEFAULT_NGRAM_SIZE = 13;
constexpr size_t MIN_NGRAM_SIZE = 8;
constexpr size_t MAX_NGRAM_SIZE = 50;

// Match result for decontamination
struct ContaminationMatch {
    std::string ngram;
    std::string source_dataset;
    size_t position_in_document = 0;
    size_t position_in_benchmark = 0;
    double match_confidence = 1.0;
    std::string benchmark_context;
    
    ContaminationMatch() = default;
    ContaminationMatch(const std::string& ng, const std::string& dataset, 
                      size_t doc_pos, size_t bench_pos)
        : ngram(ng), source_dataset(dataset), position_in_document(doc_pos), 
          position_in_benchmark(bench_pos) {}
};

// Decontamination assessment result
struct DecontaminationAssessment {
    bool is_contaminated = false;
    std::vector<ContaminationMatch> matches;
    double contamination_score = 0.0;
    size_t total_ngrams_checked = 0;
    size_t contaminated_ngrams = 0;
    std::string most_likely_source;
    
    double get_contamination_ratio() const {
        return total_ngrams_checked > 0 ? 
               static_cast<double>(contaminated_ngrams) / total_ngrams_checked : 0.0;
    }
};

// Configuration for decontamination
struct DecontaminationConfig {
    // N-gram settings
    size_t ngram_size = DEFAULT_NGRAM_SIZE;
    size_t min_ngram_size = MIN_NGRAM_SIZE;
    bool use_adaptive_ngram_size = false;
    
    // Matching thresholds
    double contamination_threshold = 0.1;  // Fraction of contaminated n-grams to reject
    size_t min_matches_to_reject = 1;      // Minimum number of matches to reject
    size_t max_matches_per_document = 100; // Stop checking after this many matches
    
    // Preprocessing options
    bool normalize_whitespace = true;
    bool case_insensitive = false;
    bool remove_punctuation = false;
    bool tokenize_before_ngrams = true;
    
    // Performance settings
    bool use_bloom_filter = true;
    bool enable_parallel_processing = true;
    size_t batch_size = 1000;
    size_t max_memory_mb = 2048;
    
    // Benchmark dataset paths
    std::vector<std::string> benchmark_files;
    std::vector<std::string> benchmark_directories;
    std::unordered_map<std::string, std::string> dataset_name_map;  // file -> dataset name
    
    // Output settings
    bool save_matches = true;
    bool detailed_logging = false;
    std::string matches_output_file;
    std::string contamination_log_file;
    
    // Advanced options
    bool check_approximate_matches = false;
    double approximate_match_threshold = 0.9;  // Jaccard similarity threshold
    bool exclude_common_phrases = true;
    size_t min_phrase_frequency_to_exclude = 100;
    
    DecontaminationConfig() = default;
};

// Statistics for decontamination
struct DecontaminationStats {
    size_t total_documents_processed = 0;
    size_t contaminated_documents = 0;
    size_t clean_documents = 0;
    size_t total_ngrams_checked = 0;
    size_t contaminated_ngrams_found = 0;
    
    std::unordered_map<std::string, size_t> contamination_by_dataset;
    std::unordered_map<size_t, size_t> matches_per_document_histogram;
    
    // Performance metrics
    double average_processing_time_ms = 0.0;
    size_t peak_memory_usage_mb = 0;
    size_t bloom_filter_false_positives = 0;
    
    double get_contamination_rate() const {
        return total_documents_processed > 0 ? 
               static_cast<double>(contaminated_documents) / total_documents_processed : 0.0;
    }
    
    double get_ngram_contamination_rate() const {
        return total_ngrams_checked > 0 ? 
               static_cast<double>(contaminated_ngrams_found) / total_ngrams_checked : 0.0;
    }
};

// Bloom filter for efficient n-gram lookup
class NGramBloomFilter {
public:
    NGramBloomFilter(size_t expected_elements, double false_positive_rate = 0.01);
    ~NGramBloomFilter() = default;
    
    void add(const std::string& ngram);
    bool might_contain(const std::string& ngram) const;
    void clear();
    
    size_t get_size() const { return bit_array_.size(); }
    size_t get_hash_functions() const { return num_hash_functions_; }
    double get_expected_false_positive_rate() const;
    
private:
    std::vector<bool> bit_array_;
    size_t num_hash_functions_;
    size_t hash_function_seeds_[8];  // Support up to 8 hash functions
    
    std::vector<size_t> hash(const std::string& item) const;
};

// Main decontamination filter
class DecontaminationFilter {
public:
    DecontaminationFilter() = default;
    explicit DecontaminationFilter(const DecontaminationConfig& config);
    ~DecontaminationFilter() = default;
    
    // Configuration
    void set_config(const DecontaminationConfig& config);
    const DecontaminationConfig& get_config() const { return config_; }
    
    // Benchmark loading
    void load_benchmark_datasets();
    void load_benchmark_file(const std::string& filename, const std::string& dataset_name = "");
    void load_benchmark_directory(const std::string& directory);
    void add_benchmark_ngrams(const std::vector<std::string>& ngrams, const std::string& source);
    
    // Document assessment
    DecontaminationAssessment assess_document(const Document& doc) const;
    std::vector<DecontaminationAssessment> assess_documents(const std::vector<Document>& docs) const;
    
    // Filtering interface (compatible with other filters)
    FilterDecision evaluate(const Document& doc) const;
    std::vector<FilterDecision> evaluate_batch(const std::vector<Document>& docs) const;
    
    // N-gram operations
    std::vector<std::string> extract_ngrams(const std::string& text, size_t n = 0) const;
    std::vector<ContaminationMatch> find_contaminated_ngrams(const Document& doc) const;
    
    // Statistics and reporting
    const DecontaminationStats& get_stats() const { return stats_; }
    void reset_stats();
    void print_contamination_report() const;
    void save_contamination_log(const std::string& filename) const;
    
    // Utility functions
    size_t get_benchmark_ngrams_count() const { return benchmark_ngrams_.size(); }
    std::vector<std::string> get_benchmark_datasets() const;
    bool is_loaded() const { return !benchmark_ngrams_.empty(); }
    
private:
    DecontaminationConfig config_;
    mutable DecontaminationStats stats_;
    
    // Benchmark data storage
    std::unordered_set<std::string> benchmark_ngrams_;
    std::unordered_map<std::string, std::string> ngram_to_dataset_;
    std::unordered_map<std::string, std::vector<size_t>> ngram_to_positions_;
    std::unique_ptr<NGramBloomFilter> bloom_filter_;
    
    // Performance optimization
    std::unordered_set<std::string> common_phrases_;
    mutable std::unordered_map<std::string, DecontaminationAssessment> assessment_cache_;
    
    // Helper methods
    std::string preprocess_text(const std::string& text) const;
    std::vector<std::string> tokenize(const std::string& text) const;
    std::string normalize_ngram(const std::string& ngram) const;
    bool is_common_phrase(const std::string& ngram) const;
    double calculate_contamination_score(const std::vector<ContaminationMatch>& matches, 
                                       size_t total_ngrams) const;
    void update_stats(const DecontaminationAssessment& assessment) const;
    void load_common_phrases();
    
    // File I/O helpers
    std::vector<std::string> read_lines_from_file(const std::string& filename) const;
    void save_matches_to_file(const std::vector<ContaminationMatch>& matches, 
                             const std::string& filename) const;
};

// Utility functions for decontamination
namespace decontamination_utils {
    // Configuration utilities
    DecontaminationConfig create_default_config();
    DecontaminationConfig create_strict_config();
    DecontaminationConfig create_fast_config();
    DecontaminationConfig load_config_from_json(const std::string& filename);
    void save_config_to_json(const DecontaminationConfig& config, const std::string& filename);
    
    // Benchmark dataset utilities
    std::vector<std::string> find_benchmark_files(const std::string& directory, 
                                                  const std::vector<std::string>& extensions = {".txt", ".json"});
    std::vector<std::string> load_benchmark_questions(const std::string& filename);
    std::vector<std::string> load_trivia_qa_dataset(const std::string& filename);
    std::vector<std::string> load_squad_dataset(const std::string& filename);
    std::vector<std::string> load_glue_dataset(const std::string& filename);
    
    // N-gram utilities
    std::vector<std::string> generate_ngrams(const std::string& text, size_t n, 
                                            bool normalize = true, bool tokenize = true);
    size_t optimal_ngram_size(const std::string& text);
    double calculate_jaccard_similarity(const std::vector<std::string>& ngrams1, 
                                       const std::vector<std::string>& ngrams2);
    
    // Text preprocessing
    std::string normalize_text(const std::string& text, bool case_insensitive = false, 
                              bool remove_punct = false, bool normalize_ws = true);
    std::vector<std::string> simple_tokenize(const std::string& text);
    std::string remove_common_artifacts(const std::string& text);
    
    // Performance utilities
    size_t estimate_memory_usage(size_t num_ngrams, size_t avg_ngram_length);
    size_t optimal_bloom_filter_size(size_t num_elements, double false_positive_rate);
    size_t optimal_hash_functions(size_t bloom_size, size_t num_elements);
    
    // Reporting utilities
    std::string generate_contamination_report(const DecontaminationStats& stats);
    void export_contamination_matches(const std::vector<ContaminationMatch>& matches, 
                                     const std::string& filename, const std::string& format = "csv");
    std::string format_match_for_review(const ContaminationMatch& match, const std::string& document_text);
    
    // Validation utilities
    bool validate_benchmark_dataset(const std::string& filename);
    std::vector<std::string> check_dataset_format(const std::string& filename);
    bool verify_decontamination_effectiveness(const std::vector<Document>& original_docs,
                                             const std::vector<Document>& filtered_docs,
                                             const std::vector<std::string>& benchmark_ngrams);
}

} // namespace dedup
} // namespace rapidsift 