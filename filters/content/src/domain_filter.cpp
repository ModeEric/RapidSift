#include "rapidsift/domain_filter.hpp"
#include "rapidsift/content_common.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>

namespace rapidsift {
namespace content {

DomainFilter::DomainFilter(const DomainFilterConfig& config) : config_(config) {
    compile_patterns();
    initialize_known_lists();
}

void DomainFilter::configure(const ContentConfig& config) {
    config_ = config.domain_config;
    compile_patterns();
    initialize_known_lists();
}

void DomainFilter::compile_patterns() {
    blocked_url_regexes_.clear();
    
    // Default suspicious URL patterns if none provided
    std::vector<std::string> default_patterns = {
        R"(.*\.tk/.*)",                    // Suspicious TLD
        R"(.*bit\.ly/[a-zA-Z0-9]{6}.*)",  // Short URLs
        R"(.*\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}.*)", // IP addresses
        R"(.*/click.*)",                  // Click tracking
        R"(.*/redirect.*)",               // Redirects
        R"(.*[0-9]{8,}.*)",              // Long number sequences in URL
        R"(.*[-]{3,}.*)",                // Multiple dashes
        R"(.*[\.]{2,}.*)",               // Multiple dots
    };
    
    auto patterns = config_.blocked_url_patterns.empty() ? default_patterns : config_.blocked_url_patterns;
    
    for (const auto& pattern : patterns) {
        try {
            blocked_url_regexes_.emplace_back(pattern, std::regex_constants::icase);
        } catch (const std::regex_error&) {
            // Skip invalid patterns
        }
    }
}

void DomainFilter::initialize_known_lists() {
    // Initialize known URL shorteners
    url_shorteners_ = {
        "bit.ly", "tinyurl.com", "t.co", "goo.gl", "ow.ly", 
        "short.link", "rb.gy", "cutt.ly", "is.gd", "v.gd"
    };
    
    // Initialize suspicious TLDs
    malicious_tlds_ = {
        "tk", "ml", "ga", "cf", "click", "download", "review"
    };
    
    // Add common spam/malicious domains to blocklist
    if (config_.blocked_domains.empty()) {
        std::unordered_set<std::string> default_blocked = {
            "spam-site.com", "malware-host.net", "phishing-example.org",
            "auto-content.info", "click-farm.co", "fake-news.net"
        };
        config_.blocked_domains.insert(default_blocked.begin(), default_blocked.end());
    }
}

void DomainFilter::set_config(const DomainFilterConfig& config) {
    config_ = config;
    compile_patterns();
    initialize_known_lists();
}

FilterDecision DomainFilter::evaluate(const Document& doc) const {
    FilterDecision decision(FilterResult::KEEP, ActionReason::CUSTOM, 1.0, "");
    
    // Extract domain from URL
    std::string domain = ContentUtils::extract_domain(doc.url);
    if (domain.empty() && !doc.url.empty()) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::SUSPICIOUS_URL;
        decision.confidence = 0.8;
        decision.details = "Invalid or malformed URL: " + doc.url;
        return decision;
    }
    
    // Store metrics
    decision.scores["domain_reputation"] = calculate_domain_reputation(domain);
    decision.scores["url_suspicion"] = is_url_suspicious(doc.url) ? 1.0 : 0.0;
    
    // Check domain blocklist
    if (is_domain_blocked(domain)) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::BLOCKED_DOMAIN;
        decision.confidence = 0.95;
        decision.details = "Domain is on blocklist: " + domain;
        return decision;
    }
    
    // Check if explicitly allowed
    if (is_domain_allowed(domain)) {
        decision.details = "Domain is on allowlist: " + domain;
        return decision;
    }
    
    // Check TLD blocklist
    std::string tld = ContentUtils::extract_tld(domain);
    if (is_tld_blocked(tld)) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::SUSPICIOUS_URL;
        decision.confidence = 0.8;
        decision.details = "Blocked TLD: " + tld;
        return decision;
    }
    
    // Check URL patterns
    if (matches_blocked_patterns(doc.url)) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::SUSPICIOUS_URL;
        decision.confidence = 0.85;
        decision.details = "URL matches suspicious pattern";
        return decision;
    }
    
    // Check for spam keywords in URL
    if (contains_spam_keywords(doc.url)) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::SUSPICIOUS_URL;
        decision.confidence = 0.7;
        decision.details = "URL contains spam keywords";
        return decision;
    }
    
    // Check IP-based URLs if configured
    if (config_.block_ip_addresses && is_ip_based_url(doc.url)) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::SUSPICIOUS_URL;
        decision.confidence = 0.9;
        decision.details = "IP-based URL detected";
        return decision;
    }
    
    // Check URL shorteners if configured
    if (config_.check_url_shorteners && is_url_shortener_domain(domain)) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::SUSPICIOUS_URL;
        decision.confidence = 0.6;
        decision.details = "URL shortener detected: " + domain;
        return decision;
    }
    
    // Calculate overall suspicion score
    size_t suspicious_patterns = count_suspicious_patterns(doc.url);
    double suspicion_score = static_cast<double>(suspicious_patterns) / 10.0; // Normalize
    
    if (suspicion_score > 0.5) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::SUSPICIOUS_URL;
        decision.confidence = suspicion_score;
        decision.details = "High suspicion score: " + std::to_string(suspicion_score);
        return decision;
    }
    
    // Domain reputation check
    double reputation = calculate_domain_reputation(domain);
    if (reputation < 0.3) {
        decision.result = FilterResult::REJECT;
        decision.reason = ActionReason::BLOCKED_DOMAIN;
        decision.confidence = 1.0 - reputation;
        decision.details = "Low domain reputation: " + std::to_string(reputation);
        return decision;
    }
    
    // Calculate quality score
    double score = 1.0;
    score *= reputation;  // Scale by domain reputation
    score *= (1.0 - suspicion_score);  // Reduce by suspicion
    
    decision.confidence = score;
    decision.details = "Domain check passed: " + domain;
    
    return decision;
}

bool DomainFilter::is_domain_blocked(const std::string& domain) const {
    return config_.blocked_domains.count(domain) > 0;
}

bool DomainFilter::is_domain_allowed(const std::string& domain) const {
    return config_.allowed_domains.count(domain) > 0;
}

bool DomainFilter::is_tld_blocked(const std::string& tld) const {
    return std::find(config_.blocked_tlds.begin(), config_.blocked_tlds.end(), tld) 
           != config_.blocked_tlds.end();
}

bool DomainFilter::is_url_suspicious(const std::string& url) const {
    return matches_blocked_patterns(url) || 
           contains_spam_keywords(url) || 
           has_suspicious_subdomain(url) ||
           has_suspicious_path(url) ||
           has_suspicious_parameters(url);
}

bool DomainFilter::matches_blocked_patterns(const std::string& url) const {
    for (const auto& regex : blocked_url_regexes_) {
        if (std::regex_match(url, regex)) {
            return true;
        }
    }
    return false;
}

bool DomainFilter::contains_spam_keywords(const std::string& url) const {
    std::string lower_url = url;
    std::transform(lower_url.begin(), lower_url.end(), lower_url.begin(), ::tolower);
    
    for (const auto& keyword : config_.spam_keywords) {
        if (lower_url.find(keyword) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool DomainFilter::is_url_shortener_domain(const std::string& domain) const {
    return url_shorteners_.count(domain) > 0;
}

bool DomainFilter::is_ip_based_url(const std::string& url) const {
    std::string domain = ContentUtils::extract_domain(url);
    return ContentUtils::is_ip_address(domain);
}

double DomainFilter::calculate_domain_reputation(const std::string& domain) const {
    if (domain.empty()) return 0.0;
    
    double reputation = 0.5; // Neutral starting point
    
    // Boost for known good domains
    if (is_domain_allowed(domain)) {
        reputation = 1.0;
    }
    
    // Penalty for blocked domains
    if (is_domain_blocked(domain)) {
        reputation = 0.0;
    }
    
    // TLD-based scoring
    std::string tld = ContentUtils::extract_tld(domain);
    if (malicious_tlds_.count(tld)) {
        reputation *= 0.3;
    } else if (tld == "com" || tld == "org" || tld == "edu" || tld == "gov") {
        reputation *= 1.2;
    }
    
    // Domain length and structure
    if (domain.length() > 50) {
        reputation *= 0.8; // Very long domains are suspicious
    }
    
    // Check for suspicious patterns in domain name
    if (domain.find("xn--") == 0) {  // Punycode domains
        reputation *= 0.7;
    }
    
    if (std::count(domain.begin(), domain.end(), '-') > 3) {
        reputation *= 0.8; // Many hyphens
    }
    
    return std::max(0.0, std::min(1.0, reputation));
}

bool DomainFilter::is_known_malicious_domain(const std::string& domain) const {
    return is_domain_blocked(domain);
}

bool DomainFilter::is_parked_domain(const std::string& domain) const {
    // Simple heuristic for parked domains
    return domain.find("parking") != std::string::npos ||
           domain.find("parked") != std::string::npos ||
           domain.find("for-sale") != std::string::npos;
}

bool DomainFilter::has_suspicious_subdomain(const std::string& url) const {
    std::string domain = ContentUtils::extract_domain(url);
    
    // Count subdomains
    size_t dot_count = std::count(domain.begin(), domain.end(), '.');
    if (dot_count > 4) return true; // Too many subdomains
    
    // Check for suspicious subdomain patterns
    if (domain.find("admin.") == 0 || 
        domain.find("secure.") == 0 ||
        domain.find("login.") == 0) {
        return true;
    }
    
    return false;
}

bool DomainFilter::has_suspicious_path(const std::string& url) const {
    size_t path_start = url.find('/', 8); // Skip http://
    if (path_start == std::string::npos) return false;
    
    std::string path = url.substr(path_start);
    
    // Very long paths
    if (path.length() > 200) return true;
    
    // Suspicious path patterns
    return path.find("/click") != std::string::npos ||
           path.find("/redirect") != std::string::npos ||
           path.find("/track") != std::string::npos ||
           path.find("/ads") != std::string::npos;
}

bool DomainFilter::has_suspicious_parameters(const std::string& url) const {
    size_t param_start = url.find('?');
    if (param_start == std::string::npos) return false;
    
    std::string params = url.substr(param_start);
    
    // Too many parameters
    if (std::count(params.begin(), params.end(), '&') > 10) return true;
    
    // Suspicious parameter names
    return params.find("click") != std::string::npos ||
           params.find("track") != std::string::npos ||
           params.find("referrer") != std::string::npos;
}

size_t DomainFilter::count_suspicious_patterns(const std::string& url) const {
    size_t count = 0;
    
    if (has_suspicious_subdomain(url)) count++;
    if (has_suspicious_path(url)) count++;
    if (has_suspicious_parameters(url)) count++;
    if (contains_spam_keywords(url)) count++;
    
    std::string domain = ContentUtils::extract_domain(url);
    if (domain.length() > 50) count++;
    if (std::count(domain.begin(), domain.end(), '-') > 3) count++;
    if (std::count(domain.begin(), domain.end(), '.') > 4) count++;
    
    return count;
}

void DomainFilter::add_blocked_domain(const std::string& domain) {
    config_.blocked_domains.insert(domain);
}

void DomainFilter::remove_blocked_domain(const std::string& domain) {
    config_.blocked_domains.erase(domain);
}

void DomainFilter::add_allowed_domain(const std::string& domain) {
    config_.allowed_domains.insert(domain);
}

void DomainFilter::load_domain_list(const std::string& filename, bool is_blocklist) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    
    std::string domain;
    while (std::getline(file, domain)) {
        if (!domain.empty() && domain[0] != '#') { // Skip comments
            if (is_blocklist) {
                config_.blocked_domains.insert(domain);
            } else {
                config_.allowed_domains.insert(domain);
            }
        }
    }
}

void DomainFilter::save_domain_list(const std::string& filename, bool is_blocklist) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    const auto& domains = is_blocklist ? config_.blocked_domains : config_.allowed_domains;
    for (const auto& domain : domains) {
        file << domain << std::endl;
    }
}

void DomainFilter::load_common_blocklists() {
    // This would typically load from external sources
    // For now, we'll use a hardcoded list
    std::vector<std::string> common_blocked = {
        "malware.com", "phishing.net", "spam.org", "virus.info",
        "scam.co", "fake.news", "click.farm", "auto.content"
    };
    
    for (const auto& domain : common_blocked) {
        config_.blocked_domains.insert(domain);
    }
}

void DomainFilter::update_threat_intelligence() {
    // Future: Integration with threat intelligence feeds
    // This would download updated blocklists from external sources
}

} // namespace content
} // namespace rapidsift 