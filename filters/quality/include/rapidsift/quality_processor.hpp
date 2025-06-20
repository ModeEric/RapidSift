#pragma once

#include "quality_common.hpp"
#include "length_filter.hpp"
#include "gibberish_filter.hpp"
#include "repetition_filter.hpp"
#include "format_filter.hpp"
#include "metadata_filter.hpp"
#include <memory>
#include <vector>

namespace rapidsift {
namespace quality {

class QualityProcessor {
private:
    QualityConfig config_;
    QualityStats stats_;
    
    // Individual filters
    std::unique_ptr<LengthFilter> length_filter_;
    std::unique_ptr<GibberishFilter> gibberish_filter_;
    std::unique_ptr<RepetitionFilter> repetition_filter_;
    std::unique_ptr<FormatFilter> format_filter_;
    std::unique_ptr<MetadataFilter> metadata_filter_;
    
    // All filters in processing order
    std::vector<std::unique_ptr<QualityFilter>> filters_;
    
    void initialize_filters();
    void update_stats(const QualityAssessment& assessment);
    
public:
    QualityProcessor() = default;
    explicit QualityProcessor(const QualityConfig& config);
    ~QualityProcessor() = default;
    
    // Move-only semantics
    QualityProcessor(const QualityProcessor&) = delete;
    QualityProcessor& operator=(const QualityProcessor&) = delete;
    QualityProcessor(QualityProcessor&&) = default;
    QualityProcessor& operator=(QualityProcessor&&) = default;
    
    // Main processing functions
    QualityAssessment assess_document(const Document& doc) const;
    std::vector<QualityAssessment> assess_documents(const std::vector<Document>& docs, 
                                                   ProgressCallback callback = nullptr) const;
    
    // Batch processing with parallel support
    std::vector<Document> filter_documents(const std::vector<Document>& docs,
                                         ProgressCallback callback = nullptr);
    
    // Stream processing
    bool should_keep_document(const Document& doc);
    FilterResult evaluate_document(const Document& doc, QualityAssessment* assessment = nullptr);
    
    // Individual filter access
    const LengthFilter* get_length_filter() const { return length_filter_.get(); }
    const GibberishFilter* get_gibberish_filter() const { return gibberish_filter_.get(); }
    const RepetitionFilter* get_repetition_filter() const { return repetition_filter_.get(); }
    const FormatFilter* get_format_filter() const { return format_filter_.get(); }
    const MetadataFilter* get_metadata_filter() const { return metadata_filter_.get(); }
    
    // Configuration management
    void set_config(const QualityConfig& config);
    const QualityConfig& get_config() const { return config_; }
    
    // Statistics
    const QualityStats& get_stats() const { return stats_; }
    void reset_stats();
    void print_stats() const;
    
    // Custom filter management
    void add_custom_filter(std::unique_ptr<QualityFilter> filter);
    void remove_custom_filters();
    size_t get_filter_count() const { return filters_.size(); }
    
    // Quality scoring
    double calculate_overall_score(const std::vector<FilterDecision>& decisions) const;
    FilterResult make_final_decision(const std::vector<FilterDecision>& decisions, 
                                   double overall_score) const;
    
    // Utilities
    void save_config(const std::string& filename) const;
    void load_config(const std::string& filename);
    void save_stats(const std::string& filename) const;
    
    // Benchmark and analysis
    void benchmark_filters(const std::vector<Document>& test_docs);
    void analyze_rejection_patterns(const std::vector<QualityAssessment>& assessments);
    
private:
    // Parallel processing helpers
    std::vector<QualityAssessment> assess_documents_parallel(const std::vector<Document>& docs,
                                                           ProgressCallback callback) const;
    std::vector<QualityAssessment> assess_documents_sequential(const std::vector<Document>& docs,
                                                             ProgressCallback callback) const;
};

// Utility functions for easy usage
namespace utils {
    // Create default configurations
    QualityConfig create_default_config();
    QualityConfig create_strict_config();
    QualityConfig create_permissive_config();
    
    // Load common blocklists and patterns
    std::unordered_set<std::string> load_common_spam_domains();
    std::vector<std::string> load_common_gibberish_patterns();
    std::vector<std::string> load_common_boilerplate_patterns();
    
    // Validation helpers
    bool validate_config(const QualityConfig& config);
    std::string get_config_summary(const QualityConfig& config);
    
    // Performance optimization
    QualityConfig optimize_config_for_performance(const QualityConfig& base_config);
    QualityConfig optimize_config_for_accuracy(const QualityConfig& base_config);
}

} // namespace quality
} // namespace rapidsift 