#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <regex>

#include "common.hpp"

namespace rapidsift {

/**
 * Configuration for text extraction
 */
struct TextExtractionConfig {
    // HTML parsing options
    bool remove_scripts = true;           // Remove <script> tags and content
    bool remove_styles = true;            // Remove <style> tags and content
    bool remove_comments = true;          // Remove HTML comments
    bool remove_navigation = true;        // Remove navigation elements
    bool remove_headers_footers = true;   // Remove header/footer sections
    bool remove_ads = true;              // Remove advertisement blocks
    bool remove_forms = true;            // Remove form elements
    bool remove_metadata = true;         // Remove meta tags and hidden content
    
    // Content filtering
    bool extract_main_content = true;     // Focus on main content areas
    bool preserve_paragraphs = true;     // Keep paragraph structure
    bool preserve_lists = true;          // Keep list structure
    bool preserve_headings = true;       // Keep heading structure
    bool preserve_links = false;         // Keep link text (remove URLs)
    bool preserve_tables = false;        // Extract table content
    
    // Text cleaning options
    bool normalize_whitespace = true;    // Normalize spaces, tabs, newlines
    bool normalize_unicode = true;       // Fix Unicode encoding issues
    bool decode_html_entities = true;    // Convert &amp; → &, etc.
    bool remove_extra_newlines = true;   // Remove excessive line breaks
    bool trim_lines = true;              // Trim whitespace from line ends
    
    // Content quality filters
    size_t min_paragraph_length = 20;    // Minimum paragraph length
    size_t min_total_length = 100;       // Minimum total text length
    double min_text_ratio = 0.3;         // Minimum text/HTML ratio
    size_t max_link_density = 5;         // Max links per 100 words
    
    // Language and encoding
    std::string default_encoding = "utf-8";
    bool auto_detect_encoding = true;
    bool fix_mojibake = true;            // Fix common encoding issues
};

/**
 * Results from text extraction operation
 */
struct TextExtractionResult {
    std::string extracted_text;           // Main extracted content
    std::string title;                   // Page title
    std::string language;                // Detected language (if available)
    std::string url;                     // Original URL (if provided)
    
    // Quality metrics
    size_t original_html_length = 0;
    size_t extracted_text_length = 0;
    double text_ratio = 0.0;             // text_length / html_length
    size_t paragraph_count = 0;
    size_t link_count = 0;
    double link_density = 0.0;           // links per 100 words
    
    // Extraction metadata
    std::vector<std::string> headings;   // Extracted headings
    std::vector<std::string> links;      // Extracted links (if preserved)
    std::unordered_map<std::string, std::string> metadata; // Meta tags
    
    bool is_valid() const {
        return extracted_text_length >= 50 && text_ratio >= 0.1;
    }
    
    double quality_score() const {
        double score = 0.0;
        score += std::min(text_ratio * 2.0, 1.0) * 0.4;  // Text ratio (40%)
        score += std::min(double(paragraph_count) / 10.0, 1.0) * 0.3;  // Paragraph count (30%)
        score += (1.0 - std::min(link_density / 10.0, 1.0)) * 0.3;  // Low link density (30%)
        return score;
    }
};

/**
 * HTML element information
 */
struct HtmlElement {
    std::string tag;
    std::string text;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<std::shared_ptr<HtmlElement>> children;
    std::weak_ptr<HtmlElement> parent;
    
    bool has_attribute(const std::string& attr) const {
        return attributes.find(attr) != attributes.end();
    }
    
    std::string get_attribute(const std::string& attr, const std::string& default_val = "") const {
        auto it = attributes.find(attr);
        return it != attributes.end() ? it->second : default_val;
    }
    
    bool has_class(const std::string& class_name) const {
        auto it = attributes.find("class");
        if (it == attributes.end()) return false;
        return it->second.find(class_name) != std::string::npos;
    }
    
    bool has_id(const std::string& id) const {
        auto it = attributes.find("id");
        return it != attributes.end() && it->second == id;
    }
};

/**
 * Simple HTML parser for text extraction
 */
class HtmlParser {
private:
    std::string html_;
    size_t pos_ = 0;
    
    // Helper methods
    void skip_whitespace();
    std::string parse_tag_name();
    std::unordered_map<std::string, std::string> parse_attributes();
    std::string parse_text();
    std::shared_ptr<HtmlElement> parse_element();
    
public:
    explicit HtmlParser(const std::string& html) : html_(html) {}
    
    std::shared_ptr<HtmlElement> parse();
    
    // Static utility methods
    static std::string extract_title(const std::string& html);
    static std::string extract_meta_content(const std::string& html, const std::string& name);
    static std::vector<std::string> extract_meta_tags(const std::string& html);
    static std::string detect_encoding(const std::string& html);
};

/**
 * Boilerplate detection and removal
 */
class BoilerplateRemover {
public:
    // Content scoring and detection methods (public for testing)
    double calculate_content_score(const std::shared_ptr<HtmlElement>& element) const;
    bool is_boilerplate_element(const std::shared_ptr<HtmlElement>& element) const;
    bool is_navigation_element(const std::shared_ptr<HtmlElement>& element) const;
    bool is_advertisement_element(const std::shared_ptr<HtmlElement>& element) const;
    bool is_content_element(const std::shared_ptr<HtmlElement>& element) const;

private:
    TextExtractionConfig config_;
    
    // Boilerplate detection patterns
    std::unordered_set<std::string> boilerplate_tags_;
    std::unordered_set<std::string> boilerplate_classes_;
    std::unordered_set<std::string> boilerplate_ids_;
    std::vector<std::regex> boilerplate_patterns_;
    
    void initialize_patterns();
    
public:
    explicit BoilerplateRemover(const TextExtractionConfig& config = TextExtractionConfig{});
    
    std::shared_ptr<HtmlElement> remove_boilerplate(std::shared_ptr<HtmlElement> root) const;
    std::vector<std::shared_ptr<HtmlElement>> find_main_content(const std::shared_ptr<HtmlElement>& root) const;
    double calculate_text_density(const std::shared_ptr<HtmlElement>& element) const;
};

/**
 * Text cleaner for post-processing extracted text
 */
class TextCleaner {
private:
    TextExtractionConfig config_;
    
    // Cleaning patterns
    std::vector<std::pair<std::regex, std::string>> cleaning_patterns_;
    std::unordered_map<std::string, std::string> html_entities_;
    
    void initialize_patterns();
    void initialize_html_entities();
    
public:
    explicit TextCleaner(const TextExtractionConfig& config = TextExtractionConfig{});
    
    std::string clean_text(const std::string& text) const;
    std::string decode_html_entities(const std::string& text) const;
    std::string normalize_unicode(const std::string& text) const;
    std::string normalize_whitespace(const std::string& text) const;
    std::string fix_mojibake(const std::string& text) const;
    std::string remove_extra_newlines(const std::string& text) const;
};

/**
 * Main text extractor class
 */
class TextExtractor {
private:
    TextExtractionConfig config_;
    std::unique_ptr<HtmlParser> parser_;
    std::unique_ptr<BoilerplateRemover> boilerplate_remover_;
    std::unique_ptr<TextCleaner> text_cleaner_;
    
    // Helper methods
    std::string extract_text_from_element(const std::shared_ptr<HtmlElement>& element) const;
    std::vector<std::string> extract_headings(const std::shared_ptr<HtmlElement>& element) const;
    std::vector<std::string> extract_links(const std::shared_ptr<HtmlElement>& element) const;
    void calculate_quality_metrics(TextExtractionResult& result) const;
    
public:
    explicit TextExtractor(const TextExtractionConfig& config = TextExtractionConfig{});
    ~TextExtractor() = default;
    
    // Move constructor and assignment
    TextExtractor(TextExtractor&& other) noexcept;
    TextExtractor& operator=(TextExtractor&& other) noexcept;
    
    // Disable copy constructor and assignment
    TextExtractor(const TextExtractor&) = delete;
    TextExtractor& operator=(const TextExtractor&) = delete;
    
    /**
     * Extract text from HTML content
     */
    TextExtractionResult extract(const std::string& html, const std::string& url = "") const;
    
    /**
     * Extract text from multiple HTML documents
     */
    std::vector<TextExtractionResult> extract_batch(
        const std::vector<std::string>& html_documents,
        const std::vector<std::string>& urls = {}
    ) const;
    
    /**
     * Extract text with progress callback
     */
    std::vector<TextExtractionResult> extract_batch(
        const std::vector<std::string>& html_documents,
        std::function<void(size_t, size_t, const std::string&)> progress_callback,
        const std::vector<std::string>& urls = {}
    ) const;
    
    /**
     * Configuration getters/setters
     */
    const TextExtractionConfig& get_config() const { return config_; }
    void set_config(const TextExtractionConfig& config);
    
    /**
     * Utility methods for quick extraction
     */
    static std::string extract_text_simple(const std::string& html);
    static std::string extract_title(const std::string& html);
    static bool is_likely_html(const std::string& content);
};

/**
 * Utility functions for text extraction
 */
namespace extraction_utils {
    /**
     * Detect content type from string
     */
    enum class ContentType {
        HTML,
        XML,
        PLAIN_TEXT,
        JSON,
        UNKNOWN
    };
    
    ContentType detect_content_type(const std::string& content);
    
    /**
     * Encoding detection and conversion
     */
    std::string detect_encoding(const std::string& content);
    std::string convert_encoding(const std::string& content, const std::string& from, const std::string& to);
    
    /**
     * URL and link processing
     */
    std::string extract_domain(const std::string& url);
    std::string normalize_url(const std::string& url);
    bool is_external_link(const std::string& url, const std::string& base_domain);
    
    /**
     * Text quality assessment
     */
    double calculate_text_quality(const std::string& text);
    bool is_likely_boilerplate(const std::string& text);
    double calculate_readability_score(const std::string& text);
    
    /**
     * Language detection helpers
     */
    std::string detect_language_from_html(const std::string& html);
    std::string extract_lang_attribute(const std::string& html);
    
    /**
     * Common web content patterns
     */
    bool is_navigation_text(const std::string& text);
    bool is_advertisement_text(const std::string& text);
    bool is_copyright_text(const std::string& text);
    bool is_menu_text(const std::string& text);
    
    /**
     * File I/O helpers for web content
     */
    std::vector<std::string> load_html_files(const std::string& directory_path);
    void save_extracted_text(const std::vector<TextExtractionResult>& results, const std::string& output_path);
    void save_extraction_report(const std::vector<TextExtractionResult>& results, const std::string& report_path);
}

} // namespace rapidsift 