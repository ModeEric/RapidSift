#include "rapidsift/license_filter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>

namespace rapidsift {
namespace content {

// Default constructor for LicenseFilterConfig
LicenseFilterConfig::LicenseFilterConfig() {
    // Initialize with safe defaults
    allowed_licenses = {
        LicenseType::PUBLIC_DOMAIN,
        LicenseType::CREATIVE_COMMONS_0,
        LicenseType::CREATIVE_COMMONS_BY,
        LicenseType::CREATIVE_COMMONS_BY_SA
    };
    
    strict_mode = false;
    allow_fair_use = false;
    require_explicit_permission = true;
    confidence_threshold = 0.7;
    enforce_robots_txt = true;
    respect_noindex_meta = true;
    check_creative_commons_meta = true;
}

// LicenseFilter Implementation
LicenseFilter::LicenseFilter(const LicenseFilterConfig& config) : config_(config) {
    initialize_default_patterns();
    initialize_default_domains();
}

FilterDecision LicenseFilter::evaluate(const Document& doc) const {
    auto assessment = assess_copyright(doc);
    
    FilterDecision decision;
    decision.filter_name = get_name();
    decision.confidence = assessment.compliance_confidence;
    
    // Determine if content should be kept
    bool keep = true;
    std::string reason;
    
    // Check opt-out requests
    std::string domain = extract_domain_from_url(doc.url);
    if (is_opted_out(domain)) {
        keep = false;
        reason = "Domain opted out: " + domain;
    }
    
    // Check removal requests
    if (requires_removal(doc.id)) {
        keep = false;
        reason = "Content removal requested: " + doc.id;
    }
    
    // Check domain allowlist
    if (keep && !config_.allowed_domains.empty() && !is_domain_allowed(domain)) {
        keep = false;
        reason = "Domain not in allowlist: " + domain;
    }
    
    // Check blocked domains
    if (keep && config_.blocked_domains.count(domain)) {
        keep = false;
        reason = "Domain blocked: " + domain;
    }
    
    // Check paywall status
    if (keep && assessment.is_paywalled) {
        keep = false;
        reason = "Content is paywalled";
    }
    
    // Check license compliance
    if (keep && !has_valid_license(assessment.detected_license)) {
        if (config_.strict_mode || assessment.has_copyright_notice) {
            keep = false;
            reason = "License not allowed: " + license_utils::license_type_to_string(assessment.detected_license);
        }
    }
    
    // Check confidence threshold
    if (keep && assessment.compliance_confidence < config_.confidence_threshold) {
        if (config_.strict_mode) {
            keep = false;
            reason = "License compliance confidence too low: " + std::to_string(assessment.compliance_confidence);
        }
    }
    
    decision.result = keep ? FilterResult::KEEP : FilterResult::REJECT;
    decision.details = reason.empty() ? "License compliant" : reason;
    
    // Update statistics
    update_stats(assessment, decision.result);
    
    return decision;
}

void LicenseFilter::configure(const ContentConfig& config) {
    // This is called from the base class - we can adapt if needed
    // For now, just ensure our internal config is set up
    if (config_.allowed_domains.empty()) {
        initialize_default_domains();
    }
}

void LicenseFilter::set_license_config(const LicenseFilterConfig& config) {
    config_ = config;
    initialize_default_patterns();
    initialize_default_domains();
}

CopyrightAssessment LicenseFilter::assess_copyright(const Document& doc) const {
    CopyrightAssessment assessment;
    
    std::string domain = extract_domain_from_url(doc.url);
    assessment.is_from_allowed_domain = is_domain_allowed(domain);
    
    // Check for copyright notices
    assessment.copyright_notices = find_copyright_notices(doc.text);
    assessment.has_copyright_notice = !assessment.copyright_notices.empty();
    
    // Detect license
    assessment.detected_license = detect_license(doc.text, doc.url);
    assessment.license_indicators = find_license_indicators(doc.text);
    
    // Check paywall status
    assessment.is_paywalled = is_paywalled(doc);
    
    // Check opt-out signals
    assessment.has_opt_out_signal = is_opted_out(domain);
    
    // Check removal requirements
    assessment.requires_removal = requires_removal(doc.id);
    
    // Calculate compliance confidence
    assessment.compliance_confidence = calculate_compliance_confidence(assessment);
    
    return assessment;
}

std::vector<CopyrightAssessment> LicenseFilter::assess_documents(const std::vector<Document>& docs) const {
    std::vector<CopyrightAssessment> assessments;
    assessments.reserve(docs.size());
    
    for (const auto& doc : docs) {
        assessments.push_back(assess_copyright(doc));
    }
    
    return assessments;
}

void LicenseFilter::add_allowed_domain(const std::string& domain) {
    config_.allowed_domains.insert(domain);
}

void LicenseFilter::add_blocked_domain(const std::string& domain) {
    config_.blocked_domains.insert(domain);
}

void LicenseFilter::load_domain_lists(const std::string& directory) {
    // Load allowed domains
    std::ifstream allowed_file(directory + "/allowed_domains.txt");
    if (allowed_file.is_open()) {
        std::string domain;
        while (std::getline(allowed_file, domain)) {
            if (!domain.empty() && domain[0] != '#') {
                config_.allowed_domains.insert(domain);
            }
        }
    }
    
    // Load blocked domains
    std::ifstream blocked_file(directory + "/blocked_domains.txt");
    if (blocked_file.is_open()) {
        std::string domain;
        while (std::getline(blocked_file, domain)) {
            if (!domain.empty() && domain[0] != '#') {
                config_.blocked_domains.insert(domain);
            }
        }
    }
    
    // Load Creative Commons domains
    std::ifstream cc_file(directory + "/creative_commons_domains.txt");
    if (cc_file.is_open()) {
        std::string domain;
        while (std::getline(cc_file, domain)) {
            if (!domain.empty() && domain[0] != '#') {
                config_.creative_commons_domains.insert(domain);
            }
        }
    }
}

LicenseType LicenseFilter::detect_license(const std::string& text, const std::string& url) const {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    // Check for Creative Commons licenses
    if (lower_text.find("creative commons") != std::string::npos || 
        lower_text.find("cc by") != std::string::npos) {
        return license_utils::detect_creative_commons_license(text);
    }
    
    // Check for public domain
    if (lower_text.find("public domain") != std::string::npos || 
        lower_text.find("cc0") != std::string::npos) {
        return LicenseType::PUBLIC_DOMAIN;
    }
    
    // Check for standard licenses
    return license_utils::detect_standard_license(text);
}

bool LicenseFilter::has_copyright_notice(const std::string& text) const {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    // Check for common copyright patterns
    return lower_text.find("copyright") != std::string::npos ||
           lower_text.find("©") != std::string::npos ||
           lower_text.find("(c)") != std::string::npos ||
           lower_text.find("all rights reserved") != std::string::npos;
}

bool LicenseFilter::is_paywalled(const Document& doc) const {
    std::string domain = extract_domain_from_url(doc.url);
    
    // Check known paywalled domains
    if (config_.paywalled_domains.count(domain)) {
        return true;
    }
    
    // Check for paywall indicators in text
    std::string lower_text = doc.text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    return lower_text.find("subscribe") != std::string::npos ||
           lower_text.find("paywall") != std::string::npos ||
           lower_text.find("premium content") != std::string::npos ||
           lower_text.find("members only") != std::string::npos;
}

void LicenseFilter::load_opt_out_list(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        size_t tab_pos = line.find('\t');
        if (tab_pos != std::string::npos) {
            std::string domain = line.substr(0, tab_pos);
            std::string reason = line.substr(tab_pos + 1);
            opted_out_domains_.insert(domain);
            opt_out_reasons_[domain] = reason;
        } else {
            opted_out_domains_.insert(line);
        }
    }
}

void LicenseFilter::add_opt_out_request(const std::string& domain, const std::string& reason) {
    opted_out_domains_.insert(domain);
    if (!reason.empty()) {
        opt_out_reasons_[domain] = reason;
    }
}

bool LicenseFilter::is_opted_out(const std::string& domain) const {
    return opted_out_domains_.count(domain) > 0;
}

bool LicenseFilter::requires_removal(const std::string& content_id) const {
    return removal_requests_.count(content_id) > 0;
}

void LicenseFilter::reset_stats() {
    stats_ = LicenseFilterStats{};
}

void LicenseFilter::print_compliance_report() const {
    std::cout << "\n=== License Compliance Report ===\n";
    std::cout << "Total processed: " << stats_.total_processed << "\n";
    std::cout << "Kept: " << stats_.kept << " (" << (stats_.get_compliance_rate() * 100) << "%)\n";
    std::cout << "Rejected: " << stats_.rejected << " (" << (stats_.get_rejection_rate() * 100) << "%)\n";
    std::cout << "Copyright notices found: " << stats_.copyright_notices_found << "\n";
    std::cout << "Paywalled content: " << stats_.paywalled_content << "\n";
    std::cout << "Opt-out requests: " << stats_.opt_out_requests << "\n";
    
    std::cout << "\nLicense Distribution:\n";
    for (const auto& [license, count] : stats_.license_counts) {
        std::cout << "  " << license_utils::license_type_to_string(license) 
                  << ": " << count << "\n";
    }
    
    if (!stats_.domain_rejections.empty()) {
        std::cout << "\nTop Rejected Domains:\n";
        std::vector<std::pair<std::string, size_t>> sorted_rejections(
            stats_.domain_rejections.begin(), stats_.domain_rejections.end());
        std::sort(sorted_rejections.begin(), sorted_rejections.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (size_t i = 0; i < std::min(size_t(10), sorted_rejections.size()); ++i) {
            std::cout << "  " << sorted_rejections[i].first 
                      << ": " << sorted_rejections[i].second << "\n";
        }
    }
}

// Private helper methods
bool LicenseFilter::is_domain_allowed(const std::string& domain) const {
    if (config_.allowed_domains.empty()) return true;  // No allowlist = allow all
    return config_.allowed_domains.count(domain) > 0 ||
           config_.creative_commons_domains.count(domain) > 0 ||
           config_.public_domain_domains.count(domain) > 0;
}

bool LicenseFilter::has_valid_license(LicenseType license) const {
    return config_.allowed_licenses.count(license) > 0;
}

std::string LicenseFilter::extract_domain_from_url(const std::string& url) const {
    // Simple domain extraction
    size_t start = url.find("://");
    if (start == std::string::npos) return url;
    start += 3;
    
    size_t end = url.find('/', start);
    if (end == std::string::npos) end = url.length();
    
    std::string domain = url.substr(start, end - start);
    
    // Remove www. prefix if present
    if (domain.substr(0, 4) == "www.") {
        domain = domain.substr(4);
    }
    
    return domain;
}

std::vector<std::string> LicenseFilter::find_copyright_notices(const std::string& text) const {
    std::vector<std::string> notices;
    
    // Simple pattern matching for now
    size_t pos = 0;
    while ((pos = text.find("©", pos)) != std::string::npos) {
        // Extract the copyright notice (next 100 characters)
        size_t end = std::min(pos + 100, text.length());
        std::string notice = text.substr(pos, end - pos);
        notices.push_back(notice);
        pos = end;
    }
    
    return notices;
}

std::vector<std::string> LicenseFilter::find_license_indicators(const std::string& text) const {
    std::vector<std::string> indicators;
    
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    // Look for license keywords
    std::vector<std::string> keywords = {
        "creative commons", "cc by", "cc0", "public domain",
        "mit license", "gpl", "apache", "bsd"
    };
    
    for (const auto& keyword : keywords) {
        if (lower_text.find(keyword) != std::string::npos) {
            indicators.push_back(keyword);
        }
    }
    
    return indicators;
}

double LicenseFilter::calculate_compliance_confidence(const CopyrightAssessment& assessment) const {
    double confidence = 0.5;  // Base confidence
    
    // Increase confidence for explicit licenses
    if (assessment.detected_license != LicenseType::UNKNOWN) {
        confidence += 0.3;
    }
    
    // Increase confidence for allowed domains
    if (assessment.is_from_allowed_domain) {
        confidence += 0.2;
    }
    
    // Decrease confidence for copyright notices without clear licenses
    if (assessment.has_copyright_notice && assessment.detected_license == LicenseType::UNKNOWN) {
        confidence -= 0.3;
    }
    
    // Decrease confidence for paywalled content
    if (assessment.is_paywalled) {
        confidence -= 0.4;
    }
    
    return std::max(0.0, std::min(1.0, confidence));
}

void LicenseFilter::update_stats(const CopyrightAssessment& assessment, FilterResult result) const {
    stats_.total_processed++;
    
    if (result == FilterResult::KEEP) {
        stats_.kept++;
    } else {
        stats_.rejected++;
    }
    
    // Update license counts
    stats_.license_counts[assessment.detected_license]++;
    
    // Update other counters
    if (assessment.has_copyright_notice) {
        stats_.copyright_notices_found++;
    }
    
    if (assessment.is_paywalled) {
        stats_.paywalled_content++;
    }
    
    if (assessment.has_opt_out_signal) {
        stats_.opt_out_requests++;
    }
}

void LicenseFilter::initialize_default_patterns() {
    // Initialize regex patterns for license detection
    // This is a simplified version - in production you'd want more sophisticated patterns
}

void LicenseFilter::initialize_default_domains() {
    if (config_.allowed_domains.empty()) {
        config_.allowed_domains = license_utils::load_common_allowed_domains();
    }
    
    if (config_.creative_commons_domains.empty()) {
        config_.creative_commons_domains = license_utils::load_creative_commons_domains();
    }
    
    if (config_.public_domain_domains.empty()) {
        config_.public_domain_domains = license_utils::load_public_domain_domains();
    }
}

// Utility function implementations
namespace license_utils {

LicenseType detect_creative_commons_license(const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    if (lower_text.find("cc0") != std::string::npos) {
        return LicenseType::CREATIVE_COMMONS_0;
    } else if (lower_text.find("cc by-sa") != std::string::npos) {
        return LicenseType::CREATIVE_COMMONS_BY_SA;
    } else if (lower_text.find("cc by-nc") != std::string::npos) {
        return LicenseType::CREATIVE_COMMONS_BY_NC;
    } else if (lower_text.find("cc by-nd") != std::string::npos) {
        return LicenseType::CREATIVE_COMMONS_BY_ND;
    } else if (lower_text.find("cc by") != std::string::npos) {
        return LicenseType::CREATIVE_COMMONS_BY;
    }
    
    return LicenseType::UNKNOWN;
}

LicenseType detect_standard_license(const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    if (lower_text.find("mit license") != std::string::npos) {
        return LicenseType::MIT;
    } else if (lower_text.find("apache license") != std::string::npos) {
        return LicenseType::APACHE_2;
    } else if (lower_text.find("gpl") != std::string::npos) {
        if (lower_text.find("version 3") != std::string::npos || lower_text.find("v3") != std::string::npos) {
            return LicenseType::GPL_V3;
        } else {
            return LicenseType::GPL_V2;
        }
    } else if (lower_text.find("bsd") != std::string::npos) {
        if (lower_text.find("3-clause") != std::string::npos) {
            return LicenseType::BSD_3_CLAUSE;
        } else {
            return LicenseType::BSD_2_CLAUSE;
        }
    }
    
    return LicenseType::UNKNOWN;
}

bool is_open_license(LicenseType license) {
    switch (license) {
        case LicenseType::PUBLIC_DOMAIN:
        case LicenseType::CREATIVE_COMMONS_0:
        case LicenseType::CREATIVE_COMMONS_BY:
        case LicenseType::CREATIVE_COMMONS_BY_SA:
        case LicenseType::MIT:
        case LicenseType::APACHE_2:
        case LicenseType::BSD_2_CLAUSE:
        case LicenseType::BSD_3_CLAUSE:
            return true;
        default:
            return false;
    }
}

std::string license_type_to_string(LicenseType license) {
    switch (license) {
        case LicenseType::UNKNOWN: return "Unknown";
        case LicenseType::PUBLIC_DOMAIN: return "Public Domain";
        case LicenseType::CREATIVE_COMMONS_0: return "CC0";
        case LicenseType::CREATIVE_COMMONS_BY: return "CC BY";
        case LicenseType::CREATIVE_COMMONS_BY_SA: return "CC BY-SA";
        case LicenseType::CREATIVE_COMMONS_BY_NC: return "CC BY-NC";
        case LicenseType::CREATIVE_COMMONS_BY_ND: return "CC BY-ND";
        case LicenseType::MIT: return "MIT";
        case LicenseType::GPL_V2: return "GPL v2";
        case LicenseType::GPL_V3: return "GPL v3";
        case LicenseType::APACHE_2: return "Apache 2.0";
        case LicenseType::BSD_2_CLAUSE: return "BSD 2-Clause";
        case LicenseType::BSD_3_CLAUSE: return "BSD 3-Clause";
        case LicenseType::PROPRIETARY: return "Proprietary";
        case LicenseType::COPYRIGHTED: return "Copyrighted";
        case LicenseType::PAYWALLED: return "Paywalled";
        case LicenseType::RESTRICTED: return "Restricted";
        default: return "Unknown";
    }
}

std::unordered_set<std::string> load_common_allowed_domains() {
    return {
        "wikipedia.org",
        "wikimedia.org",
        "archive.org",
        "gutenberg.org",
        "openlibrary.org",
        "commons.wikimedia.org",
        "github.com",
        "stackoverflow.com",
        "reddit.com",
        "arxiv.org"
    };
}

std::unordered_set<std::string> load_creative_commons_domains() {
    return {
        "creativecommons.org",
        "flickr.com",
        "commons.wikimedia.org",
        "openclipart.org",
        "freesound.org"
    };
}

std::unordered_set<std::string> load_public_domain_domains() {
    return {
        "gutenberg.org",
        "archive.org",
        "loc.gov",
        "nasa.gov",
        "nih.gov",
        "cdc.gov"
    };
}

LicenseFilterConfig create_strict_config() {
    LicenseFilterConfig config;
    config.strict_mode = true;
    config.require_explicit_permission = true;
    config.confidence_threshold = 0.8;
    config.allow_fair_use = false;
    
    // Only very safe licenses
    config.allowed_licenses = {
        LicenseType::PUBLIC_DOMAIN,
        LicenseType::CREATIVE_COMMONS_0,
        LicenseType::CREATIVE_COMMONS_BY
    };
    
    return config;
}

LicenseFilterConfig create_permissive_config() {
    LicenseFilterConfig config;
    config.strict_mode = false;
    config.require_explicit_permission = false;
    config.confidence_threshold = 0.5;
    config.allow_fair_use = true;
    
    // More permissive license set
    config.allowed_licenses = {
        LicenseType::PUBLIC_DOMAIN,
        LicenseType::CREATIVE_COMMONS_0,
        LicenseType::CREATIVE_COMMONS_BY,
        LicenseType::CREATIVE_COMMONS_BY_SA,
        LicenseType::MIT,
        LicenseType::APACHE_2,
        LicenseType::BSD_2_CLAUSE,
        LicenseType::BSD_3_CLAUSE
    };
    
    return config;
}

} // namespace license_utils

} // namespace content
} // namespace rapidsift 