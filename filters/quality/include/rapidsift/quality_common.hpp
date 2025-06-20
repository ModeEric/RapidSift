#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <functional>

namespace rapidsift {
namespace quality {

// Document structure for quality filtering
struct Document {
    std::string id;
    std::string text;
    std::string url;
    std::string domain;
    std::string content_type;
    std::unordered_map<std::string, std::string> metadata;
    
    Document() = default;
    Document(const std::string& id, const std::string& text) 
        : id(id), text(text) {}
    
    Document(const std::string& id, const std::string& text, 
             const std::string& url, const std::string& domain = "")
        : id(id), text(text), url(url), domain(domain) {}
};

// Quality filter result
enum class FilterResult {
    KEEP,
    REJECT,
    UNKNOWN
};

// Rejection reasons
enum class RejectionReason {
    TOO_SHORT,
    TOO_LONG,
    GIBBERISH,
    HIGH_REPETITION,
    BOILERPLATE,
    POOR_FORMATTING,
    MACHINE_GENERATED,
    SUSPICIOUS_URL,
    INVALID_METADATA,
    CUSTOM
};

// Filter decision with detailed information
struct FilterDecision {
    FilterResult result;
    RejectionReason reason;
    double confidence;
    std::string details;
    std::unordered_map<std::string, double> metrics;
    
    FilterDecision(FilterResult r = FilterResult::UNKNOWN, 
                   RejectionReason reason = RejectionReason::CUSTOM,
                   double conf = 0.0, 
                   const std::string& det = "")
        : result(r), reason(reason), confidence(conf), details(det) {}
};

// Overall quality assessment
struct QualityAssessment {
    Document document;
    std::vector<FilterDecision> filter_decisions;
    FilterResult final_result;
    double overall_score;
    std::unordered_map<std::string, double> feature_scores;
    
    QualityAssessment() : final_result(FilterResult::UNKNOWN), overall_score(0.0) {}
};

// Configuration for different filters
struct LengthFilterConfig {
    size_t min_words = 5;
    size_t max_words = 1000000;
    size_t min_chars = 20;
    size_t max_chars = 10000000;
    bool strict_mode = false;
};

struct GibberishFilterConfig {
    double max_non_alpha_ratio = 0.3;
    double max_digit_ratio = 0.5;
    double max_symbol_ratio = 0.2;
    double max_repetition_ratio = 0.3;
    size_t max_consecutive_chars = 50;
    double min_entropy = 2.0;
    std::vector<std::string> gibberish_patterns;
};

struct RepetitionFilterConfig {
    double max_line_repetition_ratio = 0.3;
    double max_ngram_repetition_ratio = 0.5;
    size_t min_unique_words = 10;
    double min_unique_word_ratio = 0.3;
    size_t ngram_size = 3;
    double max_boilerplate_ratio = 0.7;
};

struct FormatFilterConfig {
    double max_html_ratio = 0.1;
    double max_code_ratio = 0.2;
    double max_single_line_ratio = 0.8;
    bool allow_lists = true;
    bool allow_poetry = false;
    std::vector<std::string> unwanted_patterns;
};

struct MetadataFilterConfig {
    std::unordered_set<std::string> blocked_domains;
    std::unordered_set<std::string> spam_keywords;
    std::unordered_set<std::string> allowed_content_types;
    std::vector<std::string> machine_translation_indicators;
    double max_translation_confidence = 0.8;
};

// Main quality filter configuration
struct QualityConfig {
    LengthFilterConfig length_config;
    GibberishFilterConfig gibberish_config;
    RepetitionFilterConfig repetition_config;
    FormatFilterConfig format_config;
    MetadataFilterConfig metadata_config;
    
    bool parallel_processing = true;
    size_t num_threads = 0; // 0 = auto-detect
    bool verbose = false;
    
    // Scoring weights
    double length_weight = 1.0;
    double gibberish_weight = 2.0;
    double repetition_weight = 1.5;
    double format_weight = 1.0;
    double metadata_weight = 1.2;
    
    // Overall threshold
    double rejection_threshold = 0.5;
};

// Statistics tracking
struct QualityStats {
    size_t total_processed = 0;
    size_t kept = 0;
    size_t rejected = 0;
    
    std::unordered_map<RejectionReason, size_t> rejection_counts;
    std::unordered_map<std::string, double> processing_times;
    
    double get_rejection_rate() const {
        return total_processed > 0 ? static_cast<double>(rejected) / total_processed : 0.0;
    }
    
    double get_keep_rate() const {
        return total_processed > 0 ? static_cast<double>(kept) / total_processed : 0.0;
    }
};

// Progress callback
using ProgressCallback = std::function<void(size_t processed, size_t total, const QualityStats& stats)>;

// Text utilities
class TextUtils {
public:
    static std::vector<std::string> split_lines(const std::string& text);
    static std::vector<std::string> split_words(const std::string& text);
    static std::string normalize_whitespace(const std::string& text);
    static std::string strip_html(const std::string& text);
    static double calculate_entropy(const std::string& text);
    static double calculate_perplexity(const std::string& text);
    static bool is_ascii(const std::string& text);
    static double get_alpha_ratio(const std::string& text);
    static double get_digit_ratio(const std::string& text);
    static double get_symbol_ratio(const std::string& text);
    static size_t count_consecutive_chars(const std::string& text, char c);
    static std::string extract_domain(const std::string& url);
    static bool contains_code_patterns(const std::string& text);
    static bool is_list_like(const std::string& text);
};

// Base class for all quality filters
class QualityFilter {
public:
    virtual ~QualityFilter() = default;
    virtual FilterDecision evaluate(const Document& doc) const = 0;
    virtual std::string get_name() const = 0;
    virtual void configure(const QualityConfig& config) = 0;
};

} // namespace quality
} // namespace rapidsift 