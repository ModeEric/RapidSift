#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

#include "common.hpp"

namespace rapidsift {

/**
 * Language identification result containing detected language and confidence score
 */
struct LanguageDetection {
    std::string language;      // ISO 639-1 language code (e.g., "en", "fr", "de")
    double confidence;         // Confidence score [0.0, 1.0]
    
    LanguageDetection(const std::string& lang = "unknown", double conf = 0.0)
        : language(lang), confidence(conf) {}
};

/**
 * Configuration for language filtering
 */
struct LanguageFilterConfig {
    std::vector<std::string> target_languages;  // Languages to keep (e.g., {"en", "fr"})
    double min_confidence = 0.65;               // Minimum confidence threshold
    bool strict_mode = false;                   // If true, only keep documents with target languages
    bool remove_mixed_language = true;          // Remove documents with mixed languages
    size_t min_text_length = 10;               // Minimum text length for reliable detection
    
    // Advanced options
    bool enable_preprocessing = true;           // Clean text before detection
    bool normalize_scores = true;              // Normalize confidence scores
    double mixed_language_threshold = 0.3;    // Threshold for detecting mixed languages
};

/**
 * Results from language filtering operation
 */
struct LanguageFilterResult {
    std::vector<Document> filtered_documents;
    std::unordered_map<std::string, size_t> language_counts;
    std::vector<Document> rejected_documents;
    size_t total_processed = 0;
    size_t total_kept = 0;
    size_t total_rejected = 0;
    double average_confidence = 0.0;
    
    // Statistics
    double kept_percentage() const {
        return total_processed > 0 ? (double(total_kept) / total_processed) * 100.0 : 0.0;
    }
    
    double rejected_percentage() const {
        return total_processed > 0 ? (double(total_rejected) / total_processed) * 100.0 : 0.0;
    }
};

/**
 * Base class for language detection backends
 */
class LanguageDetector {
public:
    virtual ~LanguageDetector() = default;
    
    /**
     * Detect language of a single text
     */
    virtual LanguageDetection detect(const std::string& text) = 0;
    
    /**
     * Detect languages for multiple texts (can be optimized for batch processing)
     */
    virtual std::vector<LanguageDetection> detect_batch(const std::vector<std::string>& texts) {
        std::vector<LanguageDetection> results;
        results.reserve(texts.size());
        for (const auto& text : texts) {
            results.push_back(detect(text));
        }
        return results;
    }
    
    /**
     * Get supported languages
     */
    virtual std::vector<std::string> get_supported_languages() const = 0;
    
    /**
     * Check if detector is ready/loaded
     */
    virtual bool is_ready() const = 0;
};

/**
 * FastText-based language detector
 */
class FastTextLanguageDetector : public LanguageDetector {
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
    
public:
    explicit FastTextLanguageDetector(const std::string& model_path = "");
    ~FastTextLanguageDetector();
    
    // Move constructor and assignment
    FastTextLanguageDetector(FastTextLanguageDetector&& other) noexcept;
    FastTextLanguageDetector& operator=(FastTextLanguageDetector&& other) noexcept;
    
    // Disable copy constructor and assignment
    FastTextLanguageDetector(const FastTextLanguageDetector&) = delete;
    FastTextLanguageDetector& operator=(const FastTextLanguageDetector&) = delete;
    
    bool load_model(const std::string& model_path);
    LanguageDetection detect(const std::string& text) override;
    std::vector<LanguageDetection> detect_batch(const std::vector<std::string>& texts) override;
    std::vector<std::string> get_supported_languages() const override;
    bool is_ready() const override;
    
    // FastText specific methods
    void set_threshold(double threshold);
    void set_top_k(int k);
};

/**
 * Simple rule-based language detector (fallback when FastText is not available)
 */
class SimpleLanguageDetector : public LanguageDetector {
private:
    std::unordered_map<std::string, std::vector<std::string>> language_patterns_;
    std::unordered_map<std::string, std::vector<std::string>> language_stopwords_;
    
    void initialize_patterns();
    double calculate_language_score(const std::string& text, const std::string& language) const;
    
public:
    SimpleLanguageDetector();
    
    LanguageDetection detect(const std::string& text) override;
    std::vector<std::string> get_supported_languages() const override;
    bool is_ready() const override;
};

/**
 * Main language filtering class
 */
class LanguageFilter {
private:
    LanguageFilterConfig config_;
    std::unique_ptr<LanguageDetector> detector_;
    
    // Helper methods
    std::string preprocess_text(const std::string& text) const;
    bool should_keep_document(const LanguageDetection& detection) const;
    bool is_mixed_language(const std::string& text) const;
    
public:
    explicit LanguageFilter(const LanguageFilterConfig& config = LanguageFilterConfig{});
    ~LanguageFilter() = default;
    
    // Move constructor and assignment
    LanguageFilter(LanguageFilter&& other) noexcept;
    LanguageFilter& operator=(LanguageFilter&& other) noexcept;
    
    // Disable copy constructor and assignment
    LanguageFilter(const LanguageFilter&) = delete;
    LanguageFilter& operator=(const LanguageFilter&) = delete;
    
    /**
     * Set language detector (FastText, Simple, etc.)
     */
    void set_detector(std::unique_ptr<LanguageDetector> detector);
    
    /**
     * Filter documents based on language
     */
    LanguageFilterResult filter(const std::vector<Document>& documents) const;
    
    /**
     * Filter documents with progress callback
     */
    LanguageFilterResult filter(
        const std::vector<Document>& documents,
        std::function<void(size_t, size_t, const std::string&)> progress_callback
    ) const;
    
    /**
     * Detect language for a single document
     */
    LanguageDetection detect_language(const Document& document) const;
    
    /**
     * Get language statistics for a set of documents
     */
    std::unordered_map<std::string, size_t> get_language_stats(
        const std::vector<Document>& documents
    ) const;
    
    /**
     * Configuration getters/setters
     */
    const LanguageFilterConfig& get_config() const { return config_; }
    void set_config(const LanguageFilterConfig& config) { config_ = config; }
    
    /**
     * Check if filter is ready to use
     */
    bool is_ready() const;
    
    /**
     * Get supported languages from current detector
     */
    std::vector<std::string> get_supported_languages() const;
};

/**
 * Utility functions for language processing
 */
namespace language_utils {
    /**
     * Convert language codes between different formats
     */
    std::string iso639_1_to_name(const std::string& code);
    std::string name_to_iso639_1(const std::string& name);
    
    /**
     * Validate language code
     */
    bool is_valid_language_code(const std::string& code);
    
    /**
     * Clean text for language detection
     */
    std::string clean_text_for_detection(const std::string& text);
    
    /**
     * Detect if text contains multiple scripts/languages
     */
    bool has_mixed_scripts(const std::string& text);
    
    /**
     * Get language confidence based on text length and other factors
     */
    double adjust_confidence_for_length(double base_confidence, size_t text_length);
    
    /**
     * Download and cache language detection models
     */
    bool download_fasttext_model(const std::string& model_name, const std::string& cache_dir = "");
    
    /**
     * Get default model path for fastText language detection
     */
    std::string get_default_model_path();
}

} // namespace rapidsift 