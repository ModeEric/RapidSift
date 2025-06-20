#include "rapidsift/repetition_filter.hpp"
#include "rapidsift/quality_common.hpp"
#include <algorithm>
#include <regex>

namespace rapidsift {
namespace quality {

RepetitionFilter::RepetitionFilter(const RepetitionFilterConfig& config) : config_(config) {}

void RepetitionFilter::configure(const QualityConfig& config) {
    config_ = config.repetition_config;
}

FilterDecision RepetitionFilter::evaluate(const Document& doc) const {
    FilterDecision decision(FilterResult::KEEP, RejectionReason::CUSTOM, 1.0, "");
    
    const std::string& text = doc.text;
    if (text.empty()) {
        decision.details = "Empty document";
        return decision;
    }
    
    // Calculate metrics
    double line_repetition_ratio = calculate_line_repetition_ratio(text);
    double ngram_repetition_ratio = calculate_ngram_repetition_ratio(text, config_.ngram_size);
    double unique_word_ratio = calculate_unique_word_ratio(text);
    size_t unique_words = count_unique_words(text);
    double boilerplate_score = calculate_boilerplate_score(text);
    double lexical_diversity = calculate_lexical_diversity(text);
    
    // Store metrics
    decision.metrics["line_repetition_ratio"] = line_repetition_ratio;
    decision.metrics["ngram_repetition_ratio"] = ngram_repetition_ratio;
    decision.metrics["unique_word_ratio"] = unique_word_ratio;
    decision.metrics["unique_words"] = static_cast<double>(unique_words);
    decision.metrics["boilerplate_score"] = boilerplate_score;
    decision.metrics["lexical_diversity"] = lexical_diversity;
    
    // Check for violations
    std::vector<std::string> violations;
    
    if (has_excessive_line_repetition(text)) {
        violations.push_back("excessive line repetition (" + 
                           std::to_string(static_cast<int>(line_repetition_ratio * 100)) + "%)");
    }
    
    if (has_excessive_ngram_repetition(text)) {
        violations.push_back("excessive " + std::to_string(config_.ngram_size) + "-gram repetition (" +
                           std::to_string(static_cast<int>(ngram_repetition_ratio * 100)) + "%)");
    }
    
    if (has_insufficient_unique_words(text)) {
        violations.push_back("insufficient unique words (" + std::to_string(unique_words) + 
                           " words, " + std::to_string(static_cast<int>(unique_word_ratio * 100)) + "% unique)");
    }
    
    if (is_boilerplate(text)) {
        violations.push_back("appears to be boilerplate (score: " + 
                           std::to_string(boilerplate_score) + ")");
    }
    
    if (is_template_like(text)) {
        violations.push_back("appears template-like");
    }
    
    // Make decision
    if (!violations.empty()) {
        decision.result = FilterResult::REJECT;
        decision.reason = RejectionReason::HIGH_REPETITION;
        decision.confidence = std::min(0.95, 0.6 + static_cast<double>(violations.size()) * 0.1);
        
        decision.details = "High repetition detected: ";
        for (size_t i = 0; i < violations.size(); ++i) {
            if (i > 0) decision.details += ", ";
            decision.details += violations[i];
        }
    } else {
        // Calculate quality score
        double score = 1.0;
        
        // Penalties for borderline cases
        if (line_repetition_ratio > 0.2) score *= 0.9;
        if (unique_word_ratio < 0.5) score *= 0.9;
        if (lexical_diversity < 0.3) score *= 0.85;
        
        decision.confidence = score;
        decision.details = "Text shows acceptable diversity and low repetition";
    }
    
    return decision;
}

bool RepetitionFilter::has_excessive_line_repetition(const std::string& text) const {
    return calculate_line_repetition_ratio(text) > config_.max_line_repetition_ratio;
}

double RepetitionFilter::calculate_line_repetition_ratio(const std::string& text) const {
    auto line_counts = count_lines(text);
    if (line_counts.empty()) return 0.0;
    
    size_t total_lines = 0;
    size_t repeated_lines = 0;
    
    for (const auto& pair : line_counts) {
        total_lines += pair.second;
        if (pair.second > 1) {
            repeated_lines += pair.second;
        }
    }
    
    return total_lines > 0 ? static_cast<double>(repeated_lines) / total_lines : 0.0;
}

std::unordered_map<std::string, size_t> RepetitionFilter::count_lines(const std::string& text) const {
    auto lines = TextUtils::split_lines(text);
    std::unordered_map<std::string, size_t> line_counts;
    
    for (const auto& line : lines) {
        std::string normalized = TextUtils::normalize_whitespace(line);
        if (!normalized.empty()) {
            line_counts[normalized]++;
        }
    }
    
    return line_counts;
}

bool RepetitionFilter::has_excessive_ngram_repetition(const std::string& text) const {
    return calculate_ngram_repetition_ratio(text, config_.ngram_size) > config_.max_ngram_repetition_ratio;
}

double RepetitionFilter::calculate_ngram_repetition_ratio(const std::string& text, size_t n) const {
    auto ngrams = extract_ngrams(text, n);
    if (ngrams.empty()) return 0.0;
    
    std::unordered_map<std::string, size_t> ngram_counts;
    for (const auto& ngram : ngrams) {
        ngram_counts[ngram]++;
    }
    
    size_t repeated_ngrams = 0;
    for (const auto& pair : ngram_counts) {
        if (pair.second > 1) {
            repeated_ngrams += pair.second;
        }
    }
    
    return static_cast<double>(repeated_ngrams) / ngrams.size();
}

std::vector<std::string> RepetitionFilter::extract_ngrams(const std::string& text, size_t n) const {
    auto words = TextUtils::split_words(text);
    std::vector<std::string> ngrams;
    
    if (words.size() < n) return ngrams;
    
    for (size_t i = 0; i <= words.size() - n; ++i) {
        std::string ngram;
        for (size_t j = 0; j < n; ++j) {
            if (j > 0) ngram += " ";
            ngram += words[i + j];
        }
        ngrams.push_back(ngram);
    }
    
    return ngrams;
}

bool RepetitionFilter::has_insufficient_unique_words(const std::string& text) const {
    size_t unique_words = count_unique_words(text);
    double unique_ratio = calculate_unique_word_ratio(text);
    
    return unique_words < config_.min_unique_words || unique_ratio < config_.min_unique_word_ratio;
}

double RepetitionFilter::calculate_unique_word_ratio(const std::string& text) const {
    auto words = TextUtils::split_words(text);
    if (words.empty()) return 0.0;
    
    std::unordered_set<std::string> unique_words(words.begin(), words.end());
    return static_cast<double>(unique_words.size()) / words.size();
}

size_t RepetitionFilter::count_unique_words(const std::string& text) const {
    auto words = TextUtils::split_words(text);
    std::unordered_set<std::string> unique_words(words.begin(), words.end());
    return unique_words.size();
}

bool RepetitionFilter::is_boilerplate(const std::string& text) const {
    return calculate_boilerplate_score(text) > config_.max_boilerplate_ratio;
}

double RepetitionFilter::calculate_boilerplate_score(const std::string& text) const {
    double score = 0.0;
    
    // Check for common boilerplate patterns
    if (contains_common_boilerplate_patterns(text)) {
        score += 0.3;
    }
    
    // High line repetition indicates boilerplate
    double line_rep = calculate_line_repetition_ratio(text);
    score += line_rep * 0.4;
    
    // Low lexical diversity indicates boilerplate
    double diversity = calculate_lexical_diversity(text);
    if (diversity < 0.3) {
        score += (0.3 - diversity) * 2.0;
    }
    
    // Template-like structure
    if (is_template_like(text)) {
        score += 0.2;
    }
    
    return std::min(1.0, score);
}

bool RepetitionFilter::contains_common_boilerplate_patterns(const std::string& text) const {
    std::vector<std::regex> boilerplate_patterns = {
        std::regex(R"(\bcopyright\b.*\ball rights reserved\b)", std::regex_constants::icase),
        std::regex(R"(\bterms of service\b|\bprivacy policy\b|\bcookie policy\b)", std::regex_constants::icase),
        std::regex(R"(\bclick here\b.*\bmore information\b)", std::regex_constants::icase),
        std::regex(R"(\bsubscribe\b.*\bnewsletter\b)", std::regex_constants::icase),
        std::regex(R"(\bfollow us\b.*\bsocial media\b)", std::regex_constants::icase),
        std::regex(R"(\bpowered by\b|\bcreated by\b|\bdesigned by\b)", std::regex_constants::icase),
    };
    
    for (const auto& pattern : boilerplate_patterns) {
        if (std::regex_search(text, pattern)) {
            return true;
        }
    }
    
    return false;
}

double RepetitionFilter::calculate_pattern_diversity(const std::string& text) const {
    auto lines = TextUtils::split_lines(text);
    if (lines.empty()) return 0.0;
    
    std::unordered_set<std::string> unique_patterns;
    
    for (const auto& line : lines) {
        // Extract pattern (first few words or structural elements)
        auto words = TextUtils::split_words(line);
        if (!words.empty()) {
            std::string pattern = words[0];
            if (words.size() > 1) {
                pattern += " " + words[1];
            }
            unique_patterns.insert(pattern);
        }
    }
    
    return static_cast<double>(unique_patterns.size()) / lines.size();
}

bool RepetitionFilter::is_template_like(const std::string& text) const {
    auto lines = TextUtils::split_lines(text);
    if (lines.size() < 5) return false;
    
    // Check for repeated structural patterns
    std::unordered_map<std::string, size_t> patterns;
    
    for (const auto& line : lines) {
        // Extract structural pattern
        std::string pattern;
        for (char c : line) {
            if (std::isalpha(c)) pattern += 'A';
            else if (std::isdigit(c)) pattern += '9';
            else if (std::isspace(c)) pattern += ' ';
            else pattern += 'X';
        }
        
        pattern = TextUtils::normalize_whitespace(pattern);
        if (!pattern.empty()) {
            patterns[pattern]++;
        }
    }
    
    // If more than 30% of lines follow the same pattern, it's template-like
    size_t max_pattern_count = 0;
    for (const auto& pair : patterns) {
        max_pattern_count = std::max(max_pattern_count, pair.second);
    }
    
    return static_cast<double>(max_pattern_count) / lines.size() > 0.3;
}

bool RepetitionFilter::has_repetitive_structure(const std::string& text) const {
    return calculate_pattern_diversity(text) < 0.4;
}

double RepetitionFilter::calculate_lexical_diversity(const std::string& text) const {
    auto words = TextUtils::split_words(text);
    if (words.empty()) return 0.0;
    
    std::unordered_set<std::string> unique_words;
    for (const auto& word : words) {
        std::string lower_word = word;
        std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
        unique_words.insert(lower_word);
    }
    
    return static_cast<double>(unique_words.size()) / words.size();
}

double RepetitionFilter::calculate_type_token_ratio(const std::string& text) const {
    return calculate_lexical_diversity(text);  // Same calculation
}

} // namespace quality
} // namespace rapidsift 