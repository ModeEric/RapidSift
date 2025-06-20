#pragma once

#include "quality_common.hpp"
#include <unordered_map>

namespace rapidsift {
namespace quality {

class RepetitionFilter : public QualityFilter {
private:
    RepetitionFilterConfig config_;
    
public:
    RepetitionFilter() = default;
    explicit RepetitionFilter(const RepetitionFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "RepetitionFilter"; }
    void configure(const QualityConfig& config) override;
    
    // Line-based repetition checks
    bool has_excessive_line_repetition(const std::string& text) const;
    double calculate_line_repetition_ratio(const std::string& text) const;
    std::unordered_map<std::string, size_t> count_lines(const std::string& text) const;
    
    // N-gram repetition checks
    bool has_excessive_ngram_repetition(const std::string& text) const;
    double calculate_ngram_repetition_ratio(const std::string& text, size_t n) const;
    std::vector<std::string> extract_ngrams(const std::string& text, size_t n) const;
    
    // Word uniqueness checks
    bool has_insufficient_unique_words(const std::string& text) const;
    double calculate_unique_word_ratio(const std::string& text) const;
    size_t count_unique_words(const std::string& text) const;
    
    // Boilerplate detection
    bool is_boilerplate(const std::string& text) const;
    double calculate_boilerplate_score(const std::string& text) const;
    bool contains_common_boilerplate_patterns(const std::string& text) const;
    
    // Pattern analysis
    double calculate_pattern_diversity(const std::string& text) const;
    bool is_template_like(const std::string& text) const;
    bool has_repetitive_structure(const std::string& text) const;
    
    // Statistical measures
    double calculate_lexical_diversity(const std::string& text) const;
    double calculate_type_token_ratio(const std::string& text) const;
    
    // Configuration accessors
    const RepetitionFilterConfig& get_config() const { return config_; }
    void set_config(const RepetitionFilterConfig& config) { config_ = config; }
};

} // namespace quality
} // namespace rapidsift 