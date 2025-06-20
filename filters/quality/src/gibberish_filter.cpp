#include "rapidsift/gibberish_filter.hpp"
#include "rapidsift/quality_common.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace rapidsift {
namespace quality {

GibberishFilter::GibberishFilter(const GibberishFilterConfig& config) : config_(config) {
    compile_patterns();
}

void GibberishFilter::configure(const QualityConfig& config) {
    config_ = config.gibberish_config;
    compile_patterns();
}

void GibberishFilter::compile_patterns() {
    gibberish_regexes_.clear();
    
    // Default gibberish patterns if none provided
    std::vector<std::string> default_patterns = {
        R"([a-zA-Z]{50,})",                    // Very long strings of letters
        R"(\d{50,})",                          // Very long strings of digits
        R"([!@#$%^&*()_+={}\[\]|\\:";'<>?,./]{10,})", // Long strings of symbols
        R"((.)\1{20,})",                       // Same character repeated 20+ times
        R"([bcdfghjklmnpqrstvwxyzBCDFGHJKLMNPQRSTVWXYZ]{10,})", // Long strings without vowels
        R"(\b[a-zA-Z]{2,}[0-9]+[a-zA-Z]{2,}\b)", // Mixed alphanumeric tokens
    };
    
    auto patterns = config_.gibberish_patterns.empty() ? default_patterns : config_.gibberish_patterns;
    
    for (const auto& pattern : patterns) {
        try {
            gibberish_regexes_.emplace_back(pattern);
        } catch (const std::regex_error&) {
            // Skip invalid patterns
        }
    }
}

void GibberishFilter::set_config(const GibberishFilterConfig& config) {
    config_ = config;
    compile_patterns();
}

FilterDecision GibberishFilter::evaluate(const Document& doc) const {
    FilterDecision decision(FilterResult::KEEP, RejectionReason::CUSTOM, 1.0, "");
    
    const std::string& text = doc.text;
    if (text.empty()) {
        decision.details = "Empty document";
        return decision;
    }
    
    // Calculate various ratios and metrics
    double alpha_ratio = TextUtils::get_alpha_ratio(text);
    double digit_ratio = TextUtils::get_digit_ratio(text);
    double symbol_ratio = TextUtils::get_symbol_ratio(text);
    double entropy = calculate_character_entropy(text);
    double repetition_ratio = get_repetition_ratio(text);
    size_t max_consecutive = find_longest_consecutive_run(text);
    
    // Store metrics
    decision.metrics["alpha_ratio"] = alpha_ratio;
    decision.metrics["digit_ratio"] = digit_ratio;
    decision.metrics["symbol_ratio"] = symbol_ratio;
    decision.metrics["entropy"] = entropy;
    decision.metrics["repetition_ratio"] = repetition_ratio;
    decision.metrics["max_consecutive_chars"] = static_cast<double>(max_consecutive);
    
    // Check for various gibberish indicators
    std::vector<std::string> violations;
    
    if (has_excessive_non_alpha(text)) {
        violations.push_back("excessive non-alphabetic characters (" + 
                           std::to_string(static_cast<int>(alpha_ratio * 100)) + "% alpha)");
    }
    
    if (has_excessive_digits(text)) {
        violations.push_back("excessive digits (" + 
                           std::to_string(static_cast<int>(digit_ratio * 100)) + "% digits)");
    }
    
    if (has_excessive_symbols(text)) {
        violations.push_back("excessive symbols (" + 
                           std::to_string(static_cast<int>(symbol_ratio * 100)) + "% symbols)");
    }
    
    if (has_excessive_repetition(text)) {
        violations.push_back("excessive character repetition (" + 
                           std::to_string(static_cast<int>(repetition_ratio * 100)) + "%)");
    }
    
    if (has_long_consecutive_chars(text)) {
        violations.push_back("long consecutive character runs (max " + 
                           std::to_string(max_consecutive) + " chars)");
    }
    
    if (has_low_entropy(text)) {
        violations.push_back("low entropy (" + std::to_string(entropy) + ")");
    }
    
    if (matches_gibberish_patterns(text)) {
        violations.push_back("matches gibberish patterns");
    }
    
    if (!appears_linguistic(text)) {
        violations.push_back("does not appear linguistic");
    }
    
    // Make decision
    if (!violations.empty()) {
        decision.result = FilterResult::REJECT;
        decision.reason = RejectionReason::GIBBERISH;
        decision.confidence = std::min(0.95, 0.5 + static_cast<double>(violations.size()) * 0.1);
        
        decision.details = "Gibberish detected: ";
        for (size_t i = 0; i < violations.size(); ++i) {
            if (i > 0) decision.details += ", ";
            decision.details += violations[i];
        }
    } else {
        // Calculate quality score
        double score = 1.0;
        
        // Small penalties for borderline cases
        if (alpha_ratio < 0.7) score *= 0.95;
        if (entropy < 3.0) score *= 0.9;
        if (repetition_ratio > 0.2) score *= 0.9;
        
        decision.confidence = score;
        decision.details = "Text appears linguistic and well-formed";
    }
    
    return decision;
}

bool GibberishFilter::has_excessive_non_alpha(const std::string& text) const {
    double alpha_ratio = TextUtils::get_alpha_ratio(text);
    return alpha_ratio < (1.0 - config_.max_non_alpha_ratio);
}

bool GibberishFilter::has_excessive_digits(const std::string& text) const {
    return TextUtils::get_digit_ratio(text) > config_.max_digit_ratio;
}

bool GibberishFilter::has_excessive_symbols(const std::string& text) const {
    return TextUtils::get_symbol_ratio(text) > config_.max_symbol_ratio;
}

bool GibberishFilter::has_excessive_repetition(const std::string& text) const {
    return get_repetition_ratio(text) > config_.max_repetition_ratio;
}

bool GibberishFilter::has_long_consecutive_chars(const std::string& text) const {
    return find_longest_consecutive_run(text) > config_.max_consecutive_chars;
}

bool GibberishFilter::has_low_entropy(const std::string& text) const {
    return calculate_character_entropy(text) < config_.min_entropy;
}

bool GibberishFilter::matches_gibberish_patterns(const std::string& text) const {
    for (const auto& regex : gibberish_regexes_) {
        if (std::regex_search(text, regex)) {
            return true;
        }
    }
    return false;
}

double GibberishFilter::calculate_character_entropy(const std::string& text) const {
    return TextUtils::calculate_entropy(text);
}

double GibberishFilter::get_repetition_ratio(const std::string& text) const {
    if (text.length() < 10) return 0.0;
    
    std::unordered_map<char, size_t> char_counts;
    for (char c : text) {
        char_counts[c]++;
    }
    
    // Find most frequent character
    size_t max_count = 0;
    for (const auto& pair : char_counts) {
        max_count = std::max(max_count, pair.second);
    }
    
    return static_cast<double>(max_count) / text.length();
}

size_t GibberishFilter::find_longest_consecutive_run(const std::string& text) const {
    if (text.empty()) return 0;
    
    size_t max_run = 1;
    size_t current_run = 1;
    char prev_char = text[0];
    
    for (size_t i = 1; i < text.length(); ++i) {
        if (text[i] == prev_char) {
            current_run++;
            max_run = std::max(max_run, current_run);
        } else {
            current_run = 1;
            prev_char = text[i];
        }
    }
    
    return max_run;
}

bool GibberishFilter::appears_linguistic(const std::string& text) const {
    // Basic linguistic checks
    
    // Check vowel-consonant ratio for alphabetic text
    double vowel_consonant_ratio = calculate_vowel_consonant_ratio(text);
    if (vowel_consonant_ratio < 0.1 || vowel_consonant_ratio > 2.0) {
        return false;
    }
    
    // Check for reasonable word length distribution
    auto words = TextUtils::split_words(text);
    if (words.empty()) return false;
    
    size_t total_length = 0;
    size_t very_long_words = 0;
    
    for (const auto& word : words) {
        total_length += word.length();
        if (word.length() > 20) {
            very_long_words++;
        }
    }
    
    double avg_word_length = static_cast<double>(total_length) / words.size();
    double long_word_ratio = static_cast<double>(very_long_words) / words.size();
    
    // Reasonable average word length and not too many very long words
    return avg_word_length >= 2.0 && avg_word_length <= 15.0 && long_word_ratio < 0.1;
}

double GibberishFilter::calculate_vowel_consonant_ratio(const std::string& text) const {
    std::unordered_set<char> vowels = {'a', 'e', 'i', 'o', 'u', 'A', 'E', 'I', 'O', 'U'};
    
    size_t vowel_count = 0;
    size_t consonant_count = 0;
    
    for (char c : text) {
        if (std::isalpha(c)) {
            if (vowels.count(c)) {
                vowel_count++;
            } else {
                consonant_count++;
            }
        }
    }
    
    if (consonant_count == 0) return vowel_count > 0 ? 999.0 : 0.0;
    return static_cast<double>(vowel_count) / consonant_count;
}

} // namespace quality
} // namespace rapidsift 