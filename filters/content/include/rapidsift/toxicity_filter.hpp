#pragma once

#include "content_common.hpp"
#include <regex>
#include <unordered_map>

namespace rapidsift {
namespace content {

class ToxicityFilter : public ContentFilter {
private:
    ToxicityFilterConfig config_;
    
    // Compiled regex patterns for different toxicity categories
    std::vector<std::regex> profanity_regexes_;
    std::vector<std::regex> hate_speech_regexes_;
    std::vector<std::regex> violence_regexes_;
    std::vector<std::regex> nsfw_regexes_;
    std::vector<std::regex> harassment_regexes_;
    std::vector<std::regex> threat_regexes_;
    
    // Context detection patterns
    std::vector<std::regex> medical_context_regexes_;
    std::vector<std::regex> educational_context_regexes_;
    std::vector<std::regex> news_context_regexes_;
    std::vector<std::regex> legal_context_regexes_;
    
    void compile_patterns();
    void initialize_default_keywords();
    
public:
    ToxicityFilter() = default;
    explicit ToxicityFilter(const ToxicityFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "ToxicityFilter"; }
    void configure(const ContentConfig& config) override;
    
    // Main toxicity detection methods
    std::vector<ViolationInfo> detect_violations(const std::string& text) const;
    double calculate_toxicity_score(const std::string& text) const;
    std::vector<ToxicityCategory> get_toxicity_categories(const std::string& text) const;
    
    // Specific category detection
    bool contains_hate_speech(const std::string& text) const;
    bool contains_harassment(const std::string& text) const;
    bool contains_profanity(const std::string& text) const;
    bool contains_violence(const std::string& text) const;
    bool contains_nsfw_content(const std::string& text) const;
    bool contains_threats(const std::string& text) const;
    
    // Severity assessment
    ViolationSeverity assess_violation_severity(const ViolationInfo& violation) const;
    double calculate_category_score(const std::string& text, ToxicityCategory category) const;
    
    // Context analysis
    bool is_in_safe_context(const std::string& text, ToxicityCategory category) const;
    bool has_medical_context(const std::string& text) const;
    bool has_educational_context(const std::string& text) const;
    bool has_news_reporting_context(const std::string& text) const;
    bool has_legal_context(const std::string& text) const;
    
    // Advanced analysis
    bool is_hate_speech_pattern(const std::string& text) const;
    bool is_targeted_harassment(const std::string& text) const;
    bool is_graphic_violence(const std::string& text) const;
    bool is_explicit_sexual_content(const std::string& text) const;
    
    // Keyword management
    void add_profanity_keyword(const std::string& keyword);
    void add_hate_keyword(const std::string& keyword);
    void add_violence_keyword(const std::string& keyword);
    void load_keyword_lists(const std::string& directory);
    void save_keyword_lists(const std::string& directory) const;
    
    // ML integration (future)
    void set_ml_classifier_path(const std::string& model_path);
    double get_ml_toxicity_score(const std::string& text) const;
    
    // Configuration management
    const ToxicityFilterConfig& get_config() const { return config_; }
    void set_config(const ToxicityFilterConfig& config);
    
    // Statistics and analysis
    std::unordered_map<ToxicityCategory, size_t> get_detection_counts() const;
    void print_violation_summary(const std::vector<ViolationInfo>& violations) const;
    
private:
    // Internal helper methods
    std::vector<std::string> extract_potential_slurs(const std::string& text) const;
    bool is_false_positive(const std::string& text, const std::string& match) const;
    double calculate_context_modifier(const std::string& text, ToxicityCategory category) const;
    bool matches_pattern_list(const std::string& text, const std::vector<std::regex>& patterns) const;
};

} // namespace content
} // namespace rapidsift 