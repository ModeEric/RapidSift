#pragma once

#include "quality_common.hpp"
#include <regex>

namespace rapidsift {
namespace quality {

class MetadataFilter : public QualityFilter {
private:
    MetadataFilterConfig config_;
    std::vector<std::regex> spam_keyword_regexes_;
    std::vector<std::regex> translation_indicator_regexes_;
    
    void compile_patterns();
    
public:
    MetadataFilter() = default;
    explicit MetadataFilter(const MetadataFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "MetadataFilter"; }
    void configure(const QualityConfig& config) override;
    
    // URL and domain checks
    bool is_domain_blocked(const std::string& domain) const;
    bool is_url_suspicious(const std::string& url) const;
    bool contains_spam_keywords_in_url(const std::string& url) const;
    double calculate_url_spam_score(const std::string& url) const;
    
    // Content type validation
    bool is_content_type_allowed(const std::string& content_type) const;
    bool appears_binary_content(const Document& doc) const;
    bool is_error_page_content(const Document& doc) const;
    
    // Machine translation detection
    bool appears_machine_translated(const std::string& text) const;
    double calculate_translation_confidence(const std::string& text) const;
    bool contains_translation_indicators(const std::string& text) const;
    
    // AI-generated content detection
    bool appears_ai_generated(const std::string& text) const;
    double calculate_ai_generation_score(const std::string& text) const;
    bool has_ai_patterns(const std::string& text) const;
    
    // Spam detection
    bool appears_spammy(const Document& doc) const;
    double calculate_spam_score(const Document& doc) const;
    bool contains_spam_patterns(const std::string& text) const;
    
    // Metadata validation
    bool has_valid_metadata(const Document& doc) const;
    bool is_navigation_page(const Document& doc) const;
    bool is_duplicate_content_page(const Document& doc) const;
    
    // Source quality assessment
    double assess_source_quality(const Document& doc) const;
    bool is_trusted_source(const std::string& domain) const;
    bool has_quality_indicators(const Document& doc) const;
    
    // Language and encoding checks
    bool has_consistent_encoding(const std::string& text) const;
    bool appears_correct_language(const std::string& text) const;
    double calculate_language_consistency_score(const std::string& text) const;
    
    // Configuration accessors
    const MetadataFilterConfig& get_config() const { return config_; }
    void set_config(const MetadataFilterConfig& config);
    
    // Utility methods for domain/URL management
    void add_blocked_domain(const std::string& domain);
    void remove_blocked_domain(const std::string& domain);
    void add_spam_keyword(const std::string& keyword);
    void clear_blocked_domains();
};

} // namespace quality
} // namespace rapidsift 