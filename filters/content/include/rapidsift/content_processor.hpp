#pragma once

#include "content_common.hpp"
#include "domain_filter.hpp"
#include "toxicity_filter.hpp"
#include "pii_filter.hpp"
#include <memory>
#include <vector>

namespace rapidsift {
namespace content {

class ContentProcessor {
private:
    ContentConfig config_;
    ContentStats stats_;
    
    // Individual filters
    std::unique_ptr<DomainFilter> domain_filter_;
    std::unique_ptr<ToxicityFilter> toxicity_filter_;
    std::unique_ptr<PIIFilter> pii_filter_;
    
    // All filters in processing order
    std::vector<std::unique_ptr<ContentFilter>> filters_;
    
    void initialize_filters();
    void update_stats(const ContentAssessment& assessment);
    
public:
    ContentProcessor() = default;
    explicit ContentProcessor(const ContentConfig& config);
    ~ContentProcessor() = default;
    
    // Move-only semantics
    ContentProcessor(const ContentProcessor&) = delete;
    ContentProcessor& operator=(const ContentProcessor&) = delete;
    ContentProcessor(ContentProcessor&&) = default;
    ContentProcessor& operator=(ContentProcessor&&) = default;
    
    // Main processing functions
    ContentAssessment assess_document(const Document& doc) const;
    std::vector<ContentAssessment> assess_documents(const std::vector<Document>& docs, 
                                                   ProgressCallback callback = nullptr) const;
    
    // Batch processing with different strategies
    std::vector<Document> filter_documents(const std::vector<Document>& docs,
                                         ProgressCallback callback = nullptr);
    std::vector<Document> sanitize_documents(const std::vector<Document>& docs,
                                           ProgressCallback callback = nullptr);
    
    // Stream processing
    bool should_keep_document(const Document& doc);
    FilterResult evaluate_document(const Document& doc, ContentAssessment* assessment = nullptr);
    Document sanitize_document(const Document& doc);
    
    // Individual filter access
    const DomainFilter* get_domain_filter() const { return domain_filter_.get(); }
    const ToxicityFilter* get_toxicity_filter() const { return toxicity_filter_.get(); }
    const PIIFilter* get_pii_filter() const { return pii_filter_.get(); }
    
    // Configuration management
    void set_config(const ContentConfig& config);
    const ContentConfig& get_config() const { return config_; }
    
    // Statistics
    const ContentStats& get_stats() const { return stats_; }
    void reset_stats();
    void print_stats() const;
    void print_detailed_stats() const;
    
    // Custom filter management
    void add_custom_filter(std::unique_ptr<ContentFilter> filter);
    void remove_custom_filters();
    size_t get_filter_count() const { return filters_.size(); }
    
    // Content scoring
    double calculate_overall_safety_score(const std::vector<FilterDecision>& decisions) const;
    FilterResult make_final_decision(const std::vector<FilterDecision>& decisions, 
                                   double overall_score) const;
    std::string create_sanitized_text(const Document& doc, 
                                    const std::vector<FilterDecision>& decisions) const;
    
    // Utilities
    void save_config(const std::string& filename) const;
    void load_config(const std::string& filename);
    void save_stats(const std::string& filename) const;
    
    // Domain management
    void load_blocked_domains(const std::string& filename);
    void save_blocked_domains(const std::string& filename) const;
    void add_blocked_domain(const std::string& domain);
    void add_allowed_domain(const std::string& domain);
    
    // Toxicity keyword management
    void load_toxicity_keywords(const std::string& directory);
    void save_toxicity_keywords(const std::string& directory) const;
    void add_hate_keyword(const std::string& keyword);
    void add_profanity_keyword(const std::string& keyword);
    
    // PII pattern management
    void load_pii_patterns(const std::string& filename);
    void save_pii_patterns(const std::string& filename) const;
    void add_pii_pattern(const std::string& pattern, PIIType type);
    
    // Benchmark and analysis
    void benchmark_filters(const std::vector<Document>& test_docs);
    void analyze_content_patterns(const std::vector<ContentAssessment>& assessments);
    void generate_safety_report(const std::vector<ContentAssessment>& assessments,
                               const std::string& output_file) const;
    
    // Batch operations
    void process_directory(const std::string& input_dir, const std::string& output_dir);
    void process_large_file(const std::string& input_file, const std::string& output_file,
                          size_t batch_size = 1000);
    
private:
    // Parallel processing helpers
    std::vector<ContentAssessment> assess_documents_parallel(const std::vector<Document>& docs,
                                                           ProgressCallback callback) const;
    std::vector<ContentAssessment> assess_documents_sequential(const std::vector<Document>& docs,
                                                             ProgressCallback callback) const;
    
    // Processing strategies
    Document apply_strict_filtering(const Document& doc) const;
    Document apply_sanitization(const Document& doc) const;
    Document apply_balanced_filtering(const Document& doc) const;
};

// Utility functions for easy usage
namespace utils {
    // Create default configurations
    ContentConfig create_default_config();
    ContentConfig create_strict_config();
    ContentConfig create_permissive_config();
    ContentConfig create_privacy_focused_config();
    ContentConfig create_toxicity_focused_config();
    
    // Load common lists and patterns
    std::unordered_set<std::string> load_common_blocked_domains();
    std::vector<std::string> load_common_hate_keywords();
    std::vector<std::string> load_common_profanity_keywords();
    std::unordered_set<std::string> load_url_shortener_domains();
    
    // Validation helpers
    bool validate_config(const ContentConfig& config);
    std::string get_config_summary(const ContentConfig& config);
    
    // Performance optimization
    ContentConfig optimize_config_for_performance(const ContentConfig& base_config);
    ContentConfig optimize_config_for_safety(const ContentConfig& base_config);
    ContentConfig optimize_config_for_privacy(const ContentConfig& base_config);
    
    // Reporting utilities
    void export_safety_report(const ContentStats& stats, const std::string& filename);
    void export_pii_report(const ContentStats& stats, const std::string& filename);
    void export_toxicity_report(const ContentStats& stats, const std::string& filename);
    
    // Integration helpers
    ContentProcessor create_processor_from_file(const std::string& config_file);
    void setup_logging(const std::string& log_file, bool verbose = false);
}

} // namespace content
} // namespace rapidsift 