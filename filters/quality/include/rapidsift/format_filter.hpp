#pragma once

#include "quality_common.hpp"
#include <regex>

namespace rapidsift {
namespace quality {

class FormatFilter : public QualityFilter {
private:
    FormatFilterConfig config_;
    std::vector<std::regex> unwanted_pattern_regexes_;
    std::regex html_tag_regex_;
    std::regex code_pattern_regex_;
    
    void compile_patterns();
    
public:
    FormatFilter() = default;
    explicit FormatFilter(const FormatFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "FormatFilter"; }
    void configure(const QualityConfig& config) override;
    
    // HTML and markup checks
    bool has_excessive_html(const std::string& text) const;
    double calculate_html_ratio(const std::string& text) const;
    size_t count_html_tags(const std::string& text) const;
    bool is_mostly_markup(const std::string& text) const;
    
    // Code detection
    bool has_excessive_code(const std::string& text) const;
    double calculate_code_ratio(const std::string& text) const;
    bool appears_to_be_code(const std::string& text) const;
    bool contains_programming_keywords(const std::string& text) const;
    
    // Line structure analysis
    bool has_poor_line_structure(const std::string& text) const;
    double calculate_single_line_ratio(const std::string& text) const;
    bool has_broken_line_wrapping(const std::string& text) const;
    bool is_poetry_like(const std::string& text) const;
    
    // List and bullet detection
    bool is_list_like(const std::string& text) const;
    double calculate_list_ratio(const std::string& text) const;
    bool has_excessive_bullets(const std::string& text) const;
    
    // Formatting pattern analysis
    bool matches_unwanted_patterns(const std::string& text) const;
    bool has_inconsistent_formatting(const std::string& text) const;
    bool appears_extracted_poorly(const std::string& text) const;
    
    // Structure validation
    bool has_coherent_structure(const std::string& text) const;
    double calculate_sentence_structure_score(const std::string& text) const;
    bool has_reasonable_paragraph_structure(const std::string& text) const;
    
    // Special content detection
    bool is_navigation_content(const std::string& text) const;
    bool is_error_page(const std::string& text) const;
    bool is_form_content(const std::string& text) const;
    
    // Configuration accessors
    const FormatFilterConfig& get_config() const { return config_; }
    void set_config(const FormatFilterConfig& config);
};

} // namespace quality
} // namespace rapidsift 