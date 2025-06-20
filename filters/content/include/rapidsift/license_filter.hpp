#pragma once

#include "content_common.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <regex>
#include <memory>

namespace rapidsift {
namespace content {

// License types we can detect
enum class LicenseType {
    UNKNOWN,
    PUBLIC_DOMAIN,
    CREATIVE_COMMONS_0,      // CC0 - Public Domain
    CREATIVE_COMMONS_BY,     // CC BY
    CREATIVE_COMMONS_BY_SA,  // CC BY-SA
    CREATIVE_COMMONS_BY_NC,  // CC BY-NC
    CREATIVE_COMMONS_BY_ND,  // CC BY-ND
    MIT,
    GPL_V2,
    GPL_V3,
    APACHE_2,
    BSD_2_CLAUSE,
    BSD_3_CLAUSE,
    PROPRIETARY,
    COPYRIGHTED,
    PAYWALLED,
    RESTRICTED
};

// Copyright detection result
struct CopyrightAssessment {
    LicenseType detected_license = LicenseType::UNKNOWN;
    bool has_copyright_notice = false;
    bool is_paywalled = false;
    bool is_from_allowed_domain = false;
    bool has_opt_out_signal = false;
    bool requires_removal = false;
    
    std::vector<std::string> copyright_notices;
    std::vector<std::string> license_indicators;
    std::string detected_license_text;
    double compliance_confidence = 0.0;
    
    CopyrightAssessment() = default;
};

// Configuration for license filtering
struct LicenseFilterConfig {
    // Domain-based filtering
    std::unordered_set<std::string> allowed_domains;
    std::unordered_set<std::string> creative_commons_domains;
    std::unordered_set<std::string> public_domain_domains;
    std::unordered_set<std::string> blocked_domains;
    std::unordered_set<std::string> paywalled_domains;
    
    // License detection patterns
    std::vector<std::regex> copyright_patterns;
    std::vector<std::regex> license_patterns;
    std::vector<std::regex> paywall_patterns;
    std::vector<std::regex> opt_out_patterns;
    
    // Allowed license types
    std::unordered_set<LicenseType> allowed_licenses;
    
    // Content type restrictions
    std::unordered_set<std::string> restricted_content_types;
    std::unordered_set<std::string> news_domains;  // Special handling for news
    
    // Opt-out and removal handling
    std::string opt_out_list_path;
    std::string removal_requests_path;
    bool enforce_robots_txt = true;
    bool respect_noindex_meta = true;
    bool check_creative_commons_meta = true;
    
    // Filtering behavior
    bool strict_mode = false;  // Reject unknown licenses
    bool allow_fair_use = false;
    bool require_explicit_permission = true;
    double confidence_threshold = 0.7;
    
    // Default constructor with sensible defaults
    LicenseFilterConfig();
};

// Statistics for license filtering
struct LicenseFilterStats {
    size_t total_processed = 0;
    size_t kept = 0;
    size_t rejected = 0;
    
    std::unordered_map<LicenseType, size_t> license_counts;
    std::unordered_map<std::string, size_t> domain_rejections;
    
    size_t copyright_notices_found = 0;
    size_t paywalled_content = 0;
    size_t opt_out_requests = 0;
    size_t removal_requests_processed = 0;
    
    double get_rejection_rate() const {
        return total_processed > 0 ? static_cast<double>(rejected) / total_processed : 0.0;
    }
    
    double get_compliance_rate() const {
        return total_processed > 0 ? static_cast<double>(kept) / total_processed : 0.0;
    }
};

// Copyright and license compliance filter
class LicenseFilter : public ContentFilter {
public:
    LicenseFilter() = default;
    explicit LicenseFilter(const LicenseFilterConfig& config);
    ~LicenseFilter() override = default;
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "LicenseFilter"; }
    void configure(const ContentConfig& config) override;
    
    // License-specific interface
    void set_license_config(const LicenseFilterConfig& config);
    const LicenseFilterConfig& get_license_config() const { return config_; }
    
    // Copyright assessment
    CopyrightAssessment assess_copyright(const Document& doc) const;
    std::vector<CopyrightAssessment> assess_documents(const std::vector<Document>& docs) const;
    
    // Domain management
    void add_allowed_domain(const std::string& domain);
    void add_blocked_domain(const std::string& domain);
    void load_domain_lists(const std::string& directory);
    void save_domain_lists(const std::string& directory) const;
    
    // License detection
    LicenseType detect_license(const std::string& text, const std::string& url = "") const;
    bool has_copyright_notice(const std::string& text) const;
    bool is_paywalled(const Document& doc) const;
    bool check_robots_txt(const std::string& domain) const;
    
    // Opt-out and removal handling
    void load_opt_out_list(const std::string& filename);
    void load_removal_requests(const std::string& filename);
    void add_opt_out_request(const std::string& domain, const std::string& reason = "");
    void add_removal_request(const std::string& content_id, const std::string& reason = "");
    bool is_opted_out(const std::string& domain) const;
    bool requires_removal(const std::string& content_id) const;
    
    // Statistics
    const LicenseFilterStats& get_stats() const { return stats_; }
    void reset_stats();
    void print_compliance_report() const;
    
    // Batch processing
    std::vector<FilterDecision> evaluate_batch(const std::vector<Document>& docs) const;
    
private:
    LicenseFilterConfig config_;
    mutable LicenseFilterStats stats_;
    
    // Opt-out and removal tracking
    std::unordered_set<std::string> opted_out_domains_;
    std::unordered_set<std::string> removal_requests_;
    std::unordered_map<std::string, std::string> opt_out_reasons_;
    std::unordered_map<std::string, std::string> removal_reasons_;
    
    // Helper methods
    bool is_domain_allowed(const std::string& domain) const;
    bool has_valid_license(LicenseType license) const;
    std::string extract_domain_from_url(const std::string& url) const;
    std::vector<std::string> find_copyright_notices(const std::string& text) const;
    std::vector<std::string> find_license_indicators(const std::string& text) const;
    bool check_meta_tags(const std::string& html) const;
    double calculate_compliance_confidence(const CopyrightAssessment& assessment) const;
    
    void update_stats(const CopyrightAssessment& assessment, FilterResult result) const;
    void initialize_default_patterns();
    void initialize_default_domains();
};

// Utility functions for license compliance
namespace license_utils {
    // License detection utilities
    LicenseType detect_creative_commons_license(const std::string& text);
    LicenseType detect_standard_license(const std::string& text);
    bool is_open_license(LicenseType license);
    bool is_commercial_license(LicenseType license);
    std::string license_type_to_string(LicenseType license);
    
    // Domain utilities
    std::unordered_set<std::string> load_common_allowed_domains();
    std::unordered_set<std::string> load_creative_commons_domains();
    std::unordered_set<std::string> load_public_domain_domains();
    std::unordered_set<std::string> load_news_domains();
    std::unordered_set<std::string> load_paywalled_domains();
    
    // Pattern utilities
    std::vector<std::regex> create_copyright_patterns();
    std::vector<std::regex> create_license_patterns();
    std::vector<std::regex> create_paywall_patterns();
    std::vector<std::regex> create_opt_out_patterns();
    
    // Robots.txt utilities
    bool check_robots_txt_allows(const std::string& domain, const std::string& user_agent = "*");
    std::string download_robots_txt(const std::string& domain);
    bool parse_robots_txt(const std::string& robots_content, const std::string& user_agent);
    
    // Configuration utilities
    LicenseFilterConfig create_strict_config();
    LicenseFilterConfig create_permissive_config();
    LicenseFilterConfig create_research_config();
    LicenseFilterConfig load_config_from_file(const std::string& filename);
    void save_config_to_file(const LicenseFilterConfig& config, const std::string& filename);
    
    // Compliance utilities
    bool validate_compliance(const CopyrightAssessment& assessment, const LicenseFilterConfig& config);
    std::string generate_compliance_report(const LicenseFilterStats& stats);
    void export_rejection_log(const std::vector<CopyrightAssessment>& assessments, 
                              const std::string& filename);
}

} // namespace content
} // namespace rapidsift 