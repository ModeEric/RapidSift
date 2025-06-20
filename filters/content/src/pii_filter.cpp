#include "rapidsift/pii_filter.hpp"
#include "rapidsift/content_common.hpp"
#include <algorithm>
#include <random>
#include <iomanip>
#include <sstream>

namespace rapidsift {
namespace content {

PIIFilter::PIIFilter(const PIIFilterConfig& config) : config_(config) {
    compile_patterns();
    initialize_default_patterns();
}

void PIIFilter::configure(const ContentConfig& config) {
    config_ = config.pii_config;
    compile_patterns();
    initialize_default_patterns();
}

void PIIFilter::compile_patterns() {
    // Email pattern (comprehensive)
    email_regex_ = std::regex(
        R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b)",
        std::regex_constants::icase
    );
    
    // Phone patterns (various formats)
    phone_regex_ = std::regex(
        R"(\b(?:\+?1[-.\s]?)?\(?([0-9]{3})\)?[-.\s]?([0-9]{3})[-.\s]?([0-9]{4})\b|"
        R"(\b\d{3}-\d{3}-\d{4}\b|\b\(\d{3}\)\s?\d{3}-\d{4}\b)",
        std::regex_constants::icase
    );
    
    // SSN pattern
    ssn_regex_ = std::regex(
        R"(\b\d{3}-\d{2}-\d{4}\b|\b\d{9}\b)",
        std::regex_constants::icase
    );
    
    // Credit card pattern (basic)
    credit_card_regex_ = std::regex(
        R"(\b(?:4[0-9]{12}(?:[0-9]{3})?|5[1-5][0-9]{14}|3[47][0-9]{13}|3[0-9]{13}|6(?:011|5[0-9]{2})[0-9]{12})\b)"
    );
    
    // IP address pattern
    ip_address_regex_ = std::regex(
        R"(\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b)"
    );
    
    // Address pattern (basic)
    address_regex_ = std::regex(
        R"(\b\d+\s+[A-Za-z\s]+(?:Street|St|Avenue|Ave|Road|Rd|Boulevard|Blvd|Lane|Ln|Drive|Dr|Court|Ct|Place|Pl)\b)",
        std::regex_constants::icase
    );
    
    // Person name pattern (basic - proper nouns)
    person_name_regex_ = std::regex(
        R"(\b[A-Z][a-z]+\s+[A-Z][a-z]+\b)"
    );
    
    // Organization pattern
    organization_regex_ = std::regex(
        R"(\b[A-Z][A-Za-z\s&]+(?:Inc|Corp|LLC|Ltd|Company|Co|Corporation|Incorporated)\b)",
        std::regex_constants::icase
    );
}

void PIIFilter::initialize_default_patterns() {
    // Add default safe domains for emails
    config_.safe_domains.insert({
        "example.com", "test.com", "sample.org", "demo.net",
        "placeholder.edu", "noreply.com", "donotreply.com"
    });
}

void PIIFilter::set_config(const PIIFilterConfig& config) {
    config_ = config;
    compile_patterns();
    initialize_default_patterns();
}

FilterDecision PIIFilter::evaluate(const Document& doc) const {
    FilterDecision decision(FilterResult::KEEP, ActionReason::CUSTOM, 1.0, "");
    
    std::vector<PIIMatch> pii_matches = detect_pii(doc.text);
    
    // Store metrics
    decision.scores["pii_count"] = static_cast<double>(pii_matches.size());
    decision.scores["privacy_risk"] = calculate_privacy_risk_score(doc.text);
    
    // Categorize PII types found
    std::unordered_map<PIIType, size_t> pii_counts;
    for (const auto& match : pii_matches) {
        pii_counts[match.type]++;
        decision.pii_types.push_back(match.type);
    }
    
    if (pii_matches.empty()) {
        decision.details = "No PII detected";
        return decision;
    }
    
    // Determine action based on PII types and configuration
    bool has_sensitive_pii = false;
    std::vector<std::string> pii_descriptions;
    
    for (const auto& pair : pii_counts) {
        std::string pii_name;
        bool is_sensitive = false;
        
        switch (pair.first) {
            case PIIType::EMAIL:
                pii_name = "emails";
                is_sensitive = config_.remove_emails;
                break;
            case PIIType::PHONE:
                pii_name = "phone numbers";
                is_sensitive = config_.remove_phones;
                break;
            case PIIType::SSN:
                pii_name = "social security numbers";
                is_sensitive = config_.remove_ssn;
                break;
            case PIIType::CREDIT_CARD:
                pii_name = "credit card numbers";
                is_sensitive = config_.remove_credit_cards;
                break;
            case PIIType::IP_ADDRESS:
                pii_name = "IP addresses";
                is_sensitive = config_.remove_ip_addresses;
                break;
            case PIIType::PHYSICAL_ADDRESS:
                pii_name = "physical addresses";
                is_sensitive = config_.remove_addresses;
                break;
            case PIIType::PERSON_NAME:
                pii_name = "person names";
                is_sensitive = config_.remove_names;
                break;
            default:
                pii_name = "unknown PII";
                is_sensitive = true;
                break;
        }
        
        pii_descriptions.push_back(std::to_string(pair.second) + " " + pii_name);
        if (is_sensitive) has_sensitive_pii = true;
    }
    
    if (has_sensitive_pii) {
        if (config_.use_placeholders || config_.anonymize_instead_of_remove) {
            decision.result = FilterResult::SANITIZE;
            decision.reason = ActionReason::PII_DETECTED;
            decision.sanitized_text = sanitize_text(doc.text);
            
            // Track what was removed
            for (const auto& match : pii_matches) {
                decision.removed_elements.push_back(match.original_text + " -> " + match.replacement);
            }
        } else {
            decision.result = FilterResult::REJECT;
            decision.reason = ActionReason::PRIVACY_VIOLATION;
        }
        
        decision.confidence = 0.9;
        decision.details = "PII detected: ";
        for (size_t i = 0; i < pii_descriptions.size(); ++i) {
            if (i > 0) decision.details += ", ";
            decision.details += pii_descriptions[i];
        }
    } else {
        // PII found but not configured to be removed
        decision.details = "PII detected but not filtered: ";
        for (size_t i = 0; i < pii_descriptions.size(); ++i) {
            if (i > 0) decision.details += ", ";
            decision.details += pii_descriptions[i];
        }
    }
    
    return decision;
}

std::vector<PIIMatch> PIIFilter::detect_pii(const std::string& text) const {
    std::vector<PIIMatch> all_matches;
    
    // Detect each type of PII
    auto email_matches = detect_emails(text);
    auto phone_matches = detect_phones(text);
    auto ssn_matches = detect_ssn(text);
    auto cc_matches = detect_credit_cards(text);
    auto ip_matches = detect_ip_addresses(text);
    auto address_matches = detect_addresses(text);
    auto name_matches = detect_names(text);
    
    // Combine all matches
    all_matches.insert(all_matches.end(), email_matches.begin(), email_matches.end());
    all_matches.insert(all_matches.end(), phone_matches.begin(), phone_matches.end());
    all_matches.insert(all_matches.end(), ssn_matches.begin(), ssn_matches.end());
    all_matches.insert(all_matches.end(), cc_matches.begin(), cc_matches.end());
    all_matches.insert(all_matches.end(), ip_matches.begin(), ip_matches.end());
    all_matches.insert(all_matches.end(), address_matches.begin(), address_matches.end());
    all_matches.insert(all_matches.end(), name_matches.begin(), name_matches.end());
    
    // Sort by position for proper replacement
    std::sort(all_matches.begin(), all_matches.end(), 
              [](const PIIMatch& a, const PIIMatch& b) {
                  return a.start_pos < b.start_pos;
              });
    
    // Filter out false positives and overlaps
    std::vector<PIIMatch> filtered_matches;
    for (const auto& match : all_matches) {
        if (!is_false_positive(match, text) && !is_in_safe_context(match, text)) {
            // Check for overlaps with existing matches
            bool overlaps = false;
            for (const auto& existing : filtered_matches) {
                if (match.start_pos < existing.end_pos && match.end_pos > existing.start_pos) {
                    overlaps = true;
                    break;
                }
            }
            if (!overlaps) {
                filtered_matches.push_back(match);
            }
        }
    }
    
    return filtered_matches;
}

std::string PIIFilter::sanitize_text(const std::string& text) const {
    auto matches = detect_pii(text);
    if (matches.empty()) return text;
    
    std::string result = text;
    
    // Process matches in reverse order to maintain positions
    for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
        const auto& match = *it;
        std::string replacement = get_replacement_text(match);
        result.replace(match.start_pos, match.end_pos - match.start_pos, replacement);
    }
    
    return result;
}

std::string PIIFilter::anonymize_text(const std::string& text) const {
    return sanitize_text(text); // Same implementation for now
}

std::vector<PIIMatch> PIIFilter::detect_emails(const std::string& text) const {
    return extract_matches(text, email_regex_, PIIType::EMAIL);
}

std::vector<PIIMatch> PIIFilter::detect_phones(const std::string& text) const {
    return extract_matches(text, phone_regex_, PIIType::PHONE);
}

std::vector<PIIMatch> PIIFilter::detect_ssn(const std::string& text) const {
    return extract_matches(text, ssn_regex_, PIIType::SSN);
}

std::vector<PIIMatch> PIIFilter::detect_credit_cards(const std::string& text) const {
    auto matches = extract_matches(text, credit_card_regex_, PIIType::CREDIT_CARD);
    
    // Validate credit cards using Luhn algorithm
    std::vector<PIIMatch> valid_matches;
    for (const auto& match : matches) {
        if (passes_luhn_check(match.original_text)) {
            valid_matches.push_back(match);
        }
    }
    
    return valid_matches;
}

std::vector<PIIMatch> PIIFilter::detect_ip_addresses(const std::string& text) const {
    return extract_matches(text, ip_address_regex_, PIIType::IP_ADDRESS);
}

std::vector<PIIMatch> PIIFilter::detect_addresses(const std::string& text) const {
    return extract_matches(text, address_regex_, PIIType::PHYSICAL_ADDRESS);
}

std::vector<PIIMatch> PIIFilter::detect_names(const std::string& text) const {
    if (!config_.remove_names) return {};
    
    auto matches = extract_matches(text, person_name_regex_, PIIType::PERSON_NAME);
    
    // Filter to likely person names
    std::vector<PIIMatch> name_matches;
    for (const auto& match : matches) {
        if (is_likely_person_name(match.original_text)) {
            name_matches.push_back(match);
        }
    }
    
    return name_matches;
}

std::vector<PIIMatch> PIIFilter::extract_matches(const std::string& text, 
                                                const std::regex& pattern, 
                                                PIIType type) const {
    std::vector<PIIMatch> matches;
    std::sregex_iterator iter(text.begin(), text.end(), pattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        const std::smatch& match = *iter;
        PIIMatch pii_match;
        pii_match.type = type;
        pii_match.original_text = match.str();
        pii_match.start_pos = match.position();
        pii_match.end_pos = match.position() + match.length();
        pii_match.confidence = 0.8; // Default confidence
        pii_match.context = extract_context(text, pii_match.start_pos);
        pii_match.replacement = get_replacement_text(pii_match);
        
        matches.push_back(pii_match);
    }
    
    return matches;
}

bool PIIFilter::is_valid_email(const std::string& email) const {
    // Basic validation - more sophisticated checks could be added
    return email.find('@') != std::string::npos && 
           email.find('.') != std::string::npos &&
           email.length() >= 5;
}

bool PIIFilter::is_safe_domain_email(const std::string& email) const {
    size_t at_pos = email.find('@');
    if (at_pos == std::string::npos) return false;
    
    std::string domain = email.substr(at_pos + 1);
    return config_.safe_domains.count(domain) > 0;
}

bool PIIFilter::is_in_safe_context(const PIIMatch& match, const std::string& full_text) const {
    return has_example_context(match.context) || 
           has_test_data_context(match.context) ||
           (match.type == PIIType::EMAIL && is_safe_domain_email(match.original_text));
}

bool PIIFilter::is_false_positive(const PIIMatch& match, const std::string& full_text) const {
    // Check if this is example data
    if (is_example_data(match)) return true;
    
    // Type-specific false positive checks
    switch (match.type) {
        case PIIType::EMAIL:
            return !is_valid_email(match.original_text);
        case PIIType::CREDIT_CARD:
            return !passes_luhn_check(match.original_text);
        case PIIType::PERSON_NAME:
            return !is_likely_person_name(match.original_text);
        default:
            return false;
    }
}

bool PIIFilter::is_example_data(const PIIMatch& match) const {
    std::string lower_text = match.original_text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    return lower_text.find("example") != std::string::npos ||
           lower_text.find("test") != std::string::npos ||
           lower_text.find("sample") != std::string::npos ||
           lower_text.find("placeholder") != std::string::npos ||
           lower_text.find("dummy") != std::string::npos;
}

std::string PIIFilter::get_replacement_text(const PIIMatch& match) const {
    if (config_.anonymize_instead_of_remove) {
        switch (match.type) {
            case PIIType::EMAIL:
                return config_.use_placeholders ? config_.email_placeholder : generate_anonymous_email();
            case PIIType::PHONE:
                return config_.use_placeholders ? config_.phone_placeholder : generate_anonymous_phone();
            case PIIType::PHYSICAL_ADDRESS:
                return config_.use_placeholders ? config_.address_placeholder : generate_anonymous_address();
            case PIIType::PERSON_NAME:
                return config_.use_placeholders ? config_.name_placeholder : "[PERSON]";
            default:
                return config_.generic_placeholder;
        }
    } else {
        return config_.use_placeholders ? config_.generic_placeholder : "";
    }
}

std::string PIIFilter::generate_anonymous_email() const {
    return "user" + std::to_string(rand() % 10000) + "@example.com";
}

std::string PIIFilter::generate_anonymous_phone() const {
    return "555-0" + std::to_string(100 + (rand() % 900));
}

std::string PIIFilter::generate_anonymous_address() const {
    return std::to_string(100 + (rand() % 900)) + " Example St";
}

bool PIIFilter::is_likely_person_name(const std::string& text) const {
    // Simple heuristic for person names
    if (text.length() < 4 || text.length() > 50) return false;
    
    // Should have at least first and last name
    auto words = ContentUtils::extract_words(text);
    if (words.size() < 2 || words.size() > 4) return false;
    
    // All words should be capitalized
    for (const auto& word : words) {
        if (word.empty() || !std::isupper(word[0])) return false;
    }
    
    return true;
}

bool PIIFilter::passes_luhn_check(const std::string& number) const {
    std::string digits;
    for (char c : number) {
        if (std::isdigit(c)) digits += c;
    }
    
    if (digits.empty()) return false;
    
    int sum = 0;
    bool alternate = false;
    
    for (int i = digits.length() - 1; i >= 0; i--) {
        int digit = digits[i] - '0';
        
        if (alternate) {
            digit *= 2;
            if (digit > 9) digit = (digit % 10) + 1;
        }
        
        sum += digit;
        alternate = !alternate;
    }
    
    return (sum % 10) == 0;
}

std::string PIIFilter::extract_context(const std::string& text, size_t pos, size_t window) const {
    size_t start = pos > window ? pos - window : 0;
    size_t end = std::min(pos + window, text.length());
    return text.substr(start, end - start);
}

bool PIIFilter::has_example_context(const std::string& context) const {
    std::string lower_context = context;
    std::transform(lower_context.begin(), lower_context.end(), lower_context.begin(), ::tolower);
    
    return lower_context.find("example") != std::string::npos ||
           lower_context.find("for example") != std::string::npos ||
           lower_context.find("e.g.") != std::string::npos ||
           lower_context.find("such as") != std::string::npos;
}

bool PIIFilter::has_test_data_context(const std::string& context) const {
    std::string lower_context = context;
    std::transform(lower_context.begin(), lower_context.end(), lower_context.begin(), ::tolower);
    
    return lower_context.find("test") != std::string::npos ||
           lower_context.find("demo") != std::string::npos ||
           lower_context.find("sample") != std::string::npos;
}

double PIIFilter::calculate_privacy_risk_score(const std::string& text) const {
    auto matches = detect_pii(text);
    if (matches.empty()) return 0.0;
    
    double risk_score = 0.0;
    std::unordered_map<PIIType, double> risk_weights = {
        {PIIType::SSN, 1.0},
        {PIIType::CREDIT_CARD, 1.0},
        {PIIType::EMAIL, 0.3},
        {PIIType::PHONE, 0.5},
        {PIIType::PHYSICAL_ADDRESS, 0.7},
        {PIIType::PERSON_NAME, 0.4},
        {PIIType::IP_ADDRESS, 0.2}
    };
    
    for (const auto& match : matches) {
        auto it = risk_weights.find(match.type);
        if (it != risk_weights.end()) {
            risk_score += it->second * match.confidence;
        }
    }
    
    return std::min(1.0, risk_score / 3.0); // Normalize
}

} // namespace content
} // namespace rapidsift 