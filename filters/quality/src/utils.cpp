#include "rapidsift/quality_common.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <regex>
#include <sstream>
#include <unordered_map>

namespace rapidsift {
namespace quality {

std::vector<std::string> TextUtils::split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::vector<std::string> TextUtils::split_words(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream stream(text);
    std::string word;
    
    while (stream >> word) {
        // Remove punctuation from word boundaries
        while (!word.empty() && std::ispunct(word.front())) {
            word.erase(0, 1);
        }
        while (!word.empty() && std::ispunct(word.back())) {
            word.pop_back();
        }
        
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    
    return words;
}

std::string TextUtils::normalize_whitespace(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    
    bool in_whitespace = false;
    for (char c : text) {
        if (std::isspace(c)) {
            if (!in_whitespace) {
                result += ' ';
                in_whitespace = true;
            }
        } else {
            result += c;
            in_whitespace = false;
        }
    }
    
    // Trim leading and trailing whitespace
    size_t start = result.find_first_not_of(' ');
    if (start == std::string::npos) return "";
    
    size_t end = result.find_last_not_of(' ');
    return result.substr(start, end - start + 1);
}

std::string TextUtils::strip_html(const std::string& text) {
    std::regex html_regex("<[^>]*>");
    std::string result = std::regex_replace(text, html_regex, "");
    
    // Decode common HTML entities
    std::regex amp_regex("&amp;");
    result = std::regex_replace(result, amp_regex, "&");
    
    std::regex lt_regex("&lt;");
    result = std::regex_replace(result, lt_regex, "<");
    
    std::regex gt_regex("&gt;");
    result = std::regex_replace(result, gt_regex, ">");
    
    std::regex quot_regex("&quot;");
    result = std::regex_replace(result, quot_regex, "\"");
    
    std::regex nbsp_regex("&nbsp;");
    result = std::regex_replace(result, nbsp_regex, " ");
    
    return normalize_whitespace(result);
}

double TextUtils::calculate_entropy(const std::string& text) {
    if (text.empty()) return 0.0;
    
    std::unordered_map<char, int> freq;
    for (char c : text) {
        freq[c]++;
    }
    
    double entropy = 0.0;
    double length = static_cast<double>(text.length());
    
    for (const auto& pair : freq) {
        double probability = static_cast<double>(pair.second) / length;
        entropy -= probability * std::log2(probability);
    }
    
    return entropy;
}

double TextUtils::calculate_perplexity(const std::string& text) {
    // Simple character-level perplexity estimation
    auto words = split_words(text);
    if (words.empty()) return 0.0;
    
    std::unordered_map<std::string, int> word_freq;
    for (const auto& word : words) {
        word_freq[word]++;
    }
    
    double log_prob_sum = 0.0;
    for (const auto& word : words) {
        double prob = static_cast<double>(word_freq[word]) / words.size();
        log_prob_sum += std::log(prob);
    }
    
    double average_log_prob = log_prob_sum / words.size();
    return std::exp(-average_log_prob);
}

bool TextUtils::is_ascii(const std::string& text) {
    return std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return c < 128;
    });
}

double TextUtils::get_alpha_ratio(const std::string& text) {
    if (text.empty()) return 0.0;
    
    size_t alpha_count = 0;
    for (char c : text) {
        if (std::isalpha(c)) {
            alpha_count++;
        }
    }
    
    return static_cast<double>(alpha_count) / text.length();
}

double TextUtils::get_digit_ratio(const std::string& text) {
    if (text.empty()) return 0.0;
    
    size_t digit_count = 0;
    for (char c : text) {
        if (std::isdigit(c)) {
            digit_count++;
        }
    }
    
    return static_cast<double>(digit_count) / text.length();
}

double TextUtils::get_symbol_ratio(const std::string& text) {
    if (text.empty()) return 0.0;
    
    size_t symbol_count = 0;
    for (char c : text) {
        if (!std::isalnum(c) && !std::isspace(c)) {
            symbol_count++;
        }
    }
    
    return static_cast<double>(symbol_count) / text.length();
}

size_t TextUtils::count_consecutive_chars(const std::string& text, char target) {
    size_t max_consecutive = 0;
    size_t current_consecutive = 0;
    
    for (char c : text) {
        if (c == target) {
            current_consecutive++;
            max_consecutive = std::max(max_consecutive, current_consecutive);
        } else {
            current_consecutive = 0;
        }
    }
    
    return max_consecutive;
}

std::string TextUtils::extract_domain(const std::string& url) {
    std::regex domain_regex(R"(https?://(?:www\.)?([^/]+))");
    std::smatch match;
    
    if (std::regex_search(url, match, domain_regex) && match.size() > 1) {
        return match[1].str();
    }
    
    return "";
}

bool TextUtils::contains_code_patterns(const std::string& text) {
    // Common programming patterns
    std::vector<std::regex> code_patterns = {
        std::regex(R"(\b(function|class|import|from|def|var|let|const)\b)"),
        std::regex(R"(\{[^}]*\})"),  // Curly braces
        std::regex(R"(\([^)]*\)\s*\{)"),  // Function definitions
        std::regex(R"(#include|#define|#ifdef)"),  // C/C++ preprocessor
        std::regex(R"(\b(public|private|protected|static)\b)"),  // Access modifiers
        std::regex(R"([a-zA-Z_][a-zA-Z0-9_]*\s*=\s*[^;]+;)"),  // Assignments with semicolons
    };
    
    for (const auto& pattern : code_patterns) {
        if (std::regex_search(text, pattern)) {
            return true;
        }
    }
    
    return false;
}

bool TextUtils::is_list_like(const std::string& text) {
    auto lines = split_lines(text);
    if (lines.size() < 3) return false;
    
    size_t bullet_lines = 0;
    std::regex bullet_regex(R"(^\s*[-*•◦▪▫‣⁃]\s+)");
    std::regex numbered_regex(R"(^\s*\d+[.)]\s+)");
    
    for (const auto& line : lines) {
        if (std::regex_search(line, bullet_regex) || 
            std::regex_search(line, numbered_regex)) {
            bullet_lines++;
        }
    }
    
    double bullet_ratio = static_cast<double>(bullet_lines) / lines.size();
    return bullet_ratio > 0.6;  // More than 60% of lines are list items
}

} // namespace quality
} // namespace rapidsift 