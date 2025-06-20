#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <functional>

namespace rapidsift {
namespace content {

// Document structure for content filtering
struct Document {
    std::string id;
    std::string text;
    std::string url;
    std::string domain;
    std::string content_type;
    std::string source_ip;
    std::unordered_map<std::string, std::string> metadata;
    
    Document() = default;
    Document(const std::string& id, const std::string& text) 
        : id(id), text(text) {}
    
    Document(const std::string& id, const std::string& text, 
             const std::string& url, const std::string& domain = "")
        : id(id), text(text), url(url), domain(domain) {}
};

// Content filter result
enum class FilterResult {
    KEEP,
    REJECT,
    SANITIZE,  // Keep but modify content
    UNKNOWN
};

// Rejection/action reasons
enum class ActionReason {
    BLOCKED_DOMAIN,
    SUSPICIOUS_URL,
    TOXICITY_HIGH,
    HATE_SPEECH,
    NSFW_CONTENT,
    PROFANITY,
    VIOLENCE,
    PII_DETECTED,
    PRIVACY_VIOLATION,
    MALICIOUS_CONTENT,
    CUSTOM
};

// Toxicity categories
enum class ToxicityCategory {
    HATE_SPEECH,
    HARASSMENT,
    RACISM,
    SEXISM,
    PROFANITY,
    VIOLENCE,
    NSFW_SEXUAL,
    NSFW_GORE,
    THREAT,
    SPAM,
    NONE
};

// PII types
enum class PIIType {
    EMAIL,
    PHONE,
    SSN,
    CREDIT_CARD,
    IP_ADDRESS,
    PHYSICAL_ADDRESS,
    PERSON_NAME,
    ORGANIZATION,
    URL_PERSONAL,
    GOVERNMENT_ID,
    MEDICAL_ID,
    FINANCIAL_INFO,
    NONE
};

// Filter decision with detailed information
struct FilterDecision {
    FilterResult result;
    ActionReason reason;
    double confidence;
    std::string details;
    std::vector<ToxicityCategory> toxicity_categories;
    std::vector<PIIType> pii_types;
    std::unordered_map<std::string, double> scores;
    
    // For SANITIZE results, store the cleaned text
    std::string sanitized_text;
    std::vector<std::string> removed_elements;
    
    FilterDecision(FilterResult r = FilterResult::UNKNOWN, 
                   ActionReason reason = ActionReason::CUSTOM,
                   double conf = 0.0, 
                   const std::string& det = "")
        : result(r), reason(reason), confidence(conf), details(det) {}
};

// Overall content assessment
struct ContentAssessment {
    Document document;
    std::vector<FilterDecision> filter_decisions;
    FilterResult final_result;
    double overall_safety_score;
    std::string final_sanitized_text;
    std::unordered_map<std::string, double> safety_scores;
    
    ContentAssessment() : final_result(FilterResult::UNKNOWN), overall_safety_score(0.0) {}
};

// Configuration structures
struct DomainFilterConfig {
    std::unordered_set<std::string> blocked_domains;
    std::unordered_set<std::string> allowed_domains;
    std::vector<std::string> blocked_url_patterns;
    std::vector<std::string> blocked_tlds;
    std::unordered_set<std::string> spam_keywords;
    bool check_domain_reputation = true;
    bool allow_unknown_domains = true;
    bool block_ip_addresses = false;
    bool check_url_shorteners = true;
};

struct ToxicityFilterConfig {
    double toxicity_threshold = 0.7;
    double hate_speech_threshold = 0.8;
    double nsfw_threshold = 0.8;
    double violence_threshold = 0.7;
    double harassment_threshold = 0.6;
    
    bool use_keyword_filter = true;
    bool use_ml_classifier = false;  // Future: ML model integration
    bool context_aware = true;
    bool medical_exception = true;   // Allow medical/anatomical terms
    bool educational_exception = true; // Allow educational content
    
    std::vector<std::string> profanity_keywords;
    std::vector<std::string> hate_keywords;
    std::vector<std::string> violence_keywords;
    std::vector<std::string> nsfw_keywords;
    std::vector<std::string> exception_contexts;
};

struct PIIFilterConfig {
    bool remove_emails = true;
    bool remove_phones = true;
    bool remove_ssn = true;
    bool remove_credit_cards = true;
    bool remove_ip_addresses = true;
    bool remove_addresses = true;
    bool remove_names = false;  // Often too aggressive
    
    bool use_placeholders = true;  // Replace with [EMAIL], [PHONE], etc.
    bool preserve_structure = true; // Keep sentence structure
    bool anonymize_instead_of_remove = true;
    
    std::string email_placeholder = "[EMAIL]";
    std::string phone_placeholder = "[PHONE]";
    std::string address_placeholder = "[ADDRESS]";
    std::string name_placeholder = "[PERSON]";
    std::string generic_placeholder = "[REDACTED]";
    
    // Custom patterns
    std::vector<std::string> custom_pii_patterns;
    std::vector<std::string> safe_domains;  // Emails from these domains are OK
};

// Main content filter configuration
struct ContentConfig {
    DomainFilterConfig domain_config;
    ToxicityFilterConfig toxicity_config;
    PIIFilterConfig pii_config;
    
    bool parallel_processing = true;
    size_t num_threads = 0; // 0 = auto-detect
    bool verbose = false;
    
    // Processing modes
    bool strict_mode = false;        // Reject on any violation
    bool sanitize_mode = true;       // Try to clean before rejecting
    bool preserve_metadata = true;   // Keep metadata in sanitized docs
    
    // Scoring weights
    double domain_weight = 2.0;
    double toxicity_weight = 3.0;
    double pii_weight = 1.5;
    
    // Overall threshold
    double rejection_threshold = 0.7;
};

// Statistics tracking
struct ContentStats {
    size_t total_processed = 0;
    size_t kept = 0;
    size_t rejected = 0;
    size_t sanitized = 0;
    
    std::unordered_map<ActionReason, size_t> action_counts;
    std::unordered_map<ToxicityCategory, size_t> toxicity_counts;
    std::unordered_map<PIIType, size_t> pii_counts;
    std::unordered_map<std::string, double> processing_times;
    
    // PII removal statistics
    size_t emails_removed = 0;
    size_t phones_removed = 0;
    size_t addresses_removed = 0;
    size_t names_removed = 0;
    
    double get_rejection_rate() const {
        return total_processed > 0 ? static_cast<double>(rejected) / total_processed : 0.0;
    }
    
    double get_sanitization_rate() const {
        return total_processed > 0 ? static_cast<double>(sanitized) / total_processed : 0.0;
    }
    
    double get_keep_rate() const {
        return total_processed > 0 ? static_cast<double>(kept) / total_processed : 0.0;
    }
};

// Progress callback
using ProgressCallback = std::function<void(size_t processed, size_t total, const ContentStats& stats)>;

// Content utilities
class ContentUtils {
public:
    // URL and domain utilities
    static std::string extract_domain(const std::string& url);
    static std::string extract_tld(const std::string& domain);
    static bool is_ip_address(const std::string& str);
    static bool is_url_shortener(const std::string& domain);
    static std::string normalize_url(const std::string& url);
    static bool is_suspicious_url_pattern(const std::string& url);
    
    // Text processing utilities
    static std::string normalize_text(const std::string& text);
    static std::vector<std::string> extract_words(const std::string& text);
    static std::string mask_sensitive_info(const std::string& text, const std::string& mask);
    static double calculate_text_similarity(const std::string& text1, const std::string& text2);
    
    // Context analysis
    static bool is_medical_context(const std::string& text);
    static bool is_educational_context(const std::string& text);
    static bool is_news_context(const std::string& text);
    static bool is_legal_context(const std::string& text);
    
    // Safety utilities
    static bool contains_excessive_caps(const std::string& text);
    static bool contains_spam_patterns(const std::string& text);
    static double calculate_readability_score(const std::string& text);
};

// Base class for all content filters
class ContentFilter {
public:
    virtual ~ContentFilter() = default;
    virtual FilterDecision evaluate(const Document& doc) const = 0;
    virtual std::string get_name() const = 0;
    virtual void configure(const ContentConfig& config) = 0;
};

// Severity levels for different violations
enum class ViolationSeverity {
    LOW,      // Warning, might keep with sanitization
    MEDIUM,   // Likely reject unless context allows
    HIGH,     // Strong reject
    CRITICAL  // Always reject
};

struct ViolationInfo {
    ActionReason reason;
    ViolationSeverity severity;
    std::string description;
    std::vector<std::string> matched_patterns;
    size_t start_pos = 0;
    size_t end_pos = 0;
};

} // namespace content
} // namespace rapidsift 