#pragma once

#include "content_common.hpp"
#include <regex>
#include <unordered_set>

namespace rapidsift {
namespace content {

class DomainFilter : public ContentFilter {
private:
    DomainFilterConfig config_;
    std::vector<std::regex> blocked_url_regexes_;
    std::unordered_set<std::string> url_shorteners_;
    std::unordered_set<std::string> malicious_tlds_;
    
    void compile_patterns();
    void initialize_known_lists();
    
public:
    DomainFilter() = default;
    explicit DomainFilter(const DomainFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "DomainFilter"; }
    void configure(const ContentConfig& config) override;
    
    // Domain validation
    bool is_domain_blocked(const std::string& domain) const;
    bool is_domain_allowed(const std::string& domain) const;
    bool is_tld_blocked(const std::string& tld) const;
    bool is_url_suspicious(const std::string& url) const;
    
    // URL analysis
    bool matches_blocked_patterns(const std::string& url) const;
    bool contains_spam_keywords(const std::string& url) const;
    bool is_url_shortener_domain(const std::string& domain) const;
    bool is_ip_based_url(const std::string& url) const;
    
    // Reputation checking
    double calculate_domain_reputation(const std::string& domain) const;
    bool is_known_malicious_domain(const std::string& domain) const;
    bool is_parked_domain(const std::string& domain) const;
    
    // URL structure analysis
    bool has_suspicious_subdomain(const std::string& url) const;
    bool has_suspicious_path(const std::string& url) const;
    bool has_suspicious_parameters(const std::string& url) const;
    size_t count_suspicious_patterns(const std::string& url) const;
    
    // Domain management
    void add_blocked_domain(const std::string& domain);
    void remove_blocked_domain(const std::string& domain);
    void add_allowed_domain(const std::string& domain);
    void load_domain_list(const std::string& filename, bool is_blocklist = true);
    void save_domain_list(const std::string& filename, bool is_blocklist = true) const;
    
    // Bulk operations
    void load_common_blocklists();
    void update_threat_intelligence();  // Future: external feeds
    
    // Configuration accessors
    const DomainFilterConfig& get_config() const { return config_; }
    void set_config(const DomainFilterConfig& config);
    
    // Statistics
    size_t get_blocked_domain_count() const { return config_.blocked_domains.size(); }
    size_t get_allowed_domain_count() const { return config_.allowed_domains.size(); }
};

} // namespace content
} // namespace rapidsift 