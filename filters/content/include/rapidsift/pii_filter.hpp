#pragma once

#include "content_common.hpp"
#include <regex>
#include <unordered_map>

namespace rapidsift {
namespace content {

struct PIIMatch {
    PIIType type;
    std::string original_text;
    std::string replacement;
    size_t start_pos;
    size_t end_pos;
    double confidence;
    std::string context;
};

class PIIFilter : public ContentFilter {
private:
    PIIFilterConfig config_;
    
    // Compiled regex patterns for different PII types
    std::regex email_regex_;
    std::regex phone_regex_;
    std::regex ssn_regex_;
    std::regex credit_card_regex_;
    std::regex ip_address_regex_;
    std::regex address_regex_;
    std::vector<std::regex> name_regexes_;
    std::vector<std::regex> custom_pii_regexes_;
    
    // Named entity patterns
    std::regex person_name_regex_;
    std::regex organization_regex_;
    std::regex location_regex_;
    
    void compile_patterns();
    void initialize_default_patterns();
    
public:
    PIIFilter() = default;
    explicit PIIFilter(const PIIFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "PIIFilter"; }
    void configure(const ContentConfig& config) override;
    
    // Main PII detection and removal
    std::vector<PIIMatch> detect_pii(const std::string& text) const;
    std::string sanitize_text(const std::string& text) const;
    std::string anonymize_text(const std::string& text) const;
    
    // Specific PII type detection
    std::vector<PIIMatch> detect_emails(const std::string& text) const;
    std::vector<PIIMatch> detect_phones(const std::string& text) const;
    std::vector<PIIMatch> detect_ssn(const std::string& text) const;
    std::vector<PIIMatch> detect_credit_cards(const std::string& text) const;
    std::vector<PIIMatch> detect_ip_addresses(const std::string& text) const;
    std::vector<PIIMatch> detect_addresses(const std::string& text) const;
    std::vector<PIIMatch> detect_names(const std::string& text) const;
    
    // Advanced detection methods
    std::vector<PIIMatch> detect_government_ids(const std::string& text) const;
    std::vector<PIIMatch> detect_financial_info(const std::string& text) const;
    std::vector<PIIMatch> detect_medical_ids(const std::string& text) const;
    std::vector<PIIMatch> detect_custom_patterns(const std::string& text) const;
    
    // Validation and filtering
    bool is_valid_email(const std::string& email) const;
    bool is_valid_phone(const std::string& phone) const;
    bool is_valid_ssn(const std::string& ssn) const;
    bool is_valid_credit_card(const std::string& cc) const;
    bool is_safe_domain_email(const std::string& email) const;
    
    // Context analysis
    bool is_in_safe_context(const PIIMatch& match, const std::string& full_text) const;
    bool is_false_positive(const PIIMatch& match, const std::string& full_text) const;
    bool is_example_data(const PIIMatch& match) const;
    
    // Replacement strategies
    std::string get_replacement_text(const PIIMatch& match) const;
    std::string generate_anonymous_email() const;
    std::string generate_anonymous_phone() const;
    std::string generate_anonymous_address() const;
    
    // Bulk processing
    std::string process_document(const std::string& text, std::vector<PIIMatch>* matches = nullptr) const;
    void sanitize_document_in_place(Document& doc) const;
    
    // Pattern management
    void add_custom_pattern(const std::string& pattern, PIIType type);
    void add_safe_domain(const std::string& domain);
    void load_patterns_from_file(const std::string& filename);
    void save_patterns_to_file(const std::string& filename) const;
    
    // Statistics and reporting
    std::unordered_map<PIIType, size_t> get_detection_counts() const;
    void generate_privacy_report(const std::vector<PIIMatch>& matches) const;
    double calculate_privacy_risk_score(const std::string& text) const;
    
    // Configuration management
    const PIIFilterConfig& get_config() const { return config_; }
    void set_config(const PIIFilterConfig& config);
    
private:
    // Internal helper methods
    std::vector<PIIMatch> extract_matches(const std::string& text, 
                                         const std::regex& pattern, 
                                         PIIType type) const;
    bool is_likely_person_name(const std::string& text) const;
    bool is_organization_name(const std::string& text) const;
    std::string normalize_phone_number(const std::string& phone) const;
    std::string normalize_email(const std::string& email) const;
    bool passes_luhn_check(const std::string& number) const;  // For credit cards
    
    // Named entity recognition helpers
    bool is_proper_noun(const std::string& word) const;
    bool is_common_name(const std::string& name) const;
    bool is_place_name(const std::string& name) const;
    
    // Context analysis helpers
    std::string extract_context(const std::string& text, size_t pos, size_t window = 50) const;
    bool has_contact_context(const std::string& context) const;
    bool has_example_context(const std::string& context) const;
    bool has_test_data_context(const std::string& context) const;
};

} // namespace content
} // namespace rapidsift 