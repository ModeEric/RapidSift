#include "rapidsift/length_filter.hpp"
#include "rapidsift/quality_common.hpp"
#include <sstream>

namespace rapidsift {
namespace quality {

LengthFilter::LengthFilter(const LengthFilterConfig& config) : config_(config) {}

void LengthFilter::configure(const QualityConfig& config) {
    config_ = config.length_config;
}

FilterDecision LengthFilter::evaluate(const Document& doc) const {
    FilterDecision decision(FilterResult::KEEP, RejectionReason::CUSTOM, 1.0, "");
    
    size_t word_count = count_words(doc.text);
    size_t char_count = count_characters(doc.text);
    
    // Store metrics
    decision.metrics["word_count"] = static_cast<double>(word_count);
    decision.metrics["char_count"] = static_cast<double>(char_count);
    
    // Check if too short
    if (is_too_short(doc)) {
        decision.result = FilterResult::REJECT;
        if (word_count < config_.min_words) {
            decision.reason = RejectionReason::TOO_SHORT;
            decision.details = "Document has " + std::to_string(word_count) + 
                             " words, minimum required: " + std::to_string(config_.min_words);
        } else {
            decision.reason = RejectionReason::TOO_SHORT;
            decision.details = "Document has " + std::to_string(char_count) + 
                             " characters, minimum required: " + std::to_string(config_.min_chars);
        }
        decision.confidence = 0.95;
        return decision;
    }
    
    // Check if too long
    if (is_too_long(doc)) {
        decision.result = FilterResult::REJECT;
        if (word_count > config_.max_words) {
            decision.reason = RejectionReason::TOO_LONG;
            decision.details = "Document has " + std::to_string(word_count) + 
                             " words, maximum allowed: " + std::to_string(config_.max_words);
        } else {
            decision.reason = RejectionReason::TOO_LONG;
            decision.details = "Document has " + std::to_string(char_count) + 
                             " characters, maximum allowed: " + std::to_string(config_.max_chars);
        }
        decision.confidence = 0.95;
        return decision;
    }
    
    // Calculate quality score based on length characteristics
    double score = 1.0;
    
    // Prefer documents with reasonable length
    if (word_count < 20) {
        score *= 0.8; // Slightly penalize very short documents
    } else if (word_count > 10000) {
        score *= 0.9; // Slightly penalize very long documents
    }
    
    decision.confidence = score;
    decision.details = "Document length acceptable: " + std::to_string(word_count) + " words, " +
                      std::to_string(char_count) + " characters";
    
    return decision;
}

bool LengthFilter::is_too_short(const Document& doc) const {
    size_t word_count = count_words(doc.text);
    size_t char_count = count_characters(doc.text);
    
    if (config_.strict_mode) {
        return word_count < config_.min_words || char_count < config_.min_chars;
    } else {
        return word_count < config_.min_words && char_count < config_.min_chars;
    }
}

bool LengthFilter::is_too_long(const Document& doc) const {
    size_t word_count = count_words(doc.text);
    size_t char_count = count_characters(doc.text);
    
    if (config_.strict_mode) {
        return word_count > config_.max_words || char_count > config_.max_chars;
    } else {
        return word_count > config_.max_words && char_count > config_.max_chars;
    }
}

size_t LengthFilter::count_words(const std::string& text) const {
    auto words = TextUtils::split_words(text);
    return words.size();
}

size_t LengthFilter::count_characters(const std::string& text) const {
    // Count non-whitespace characters
    size_t count = 0;
    for (char c : text) {
        if (!std::isspace(c)) {
            count++;
        }
    }
    return count;
}

} // namespace quality
} // namespace rapidsift 