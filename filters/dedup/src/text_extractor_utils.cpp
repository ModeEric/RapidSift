#include "rapidsift/text_extractor.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <regex>
#include <cmath>

namespace rapidsift {
namespace extraction_utils {

ContentType detect_content_type(const std::string& content) {
    if (content.empty()) return ContentType::UNKNOWN;
    
    // Check for HTML indicators
    if (content.find("<!DOCTYPE html") != std::string::npos ||
        content.find("<html") != std::string::npos ||
        content.find("<head>") != std::string::npos ||
        content.find("<body>") != std::string::npos ||
        (content.find('<') != std::string::npos && 
         content.find('>') != std::string::npos &&
         content.find("</") != std::string::npos)) {
        return ContentType::HTML;
    }
    
    // Check for XML indicators
    if (content.find("<?xml") != std::string::npos ||
        (content.find('<') != std::string::npos && 
         content.find('>') != std::string::npos &&
         content.find("xmlns") != std::string::npos)) {
        return ContentType::XML;
    }
    
    // Check for JSON indicators
    std::string trimmed = content;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
    
    if ((trimmed.front() == '{' && trimmed.back() == '}') ||
        (trimmed.front() == '[' && trimmed.back() == ']')) {
        return ContentType::JSON;
    }
    
    return ContentType::PLAIN_TEXT;
}

std::string detect_encoding(const std::string& content) {
    // Look for BOM (Byte Order Mark)
    if (content.length() >= 3) {
        if (content.substr(0, 3) == "\xEF\xBB\xBF") {
            return "utf-8";
        }
    }
    
    if (content.length() >= 2) {
        if (content.substr(0, 2) == "\xFF\xFE") {
            return "utf-16le";
        }
        if (content.substr(0, 2) == "\xFE\xFF") {
            return "utf-16be";
        }
    }
    
    // Look for charset declaration in HTML/XML
    std::regex charset_regex(R"(charset\s*=\s*["\']?([^"\'>\s]+))", std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(content, match, charset_regex)) {
        std::string encoding = match[1].str();
        std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::tolower);
        return encoding;
    }
    
    // Simple heuristic: check for high-bit characters
    size_t high_bit_chars = 0;
    for (char c : content) {
        if (static_cast<unsigned char>(c) > 127) {
            high_bit_chars++;
        }
    }
    
    if (high_bit_chars == 0) {
        return "ascii";
    }
    
    // Default to UTF-8 for modern content
    return "utf-8";
}

std::string convert_encoding(const std::string& content, const std::string& from, const std::string& to) {
    // Simplified encoding conversion - in a full implementation,
    // you'd use ICU or iconv
    if (from == to) {
        return content;
    }
    
    // For now, just return the original content
    // In production, implement proper encoding conversion
    return content;
}

std::string extract_domain(const std::string& url) {
    std::regex url_regex(R"(^https?://([^/]+))");
    std::smatch match;
    
    if (std::regex_search(url, match, url_regex)) {
        return match[1].str();
    }
    
    return "";
}

std::string normalize_url(const std::string& url) {
    std::string normalized = url;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove fragment
    size_t fragment_pos = normalized.find('#');
    if (fragment_pos != std::string::npos) {
        normalized = normalized.substr(0, fragment_pos);
    }
    
    // Remove trailing slash
    if (normalized.length() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    // Remove common tracking parameters
    std::vector<std::string> tracking_params = {
        "utm_source", "utm_medium", "utm_campaign", "utm_term", "utm_content",
        "fbclid", "gclid", "ref", "source"
    };
    
    for (const auto& param : tracking_params) {
        std::regex param_regex("&?" + param + "=[^&]*");
        normalized = std::regex_replace(normalized, param_regex, "");
    }
    
    // Clean up query string
    if (normalized.find('?') != std::string::npos && normalized.back() == '?') {
        normalized.pop_back();
    }
    
    return normalized;
}

bool is_external_link(const std::string& url, const std::string& base_domain) {
    if (url.empty() || base_domain.empty()) return false;
    
    // Relative URLs are internal
    if (url[0] == '/' || url[0] == '#' || url[0] == '?') {
        return false;
    }
    
    std::string link_domain = extract_domain(url);
    return !link_domain.empty() && link_domain != base_domain;
}

double calculate_text_quality(const std::string& text) {
    if (text.empty()) return 0.0;
    
    double quality = 0.0;
    
    // Length factor (normalized)
    double length_score = std::min(text.length() / 1000.0, 1.0);
    quality += length_score * 0.3;
    
    // Sentence structure (periods, question marks, exclamations)
    size_t sentence_endings = 0;
    for (char c : text) {
        if (c == '.' || c == '?' || c == '!') {
            sentence_endings++;
        }
    }
    
    double sentence_score = std::min(sentence_endings / 10.0, 1.0);
    quality += sentence_score * 0.2;
    
    // Word diversity (rough estimate)
    std::istringstream iss(text);
    std::unordered_set<std::string> unique_words;
    std::string word;
    size_t total_words = 0;
    
    while (iss >> word) {
        // Remove punctuation
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        if (!word.empty()) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            unique_words.insert(word);
            total_words++;
        }
    }
    
    double diversity_score = total_words > 0 ? 
        double(unique_words.size()) / total_words : 0.0;
    quality += diversity_score * 0.3;
    
    // Capitalization (proper sentences should start with capital letters)
    std::regex sentence_start(R"(\. [A-Z])");
    std::sregex_iterator iter(text.begin(), text.end(), sentence_start);
    std::sregex_iterator end;
    size_t proper_sentences = std::distance(iter, end);
    
    double capitalization_score = sentence_endings > 0 ? 
        double(proper_sentences) / sentence_endings : 0.0;
    quality += capitalization_score * 0.2;
    
    return std::min(quality, 1.0);
}

bool is_likely_boilerplate(const std::string& text) {
    if (text.length() < 10) return true;
    
    // Convert to lowercase for pattern matching
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    // Common boilerplate patterns
    std::vector<std::string> boilerplate_patterns = {
        "click here", "read more", "continue reading", "learn more",
        "privacy policy", "terms of service", "cookie policy",
        "all rights reserved", "copyright", "subscribe to",
        "follow us", "share this", "like us on facebook",
        "sign up", "newsletter", "advertisement", "sponsored"
    };
    
    size_t pattern_matches = 0;
    for (const auto& pattern : boilerplate_patterns) {
        if (lower_text.find(pattern) != std::string::npos) {
            pattern_matches++;
        }
    }
    
    // If more than 20% of patterns match, likely boilerplate
    return double(pattern_matches) / boilerplate_patterns.size() > 0.2;
}

double calculate_readability_score(const std::string& text) {
    if (text.empty()) return 0.0;
    
    // Simple readability score based on sentence and word length
    std::istringstream iss(text);
    std::string sentence;
    
    size_t total_sentences = 0;
    size_t total_words = 0;
    size_t total_syllables = 0;
    
    // Split by sentence endings
    std::regex sentence_regex(R"([.!?]+)");
    std::sregex_token_iterator iter(text.begin(), text.end(), sentence_regex, -1);
    std::sregex_token_iterator end;
    
    for (; iter != end; ++iter) {
        std::string sentence = iter->str();
        if (sentence.length() < 5) continue; // Skip very short "sentences"
        
        total_sentences++;
        
        std::istringstream sentence_stream(sentence);
        std::string word;
        
        while (sentence_stream >> word) {
            // Remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (!word.empty()) {
                total_words++;
                
                // Simple syllable counting (count vowel groups)
                size_t syllables = 0;
                bool prev_vowel = false;
                
                for (char c : word) {
                    bool is_vowel = (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
                                   c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
                    
                    if (is_vowel && !prev_vowel) {
                        syllables++;
                    }
                    prev_vowel = is_vowel;
                }
                
                total_syllables += std::max(syllables, size_t(1)); // At least 1 syllable per word
            }
        }
    }
    
    if (total_sentences == 0 || total_words == 0) return 0.0;
    
    // Simplified Flesch Reading Ease formula
    double avg_sentence_length = double(total_words) / total_sentences;
    double avg_syllables_per_word = double(total_syllables) / total_words;
    
    double flesch_score = 206.835 - (1.015 * avg_sentence_length) - (84.6 * avg_syllables_per_word);
    
    // Normalize to 0-1 range (Flesch scores typically range from 0-100)
    return std::max(0.0, std::min(flesch_score / 100.0, 1.0));
}

std::string detect_language_from_html(const std::string& html) {
    // Look for lang attribute in html tag
    std::regex html_lang(R"(<html[^>]+lang\s*=\s*["\']([^"\']+)["\'])", std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, html_lang)) {
        return match[1].str();
    }
    
    return "";
}

std::string extract_lang_attribute(const std::string& html) {
    return detect_language_from_html(html);
}

bool is_navigation_text(const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    std::vector<std::string> nav_patterns = {
        "home", "about", "contact", "services", "products", "blog",
        "news", "events", "gallery", "portfolio", "team", "careers",
        "login", "register", "search", "menu", "navigation", "sitemap"
    };
    
    for (const auto& pattern : nav_patterns) {
        if (lower_text.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool is_advertisement_text(const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    std::vector<std::string> ad_patterns = {
        "advertisement", "sponsored", "ads by", "promoted", 
        "buy now", "shop now", "sale", "discount", "offer",
        "click here", "learn more", "sign up now", "free trial"
    };
    
    for (const auto& pattern : ad_patterns) {
        if (lower_text.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool is_copyright_text(const std::string& text) {
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    return lower_text.find("copyright") != std::string::npos ||
           lower_text.find("all rights reserved") != std::string::npos ||
           lower_text.find("©") != std::string::npos ||
           lower_text.find("(c)") != std::string::npos;
}

bool is_menu_text(const std::string& text) {
    if (text.length() > 100) return false; // Menus are typically short
    
    // Count the number of separator characters (common in menus)
    size_t separators = 0;
    for (char c : text) {
        if (c == '|' || c == '*' || c == '-' || c == '>' || c == '/') {
            separators++;
        }
    }
    
    // If more than 20% of characters are separators, likely a menu
    return double(separators) / text.length() > 0.2;
}

std::vector<std::string> load_html_files(const std::string& directory_path) {
    std::vector<std::string> html_contents;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".html" || extension == ".htm") {
                    std::ifstream file(entry.path());
                    if (file.is_open()) {
                        std::ostringstream content;
                        content << file.rdbuf();
                        html_contents.push_back(content.str());
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
    
    return html_contents;
}

void save_extracted_text(const std::vector<TextExtractionResult>& results, const std::string& output_path) {
    std::ofstream output_file(output_path);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file: " << output_path << std::endl;
        return;
    }
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        output_file << "=== Document " << (i + 1) << " ===\n";
        if (!result.url.empty()) {
            output_file << "URL: " << result.url << "\n";
        }
        if (!result.title.empty()) {
            output_file << "Title: " << result.title << "\n";
        }
        output_file << "Length: " << result.extracted_text_length << " characters\n";
        output_file << "Quality Score: " << std::fixed << std::setprecision(3) << result.quality_score() << "\n";
        output_file << "\n" << result.extracted_text << "\n\n";
    }
    
    output_file.close();
    std::cout << "Extracted text saved to: " << output_path << std::endl;
}

void save_extraction_report(const std::vector<TextExtractionResult>& results, const std::string& report_path) {
    std::ofstream report_file(report_path);
    if (!report_file.is_open()) {
        std::cerr << "Error: Could not open report file: " << report_path << std::endl;
        return;
    }
    
    // Calculate summary statistics
    size_t total_documents = results.size();
    size_t valid_documents = 0;
    size_t total_html_length = 0;
    size_t total_text_length = 0;
    double total_quality_score = 0.0;
    
    for (const auto& result : results) {
        if (result.is_valid()) {
            valid_documents++;
        }
        total_html_length += result.original_html_length;
        total_text_length += result.extracted_text_length;
        total_quality_score += result.quality_score();
    }
    
    report_file << "Text Extraction Report\n";
    report_file << "=====================\n\n";
    
    report_file << "Summary Statistics:\n";
    report_file << "- Total documents processed: " << total_documents << "\n";
    report_file << "- Valid extractions: " << valid_documents << " (" 
                << std::fixed << std::setprecision(1) 
                << (100.0 * valid_documents / total_documents) << "%)\n";
    report_file << "- Total HTML size: " << total_html_length << " characters\n";
    report_file << "- Total extracted text: " << total_text_length << " characters\n";
    report_file << "- Overall text ratio: " 
                << std::fixed << std::setprecision(3)
                << (total_html_length > 0 ? double(total_text_length) / total_html_length : 0.0) << "\n";
    report_file << "- Average quality score: " 
                << std::fixed << std::setprecision(3)
                << (total_documents > 0 ? total_quality_score / total_documents : 0.0) << "\n\n";
    
    report_file << "Individual Document Results:\n";
    report_file << "----------------------------\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        report_file << "Document " << (i + 1) << ":\n";
        if (!result.url.empty()) {
            report_file << "  URL: " << result.url << "\n";
        }
        if (!result.title.empty()) {
            report_file << "  Title: " << result.title << "\n";
        }
        report_file << "  HTML length: " << result.original_html_length << "\n";
        report_file << "  Text length: " << result.extracted_text_length << "\n";
        report_file << "  Text ratio: " << std::fixed << std::setprecision(3) << result.text_ratio << "\n";
        report_file << "  Quality score: " << std::fixed << std::setprecision(3) << result.quality_score() << "\n";
        report_file << "  Valid: " << (result.is_valid() ? "Yes" : "No") << "\n";
        report_file << "\n";
    }
    
    report_file.close();
    std::cout << "Extraction report saved to: " << report_path << std::endl;
}

} // namespace extraction_utils
} // namespace rapidsift 