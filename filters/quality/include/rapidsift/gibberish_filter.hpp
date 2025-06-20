#pragma once

#include "quality_common.hpp"
#include <regex>

namespace rapidsift {
namespace quality {

class GibberishFilter : public QualityFilter {
private:
    GibberishFilterConfig config_;
    std::vector<std::regex> gibberish_regexes_;
    
    void compile_patterns();
    
public:
    GibberishFilter() = default;
    explicit GibberishFilter(const GibberishFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "GibberishFilter"; }
    void configure(const QualityConfig& config) override;
    
    // Specific gibberish checks
    bool has_excessive_non_alpha(const std::string& text) const;
    bool has_excessive_digits(const std::string& text) const;
    bool has_excessive_symbols(const std::string& text) const;
    bool has_excessive_repetition(const std::string& text) const;
    bool has_long_consecutive_chars(const std::string& text) const;
    bool has_low_entropy(const std::string& text) const;
    bool matches_gibberish_patterns(const std::string& text) const;
    
    // Character analysis
    double calculate_character_entropy(const std::string& text) const;
    double get_repetition_ratio(const std::string& text) const;
    size_t find_longest_consecutive_run(const std::string& text) const;
    
    // Linguistic checks
    bool appears_linguistic(const std::string& text) const;
    double calculate_vowel_consonant_ratio(const std::string& text) const;
    
    // Configuration accessors
    const GibberishFilterConfig& get_config() const { return config_; }
    void set_config(const GibberishFilterConfig& config);
};

} // namespace quality
} // namespace rapidsift 