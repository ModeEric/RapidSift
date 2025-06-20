#include "rapidsift/decontamination_filter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <random>
#include <filesystem>
#include <cmath>

namespace rapidsift {
namespace dedup {

// NGramBloomFilter Implementation
NGramBloomFilter::NGramBloomFilter(size_t expected_elements, double false_positive_rate) {
    // Calculate optimal bloom filter size
    double ln2 = std::log(2.0);
    size_t optimal_size = static_cast<size_t>(
        -expected_elements * std::log(false_positive_rate) / (ln2 * ln2));
    
    bit_array_.resize(optimal_size, false);
    
    // Calculate optimal number of hash functions
    num_hash_functions_ = std::max(1, static_cast<int>(
        (static_cast<double>(optimal_size) / expected_elements) * ln2));
    num_hash_functions_ = std::min(num_hash_functions_, size_t(8));  // Cap at 8
    
    // Initialize hash function seeds
    std::random_device rd;
    std::mt19937 gen(rd());
    for (size_t i = 0; i < num_hash_functions_; ++i) {
        hash_function_seeds_[i] = gen();
    }
}

void NGramBloomFilter::add(const std::string& ngram) {
    auto hashes = hash(ngram);
    for (size_t i = 0; i < num_hash_functions_; ++i) {
        bit_array_[hashes[i] % bit_array_.size()] = true;
    }
}

bool NGramBloomFilter::might_contain(const std::string& ngram) const {
    auto hashes = hash(ngram);
    for (size_t i = 0; i < num_hash_functions_; ++i) {
        if (!bit_array_[hashes[i] % bit_array_.size()]) {
            return false;
        }
    }
    return true;
}

void NGramBloomFilter::clear() {
    std::fill(bit_array_.begin(), bit_array_.end(), false);
}

std::vector<size_t> NGramBloomFilter::hash(const std::string& item) const {
    std::vector<size_t> hashes;
    hashes.reserve(num_hash_functions_);
    
    for (size_t i = 0; i < num_hash_functions_; ++i) {
        std::hash<std::string> hasher;
        size_t hash_value = hasher(item + std::to_string(hash_function_seeds_[i]));
        hashes.push_back(hash_value);
    }
    
    return hashes;
}

// DecontaminationFilter Implementation
DecontaminationFilter::DecontaminationFilter(const DecontaminationConfig& config) 
    : config_(config) {
    if (config_.use_bloom_filter) {
        bloom_filter_ = std::make_unique<NGramBloomFilter>(1000000, 0.01);  // 1M elements, 1% FPR
    }
    
    if (config_.exclude_common_phrases) {
        load_common_phrases();
    }
}

void DecontaminationFilter::set_config(const DecontaminationConfig& config) {
    config_ = config;
    
    // Reinitialize bloom filter if needed
    if (config_.use_bloom_filter && !bloom_filter_) {
        bloom_filter_ = std::make_unique<NGramBloomFilter>(1000000, 0.01);
        // Repopulate with existing n-grams
        for (const auto& ngram : benchmark_ngrams_) {
            bloom_filter_->add(ngram);
        }
    }
}

void DecontaminationFilter::load_benchmark_datasets() {
    std::cout << "Loading benchmark datasets..." << std::endl;
    
    // Load individual files
    for (const auto& filename : config_.benchmark_files) {
        std::string dataset_name = config_.dataset_name_map.count(filename) ? 
                                   config_.dataset_name_map.at(filename) : 
                                   std::filesystem::path(filename).stem().string();
        load_benchmark_file(filename, dataset_name);
    }
    
    // Load directories
    for (const auto& directory : config_.benchmark_directories) {
        load_benchmark_directory(directory);
    }
    
    std::cout << "Loaded " << benchmark_ngrams_.size() << " benchmark n-grams from " 
              << get_benchmark_datasets().size() << " datasets" << std::endl;
}

void DecontaminationFilter::load_benchmark_file(const std::string& filename, const std::string& dataset_name) {
    std::cout << "Loading benchmark file: " << filename << std::endl;
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Cannot open benchmark file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    size_t line_count = 0;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        // Extract n-grams from this line
        auto ngrams = extract_ngrams(line, config_.ngram_size);
        
        for (const auto& ngram : ngrams) {
            benchmark_ngrams_.insert(ngram);
            ngram_to_dataset_[ngram] = dataset_name.empty() ? filename : dataset_name;
            
            if (bloom_filter_) {
                bloom_filter_->add(ngram);
            }
        }
        
        line_count++;
        if (line_count % 1000 == 0) {
            std::cout << "  Processed " << line_count << " lines from " << filename << std::endl;
        }
    }
    
    std::cout << "Loaded " << line_count << " lines from " << filename << std::endl;
}

void DecontaminationFilter::load_benchmark_directory(const std::string& directory) {
    std::cout << "Loading benchmark directory: " << directory << std::endl;
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().string();
            std::string extension = entry.path().extension().string();
            
            // Only process text files
            if (extension == ".txt" || extension == ".json" || extension == ".csv") {
                std::string dataset_name = entry.path().stem().string();
                load_benchmark_file(filename, dataset_name);
            }
        }
    }
}

DecontaminationAssessment DecontaminationFilter::assess_document(const Document& doc) const {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    DecontaminationAssessment assessment;
    
    // Check cache first
    if (assessment_cache_.count(doc.id)) {
        return assessment_cache_[doc.id];
    }
    
    // Extract n-grams from document
    auto document_ngrams = extract_ngrams(doc.text, config_.ngram_size);
    assessment.total_ngrams_checked = document_ngrams.size();
    
    // Find contaminated n-grams
    assessment.matches = find_contaminated_ngrams(doc);
    assessment.contaminated_ngrams = assessment.matches.size();
    
    // Calculate contamination score
    assessment.contamination_score = calculate_contamination_score(
        assessment.matches, assessment.total_ngrams_checked);
    
    // Determine if document is contaminated
    assessment.is_contaminated = assessment.contamination_score >= config_.contamination_threshold;
    
    // Find most likely source
    std::unordered_map<std::string, size_t> source_counts;
    for (const auto& match : assessment.matches) {
        source_counts[match.source_dataset]++;
    }
    
    if (!source_counts.empty()) {
        auto max_source = std::max_element(source_counts.begin(), source_counts.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        assessment.most_likely_source = max_source->first;
    }
    
    // Cache the result
    assessment_cache_[doc.id] = assessment;
    
    // Update statistics
    update_stats(assessment);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    stats_.average_processing_time_ms = 
        (stats_.average_processing_time_ms * (stats_.total_documents_processed - 1) + duration.count()) / 
        stats_.total_documents_processed;
    
    return assessment;
}

std::vector<DecontaminationAssessment> DecontaminationFilter::assess_documents(const std::vector<Document>& docs) const {
    std::vector<DecontaminationAssessment> assessments;
    assessments.reserve(docs.size());
    
    for (const auto& doc : docs) {
        assessments.push_back(assess_document(doc));
    }
    
    return assessments;
}

FilterDecision DecontaminationFilter::evaluate(const Document& doc) const {
    auto assessment = assess_document(doc);
    
    FilterDecision decision;
    decision.filter_name = "DecontaminationFilter";
    decision.confidence = assessment.contamination_score;
    decision.result = assessment.is_contaminated ? FilterResult::REJECT : FilterResult::KEEP;
    
    if (assessment.is_contaminated) {
        decision.details = "Document contaminated with " + std::to_string(assessment.contaminated_ngrams) + 
                          " n-grams from " + assessment.most_likely_source;
    } else {
        decision.details = "No contamination detected";
    }
    
    return decision;
}

std::vector<std::string> DecontaminationFilter::extract_ngrams(const std::string& text, size_t n) const {
    if (n == 0) n = config_.ngram_size;
    
    std::vector<std::string> ngrams;
    std::string processed_text = preprocess_text(text);
    
    if (config_.tokenize_before_ngrams) {
        auto tokens = tokenize(processed_text);
        
        if (tokens.size() < n) return ngrams;
        
        for (size_t i = 0; i <= tokens.size() - n; ++i) {
            std::string ngram;
            for (size_t j = 0; j < n; ++j) {
                if (j > 0) ngram += " ";
                ngram += tokens[i + j];
            }
            ngrams.push_back(normalize_ngram(ngram));
        }
    } else {
        // Character-level n-grams
        if (processed_text.length() < n) return ngrams;
        
        for (size_t i = 0; i <= processed_text.length() - n; ++i) {
            std::string ngram = processed_text.substr(i, n);
            ngrams.push_back(normalize_ngram(ngram));
        }
    }
    
    return ngrams;
}

std::vector<ContaminationMatch> DecontaminationFilter::find_contaminated_ngrams(const Document& doc) const {
    std::vector<ContaminationMatch> matches;
    auto document_ngrams = extract_ngrams(doc.text, config_.ngram_size);
    
    for (size_t i = 0; i < document_ngrams.size(); ++i) {
        const auto& ngram = document_ngrams[i];
        
        // Skip common phrases if configured
        if (config_.exclude_common_phrases && is_common_phrase(ngram)) {
            continue;
        }
        
        // Check bloom filter first for efficiency
        if (bloom_filter_ && !bloom_filter_->might_contain(ngram)) {
            continue;
        }
        
        // Check if n-gram exists in benchmark
        if (benchmark_ngrams_.count(ngram)) {
            ContaminationMatch match;
            match.ngram = ngram;
            match.position_in_document = i;
            match.source_dataset = ngram_to_dataset_.count(ngram) ? 
                                   ngram_to_dataset_.at(ngram) : "unknown";
            matches.push_back(match);
            
            // Stop if we've found too many matches
            if (matches.size() >= config_.max_matches_per_document) {
                break;
            }
        }
    }
    
    return matches;
}

void DecontaminationFilter::reset_stats() {
    stats_ = DecontaminationStats{};
}

void DecontaminationFilter::print_contamination_report() const {
    std::cout << "\n=== Decontamination Report ===\n";
    std::cout << "Total documents processed: " << stats_.total_documents_processed << "\n";
    std::cout << "Contaminated documents: " << stats_.contaminated_documents 
              << " (" << (stats_.get_contamination_rate() * 100) << "%)\n";
    std::cout << "Clean documents: " << stats_.clean_documents << "\n";
    std::cout << "Total n-grams checked: " << stats_.total_ngrams_checked << "\n";
    std::cout << "Contaminated n-grams found: " << stats_.contaminated_ngrams_found 
              << " (" << (stats_.get_ngram_contamination_rate() * 100) << "%)\n";
    std::cout << "Average processing time: " << stats_.average_processing_time_ms << " ms\n";
    
    if (!stats_.contamination_by_dataset.empty()) {
        std::cout << "\nContamination by dataset:\n";
        for (const auto& [dataset, count] : stats_.contamination_by_dataset) {
            std::cout << "  " << dataset << ": " << count << " documents\n";
        }
    }
    
    if (bloom_filter_) {
        std::cout << "\nBloom filter stats:\n";
        std::cout << "  Size: " << bloom_filter_->get_size() << " bits\n";
        std::cout << "  Hash functions: " << bloom_filter_->get_hash_functions() << "\n";
        std::cout << "  False positives: " << stats_.bloom_filter_false_positives << "\n";
    }
}

std::vector<std::string> DecontaminationFilter::get_benchmark_datasets() const {
    std::unordered_set<std::string> datasets;
    for (const auto& [ngram, dataset] : ngram_to_dataset_) {
        datasets.insert(dataset);
    }
    return std::vector<std::string>(datasets.begin(), datasets.end());
}

// Private helper methods
std::string DecontaminationFilter::preprocess_text(const std::string& text) const {
    std::string result = text;
    
    if (config_.normalize_whitespace) {
        // Replace multiple whitespace with single space
        result = std::regex_replace(result, std::regex("\\s+"), " ");
    }
    
    if (config_.case_insensitive) {
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    }
    
    if (config_.remove_punctuation) {
        result.erase(std::remove_if(result.begin(), result.end(), ::ispunct), result.end());
    }
    
    return result;
}

std::vector<std::string> DecontaminationFilter::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string DecontaminationFilter::normalize_ngram(const std::string& ngram) const {
    std::string result = ngram;
    
    // Trim whitespace
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    
    return result;
}

bool DecontaminationFilter::is_common_phrase(const std::string& ngram) const {
    return common_phrases_.count(ngram) > 0;
}

double DecontaminationFilter::calculate_contamination_score(const std::vector<ContaminationMatch>& matches, 
                                                          size_t total_ngrams) const {
    if (total_ngrams == 0) return 0.0;
    return static_cast<double>(matches.size()) / total_ngrams;
}

void DecontaminationFilter::update_stats(const DecontaminationAssessment& assessment) const {
    stats_.total_documents_processed++;
    stats_.total_ngrams_checked += assessment.total_ngrams_checked;
    stats_.contaminated_ngrams_found += assessment.contaminated_ngrams;
    
    if (assessment.is_contaminated) {
        stats_.contaminated_documents++;
        if (!assessment.most_likely_source.empty()) {
            stats_.contamination_by_dataset[assessment.most_likely_source]++;
        }
    } else {
        stats_.clean_documents++;
    }
    
    // Update histogram
    stats_.matches_per_document_histogram[assessment.contaminated_ngrams]++;
}

void DecontaminationFilter::load_common_phrases() {
    // Load common phrases that should be excluded from contamination detection
    // These are phrases that are too common to be meaningful contamination indicators
    common_phrases_ = {
        "the", "of", "and", "to", "a", "in", "is", "it", "you", "that",
        "he", "was", "for", "on", "are", "as", "with", "his", "they", "i",
        "at", "be", "this", "have", "from", "or", "one", "had", "by", "word",
        "what", "all", "were", "we", "when", "your", "can", "said", "there",
        "each", "which", "she", "do", "how", "their", "if", "will", "up",
        "other", "about", "out", "many", "then", "them", "these", "so", "some"
    };
}

// Utility function implementations
namespace decontamination_utils {

DecontaminationConfig create_default_config() {
    DecontaminationConfig config;
    config.ngram_size = DEFAULT_NGRAM_SIZE;
    config.contamination_threshold = 0.1;
    config.use_bloom_filter = true;
    config.enable_parallel_processing = true;
    config.normalize_whitespace = true;
    config.tokenize_before_ngrams = true;
    config.exclude_common_phrases = true;
    return config;
}

DecontaminationConfig create_strict_config() {
    DecontaminationConfig config = create_default_config();
    config.contamination_threshold = 0.01;  // Very low threshold
    config.min_matches_to_reject = 1;       // Reject on any match
    config.ngram_size = 8;                  // Smaller n-grams catch more
    config.check_approximate_matches = true;
    return config;
}

DecontaminationConfig create_fast_config() {
    DecontaminationConfig config = create_default_config();
    config.contamination_threshold = 0.2;   // Higher threshold
    config.use_bloom_filter = true;         // Fast filtering
    config.max_matches_per_document = 10;   // Stop early
    config.exclude_common_phrases = true;   // Skip common phrases
    config.detailed_logging = false;        // Less overhead
    return config;
}

std::vector<std::string> generate_ngrams(const std::string& text, size_t n, 
                                        bool normalize, bool tokenize) {
    std::vector<std::string> ngrams;
    std::string processed_text = text;
    
    if (normalize) {
        processed_text = normalize_text(processed_text);
    }
    
    if (tokenize) {
        auto tokens = simple_tokenize(processed_text);
        if (tokens.size() < n) return ngrams;
        
        for (size_t i = 0; i <= tokens.size() - n; ++i) {
            std::string ngram;
            for (size_t j = 0; j < n; ++j) {
                if (j > 0) ngram += " ";
                ngram += tokens[i + j];
            }
            ngrams.push_back(ngram);
        }
    } else {
        if (processed_text.length() < n) return ngrams;
        
        for (size_t i = 0; i <= processed_text.length() - n; ++i) {
            ngrams.push_back(processed_text.substr(i, n));
        }
    }
    
    return ngrams;
}

std::string normalize_text(const std::string& text, bool case_insensitive, 
                          bool remove_punct, bool normalize_ws) {
    std::string result = text;
    
    if (case_insensitive) {
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    }
    
    if (remove_punct) {
        result.erase(std::remove_if(result.begin(), result.end(), ::ispunct), result.end());
    }
    
    if (normalize_ws) {
        result = std::regex_replace(result, std::regex("\\s+"), " ");
    }
    
    return result;
}

std::vector<std::string> simple_tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string generate_contamination_report(const DecontaminationStats& stats) {
    std::ostringstream report;
    
    report << "=== Decontamination Summary Report ===\n";
    report << "Total Documents Processed: " << stats.total_documents_processed << "\n";
    report << "Contaminated Documents: " << stats.contaminated_documents 
           << " (" << (stats.get_contamination_rate() * 100) << "%)\n";
    report << "Clean Documents: " << stats.clean_documents << "\n";
    report << "Total N-grams Checked: " << stats.total_ngrams_checked << "\n";
    report << "Contaminated N-grams Found: " << stats.contaminated_ngrams_found 
           << " (" << (stats.get_ngram_contamination_rate() * 100) << "%)\n";
    report << "Average Processing Time: " << stats.average_processing_time_ms << " ms per document\n";
    
    if (!stats.contamination_by_dataset.empty()) {
        report << "\nContamination by Dataset:\n";
        for (const auto& [dataset, count] : stats.contamination_by_dataset) {
            report << "  " << dataset << ": " << count << " documents\n";
        }
    }
    
    return report.str();
}

} // namespace decontamination_utils

} // namespace dedup
} // namespace rapidsift 