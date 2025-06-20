#pragma once

#include "quality_common.hpp"

namespace rapidsift {
namespace quality {

class LengthFilter : public QualityFilter {
private:
    LengthFilterConfig config_;
    
public:
    LengthFilter() = default;
    explicit LengthFilter(const LengthFilterConfig& config);
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "LengthFilter"; }
    void configure(const QualityConfig& config) override;
    
    // Specific length checks
    bool is_too_short(const Document& doc) const;
    bool is_too_long(const Document& doc) const;
    
    // Utility functions
    size_t count_words(const std::string& text) const;
    size_t count_characters(const std::string& text) const;
    
    // Configuration accessors
    const LengthFilterConfig& get_config() const { return config_; }
    void set_config(const LengthFilterConfig& config) { config_ = config; }
};

} // namespace quality
} // namespace rapidsift 