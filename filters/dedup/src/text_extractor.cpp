#include "rapidsift/text_extractor.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <filesystem>

namespace rapidsift {

// ==============================================================================
// HTML Parser Implementation
// ==============================================================================

void HtmlParser::skip_whitespace() {
    while (pos_ < html_.length() && std::isspace(html_[pos_])) {
        pos_++;
    }
}

std::string HtmlParser::parse_tag_name() {
    std::string tag_name;
    while (pos_ < html_.length() && 
           (std::isalnum(html_[pos_]) || html_[pos_] == '-' || html_[pos_] == '_')) {
        tag_name += std::tolower(html_[pos_]);
        pos_++;
    }
    return tag_name;
}

std::unordered_map<std::string, std::string> HtmlParser::parse_attributes() {
    std::unordered_map<std::string, std::string> attributes;
    
    while (pos_ < html_.length() && html_[pos_] != '>' && html_[pos_] != '/') {
        skip_whitespace();
        if (pos_ >= html_.length() || html_[pos_] == '>' || html_[pos_] == '/') break;
        
        // Parse attribute name
        std::string attr_name;
        while (pos_ < html_.length() && 
               (std::isalnum(html_[pos_]) || html_[pos_] == '-' || html_[pos_] == '_')) {
            attr_name += std::tolower(html_[pos_]);
            pos_++;
        }
        
        skip_whitespace();
        std::string attr_value;
        
        if (pos_ < html_.length() && html_[pos_] == '=') {
            pos_++; // Skip '='
            skip_whitespace();
            
            if (pos_ < html_.length() && (html_[pos_] == '"' || html_[pos_] == '\'')) {
                char quote = html_[pos_];
                pos_++; // Skip opening quote
                
                while (pos_ < html_.length() && html_[pos_] != quote) {
                    attr_value += html_[pos_];
                    pos_++;
                }
                
                if (pos_ < html_.length()) pos_++; // Skip closing quote
            } else {
                // Unquoted attribute value
                while (pos_ < html_.length() && !std::isspace(html_[pos_]) && 
                       html_[pos_] != '>' && html_[pos_] != '/') {
                    attr_value += html_[pos_];
                    pos_++;
                }
            }
        }
        
        if (!attr_name.empty()) {
            attributes[attr_name] = attr_value;
        }
    }
    
    return attributes;
}

std::string HtmlParser::parse_text() {
    std::string text;
    while (pos_ < html_.length() && html_[pos_] != '<') {
        text += html_[pos_];
        pos_++;
    }
    return text;
}

std::shared_ptr<HtmlElement> HtmlParser::parse_element() {
    if (pos_ >= html_.length()) return nullptr;
    
    auto element = std::make_shared<HtmlElement>();
    
    if (html_[pos_] == '<') {
        pos_++; // Skip '<'
        skip_whitespace();
        
        // Check for closing tag
        if (pos_ < html_.length() && html_[pos_] == '/') {
            return nullptr; // This is a closing tag, handled by parent
        }
        
        // Parse tag name
        element->tag = parse_tag_name();
        skip_whitespace();
        
        // Parse attributes
        element->attributes = parse_attributes();
        skip_whitespace();
        
        // Check for self-closing tag
        bool self_closing = false;
        if (pos_ < html_.length() && html_[pos_] == '/') {
            self_closing = true;
            pos_++;
        }
        
        if (pos_ < html_.length() && html_[pos_] == '>') {
            pos_++; // Skip '>'
        }
        
        if (!self_closing) {
            // Parse content and children
            while (pos_ < html_.length()) {
                skip_whitespace();
                if (pos_ >= html_.length()) break;
                
                if (html_[pos_] == '<') {
                    // Check for closing tag
                    size_t saved_pos = pos_;
                    pos_++; // Skip '<'
                    skip_whitespace();
                    
                    if (pos_ < html_.length() && html_[pos_] == '/') {
                        pos_++; // Skip '/'
                        skip_whitespace();
                        std::string closing_tag = parse_tag_name();
                        
                        if (closing_tag == element->tag) {
                            // Found matching closing tag
                            while (pos_ < html_.length() && html_[pos_] != '>') pos_++;
                            if (pos_ < html_.length()) pos_++; // Skip '>'
                            break;
                        } else {
                            // Not our closing tag, restore position
                            pos_ = saved_pos;
                        }
                    } else {
                        // Not a closing tag, restore position
                        pos_ = saved_pos;
                    }
                    
                    // Parse child element
                    auto child = parse_element();
                    if (child) {
                        child->parent = element;
                        element->children.push_back(child);
                    }
                } else {
                    // Parse text content
                    std::string text = parse_text();
                    if (!text.empty()) {
                        element->text += text;
                    }
                }
            }
        }
    } else {
        // Text node
        element->text = parse_text();
    }
    
    return element;
}

std::shared_ptr<HtmlElement> HtmlParser::parse() {
    pos_ = 0;
    auto root = std::make_shared<HtmlElement>();
    root->tag = "document";
    
    while (pos_ < html_.length()) {
        skip_whitespace();
        if (pos_ >= html_.length()) break;
        
        auto element = parse_element();
        if (element) {
            element->parent = root;
            root->children.push_back(element);
        }
    }
    
    return root;
}

std::string HtmlParser::extract_title(const std::string& html) {
    std::regex title_regex(R"(<title[^>]*>(.*?)</title>)", std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, title_regex)) {
        return match[1].str();
    }
    
    return "";
}

std::string HtmlParser::extract_meta_content(const std::string& html, const std::string& name) {
    std::string pattern = R"(<meta[^>]+name\s*=\s*["\'])" + name + R"(["\'][^>]+content\s*=\s*["\']([^"\']*)["\'])";
    std::regex meta_regex(pattern, std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, meta_regex)) {
        return match[1].str();
    }
    
    return "";
}

std::vector<std::string> HtmlParser::extract_meta_tags(const std::string& html) {
    std::vector<std::string> meta_tags;
    std::regex meta_regex(R"(<meta[^>]*>)", std::regex_constants::icase);
    std::sregex_iterator iter(html.begin(), html.end(), meta_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        meta_tags.push_back(iter->str());
    }
    
    return meta_tags;
}

std::string HtmlParser::detect_encoding(const std::string& html) {
    // Look for charset in meta tags
    std::regex charset_regex(R"(charset\s*=\s*["\']?([^"\'>\s]+))", std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, charset_regex)) {
        return match[1].str();
    }
    
    return "utf-8"; // Default
}

// ==============================================================================
// Boilerplate Remover Implementation
// ==============================================================================

BoilerplateRemover::BoilerplateRemover(const TextExtractionConfig& config) : config_(config) {
    initialize_patterns();
}

void BoilerplateRemover::initialize_patterns() {
    // Common boilerplate tags
    boilerplate_tags_ = {
        "script", "style", "noscript", "iframe", "embed", "object",
        "nav", "header", "footer", "aside", "menu"
    };
    
    // Common boilerplate class patterns
    boilerplate_classes_ = {
        "nav", "navigation", "menu", "sidebar", "footer", "header",
        "ad", "ads", "advertisement", "banner", "popup", "modal",
        "breadcrumb", "pagination", "social", "share", "comment",
        "widget", "plugin", "tracking", "analytics"
    };
    
    // Common boilerplate ID patterns
    boilerplate_ids_ = {
        "nav", "navigation", "menu", "sidebar", "footer", "header",
        "ad", "ads", "banner", "popup", "modal", "comments"
    };
    
    // Regex patterns for boilerplate detection
    boilerplate_patterns_ = {
        std::regex(R"(\b(cookie|privacy|terms|policy)\b)", std::regex_constants::icase),
        std::regex(R"(\b(subscribe|newsletter|signup)\b)", std::regex_constants::icase),
        std::regex(R"(\b(copyright|©|all rights reserved)\b)", std::regex_constants::icase),
        std::regex(R"(\b(click here|read more|continue reading)\b)", std::regex_constants::icase)
    };
}

bool BoilerplateRemover::is_boilerplate_element(const std::shared_ptr<HtmlElement>& element) const {
    if (!element) return true;
    
    // Check tag name
    if (boilerplate_tags_.find(element->tag) != boilerplate_tags_.end()) {
        return true;
    }
    
    // Check class attribute
    std::string class_attr = element->get_attribute("class");
    if (!class_attr.empty()) {
        std::transform(class_attr.begin(), class_attr.end(), class_attr.begin(), ::tolower);
        for (const auto& pattern : boilerplate_classes_) {
            if (class_attr.find(pattern) != std::string::npos) {
                return true;
            }
        }
    }
    
    // Check id attribute
    std::string id_attr = element->get_attribute("id");
    if (!id_attr.empty()) {
        std::transform(id_attr.begin(), id_attr.end(), id_attr.begin(), ::tolower);
        if (boilerplate_ids_.find(id_attr) != boilerplate_ids_.end()) {
            return true;
        }
    }
    
    return false;
}

bool BoilerplateRemover::is_navigation_element(const std::shared_ptr<HtmlElement>& element) const {
    if (!element) return false;
    
    return element->tag == "nav" || 
           element->has_class("nav") || 
           element->has_class("navigation") ||
           element->has_class("menu") ||
           element->has_id("nav") ||
           element->has_id("navigation");
}

bool BoilerplateRemover::is_advertisement_element(const std::shared_ptr<HtmlElement>& element) const {
    if (!element) return false;
    
    std::string class_attr = element->get_attribute("class");
    std::string id_attr = element->get_attribute("id");
    
    std::vector<std::string> ad_patterns = {"ad", "ads", "advertisement", "banner", "sponsor"};
    
    for (const auto& pattern : ad_patterns) {
        if (class_attr.find(pattern) != std::string::npos ||
            id_attr.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool BoilerplateRemover::is_content_element(const std::shared_ptr<HtmlElement>& element) const {
    if (!element) return false;
    
    // Content tags
    std::unordered_set<std::string> content_tags = {
        "article", "main", "section", "div", "p", "h1", "h2", "h3", "h4", "h5", "h6"
    };
    
    if (content_tags.find(element->tag) != content_tags.end()) {
        return true;
    }
    
    // Content class patterns
    std::string class_attr = element->get_attribute("class");
    std::vector<std::string> content_patterns = {
        "content", "article", "main", "body", "text", "post", "entry"
    };
    
    for (const auto& pattern : content_patterns) {
        if (class_attr.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

double BoilerplateRemover::calculate_content_score(const std::shared_ptr<HtmlElement>& element) const {
    if (!element) return 0.0;
    
    double score = 0.0;
    
    // Positive scoring for content elements
    if (is_content_element(element)) {
        score += 10.0;
    }
    
    // Negative scoring for boilerplate elements
    if (is_boilerplate_element(element)) {
        score -= 20.0;
    }
    
    if (is_navigation_element(element)) {
        score -= 15.0;
    }
    
    if (is_advertisement_element(element)) {
        score -= 25.0;
    }
    
    // Text content scoring
    size_t text_length = element->text.length();
    if (text_length > 0) {
        score += std::min(text_length / 10.0, 20.0); // Cap at 20 points
    }
    
    // Link density penalty
    size_t link_count = 0;
    for (const auto& child : element->children) {
        if (child->tag == "a") {
            link_count++;
        }
    }
    
    if (text_length > 0) {
        double link_density = (double(link_count) / text_length) * 100.0;
        if (link_density > 5.0) {
            score -= link_density;
        }
    }
    
    return score;
}

double BoilerplateRemover::calculate_text_density(const std::shared_ptr<HtmlElement>& element) const {
    if (!element) return 0.0;
    
    size_t text_length = 0;
    size_t tag_count = 0;
    
    std::function<void(const std::shared_ptr<HtmlElement>&)> count_content = 
        [&](const std::shared_ptr<HtmlElement>& elem) {
            if (!elem) return;
            
            text_length += elem->text.length();
            tag_count++;
            
            for (const auto& child : elem->children) {
                count_content(child);
            }
        };
    
    count_content(element);
    
    return tag_count > 0 ? double(text_length) / tag_count : 0.0;
}

std::shared_ptr<HtmlElement> BoilerplateRemover::remove_boilerplate(std::shared_ptr<HtmlElement> root) const {
    if (!root) return nullptr;
    
    // Remove boilerplate children
    auto it = root->children.begin();
    while (it != root->children.end()) {
        if (is_boilerplate_element(*it)) {
            it = root->children.erase(it);
        } else {
            *it = remove_boilerplate(*it);
            ++it;
        }
    }
    
    return root;
}

std::vector<std::shared_ptr<HtmlElement>> BoilerplateRemover::find_main_content(const std::shared_ptr<HtmlElement>& root) const {
    std::vector<std::shared_ptr<HtmlElement>> content_elements;
    
    if (!root) return content_elements;
    
    std::function<void(const std::shared_ptr<HtmlElement>&)> find_content = 
        [&](const std::shared_ptr<HtmlElement>& element) {
            if (!element) return;
            
            double score = calculate_content_score(element);
            if (score > 5.0 && !element->text.empty()) {
                content_elements.push_back(element);
            }
            
            for (const auto& child : element->children) {
                find_content(child);
            }
        };
    
    find_content(root);
    
    // Sort by content score (descending)
    std::sort(content_elements.begin(), content_elements.end(),
        [this](const std::shared_ptr<HtmlElement>& a, const std::shared_ptr<HtmlElement>& b) {
            return calculate_content_score(a) > calculate_content_score(b);
        });
    
    return content_elements;
}

// ==============================================================================
// Text Cleaner Implementation  
// ==============================================================================

TextCleaner::TextCleaner(const TextExtractionConfig& config) : config_(config) {
    initialize_patterns();
    initialize_html_entities();
}

void TextCleaner::initialize_patterns() {
    // Common cleaning patterns
    cleaning_patterns_ = {
        // Remove extra whitespace
        {std::regex(R"(\s+)"), " "},
        // Remove leading/trailing whitespace from lines
        {std::regex(R"(^\s+|\s+$)", std::regex_constants::multiline), ""},
        // Remove excessive newlines
        {std::regex(R"(\n\s*\n\s*\n+)"), "\n\n"},
        // Remove remaining HTML tags
        {std::regex(R"(<[^>]*>)"), ""},
        // Remove script/style content that might remain
        {std::regex(R"(<script[^>]*>.*?</script>)", std::regex_constants::icase), ""},
        {std::regex(R"(<style[^>]*>.*?</style>)", std::regex_constants::icase), ""},
    };
}

void TextCleaner::initialize_html_entities() {
    html_entities_ = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&quot;", "\""},
        {"&apos;", "'"},
        {"&nbsp;", " "},
        {"&copy;", "©"},
        {"&reg;", "®"},
        {"&trade;", "™"},
        {"&mdash;", "—"},
        {"&ndash;", "–"},
        {"&hellip;", "…"},
        {"&laquo;", "«"},
        {"&raquo;", "»"},
        {"&ldquo;", "\""},
        {"&rdquo;", "\""},
        {"&lsquo;", "'"},
        {"&rsquo;", "'"}
    };
}

std::string TextCleaner::decode_html_entities(const std::string& text) const {
    std::string result = text;
    
    for (const auto& [entity, replacement] : html_entities_) {
        size_t pos = 0;
        while ((pos = result.find(entity, pos)) != std::string::npos) {
            result.replace(pos, entity.length(), replacement);
            pos += replacement.length();
        }
    }
    
    // Handle numeric entities
    std::regex numeric_entity(R"(&#(\d+);)");
    std::smatch match;
    
    while (std::regex_search(result, match, numeric_entity)) {
        int code = std::stoi(match[1].str());
        if (code >= 32 && code <= 126) { // Printable ASCII
            char ch = static_cast<char>(code);
            result.replace(match.position(), match.length(), 1, ch);
        } else {
            result.replace(match.position(), match.length(), "");
        }
    }
    
    return result;
}

std::string TextCleaner::normalize_unicode(const std::string& text) const {
    // Basic Unicode normalization - in a full implementation, 
    // you'd use ICU or similar library
    std::string result = text;
    
    // Replace common problematic characters
    std::vector<std::pair<std::string, std::string>> replacements = {
        {"\xC2\xA0", " "},  // Non-breaking space
        {"\xE2\x80\x93", "-"},  // En dash
        {"\xE2\x80\x94", "—"},  // Em dash
        {"\xE2\x80\x98", "'"},  // Left single quotation mark
        {"\xE2\x80\x99", "'"},  // Right single quotation mark
        {"\xE2\x80\x9C", "\""},  // Left double quotation mark
        {"\xE2\x80\x9D", "\""},  // Right double quotation mark
        {"\xE2\x80\xA6", "..."},  // Horizontal ellipsis
    };
    
    for (const auto& [from, to] : replacements) {
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
    }
    
    return result;
}

std::string TextCleaner::normalize_whitespace(const std::string& text) const {
    std::string result = text;
    
    // Replace tabs and multiple spaces with single space
    result = std::regex_replace(result, std::regex(R"([\t ]+)"), " ");
    
    // Normalize line endings
    result = std::regex_replace(result, std::regex(R"(\r\n|\r)"), "\n");
    
    return result;
}

std::string TextCleaner::fix_mojibake(const std::string& text) const {
    std::string result = text;
    
    // Common mojibake patterns (simplified)
    std::vector<std::pair<std::string, std::string>> mojibake_fixes = {
        {"Ã¡", "á"}, {"Ã©", "é"}, {"Ã­", "í"}, {"Ã³", "ó"}, {"Ãº", "ú"},
        {"Ã ", "à"}, {"Ã¨", "è"}, {"Ã¬", "ì"}, {"Ã²", "ò"}, {"Ã¹", "ù"},
        {"Ã¢", "â"}, {"Ãª", "ê"}, {"Ã®", "î"}, {"Ã´", "ô"}, {"Ã»", "û"},
        {"Ã£", "ã"}, {"Ã±", "ñ"}, {"Ã§", "ç"}
    };
    
    for (const auto& [mojibake, correct] : mojibake_fixes) {
        size_t pos = 0;
        while ((pos = result.find(mojibake, pos)) != std::string::npos) {
            result.replace(pos, mojibake.length(), correct);
            pos += correct.length();
        }
    }
    
    return result;
}

std::string TextCleaner::remove_extra_newlines(const std::string& text) const {
    std::string result = text;
    
    // Replace multiple consecutive newlines with double newline
    result = std::regex_replace(result, std::regex(R"(\n\s*\n\s*\n+)"), "\n\n");
    
    // Remove leading and trailing newlines
    result = std::regex_replace(result, std::regex(R"(^\n+|\n+$)"), "");
    
    return result;
}

std::string TextCleaner::clean_text(const std::string& text) const {
    std::string result = text;
    
    if (config_.decode_html_entities) {
        result = decode_html_entities(result);
    }
    
    if (config_.normalize_unicode) {
        result = normalize_unicode(result);
    }
    
    if (config_.fix_mojibake) {
        result = fix_mojibake(result);
    }
    
    if (config_.normalize_whitespace) {
        result = normalize_whitespace(result);
    }
    
    // Apply cleaning patterns
    for (const auto& [pattern, replacement] : cleaning_patterns_) {
        result = std::regex_replace(result, pattern, replacement);
    }
    
    if (config_.remove_extra_newlines) {
        result = remove_extra_newlines(result);
    }
    
    if (config_.trim_lines) {
        std::istringstream iss(result);
        std::ostringstream oss;
        std::string line;
        bool first = true;
        
        while (std::getline(iss, line)) {
            if (!first) oss << "\n";
            first = false;
            
            // Trim line
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            oss << line;
        }
        
        result = oss.str();
    }
    
    return result;
}

// ==============================================================================
// Text Extractor Implementation
// ==============================================================================

TextExtractor::TextExtractor(const TextExtractionConfig& config) : config_(config) {
    parser_ = std::make_unique<HtmlParser>("");
    boilerplate_remover_ = std::make_unique<BoilerplateRemover>(config);
    text_cleaner_ = std::make_unique<TextCleaner>(config);
}

TextExtractor::TextExtractor(TextExtractor&& other) noexcept
    : config_(std::move(other.config_)),
      parser_(std::move(other.parser_)),
      boilerplate_remover_(std::move(other.boilerplate_remover_)),
      text_cleaner_(std::move(other.text_cleaner_)) {}

TextExtractor& TextExtractor::operator=(TextExtractor&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
        parser_ = std::move(other.parser_);
        boilerplate_remover_ = std::move(other.boilerplate_remover_);
        text_cleaner_ = std::move(other.text_cleaner_);
    }
    return *this;
}

void TextExtractor::set_config(const TextExtractionConfig& config) {
    config_ = config;
    boilerplate_remover_ = std::make_unique<BoilerplateRemover>(config);
    text_cleaner_ = std::make_unique<TextCleaner>(config);
}

std::string TextExtractor::extract_text_from_element(const std::shared_ptr<HtmlElement>& element) const {
    if (!element) return "";
    
    std::ostringstream text_stream;
    
    std::function<void(const std::shared_ptr<HtmlElement>&)> extract_recursive = 
        [&](const std::shared_ptr<HtmlElement>& elem) {
            if (!elem) return;
            
            // Add element text
            if (!elem->text.empty()) {
                text_stream << elem->text;
            }
            
            // Add paragraph breaks for block elements
            if (elem->tag == "p" || elem->tag == "div" || 
                elem->tag == "br" || elem->tag.substr(0, 1) == "h") {
                text_stream << "\n";
            }
            
            // Process children
            for (const auto& child : elem->children) {
                extract_recursive(child);
            }
            
            // Add extra break after paragraphs and headings
            if (elem->tag == "p" || elem->tag.substr(0, 1) == "h") {
                text_stream << "\n";
            }
        };
    
    extract_recursive(element);
    return text_stream.str();
}

std::vector<std::string> TextExtractor::extract_headings(const std::shared_ptr<HtmlElement>& element) const {
    std::vector<std::string> headings;
    
    if (!element) return headings;
    
    std::function<void(const std::shared_ptr<HtmlElement>&)> find_headings = 
        [&](const std::shared_ptr<HtmlElement>& elem) {
            if (!elem) return;
            
            if (elem->tag.length() == 2 && elem->tag[0] == 'h' && 
                std::isdigit(elem->tag[1]) && !elem->text.empty()) {
                headings.push_back(elem->text);
            }
            
            for (const auto& child : elem->children) {
                find_headings(child);
            }
        };
    
    find_headings(element);
    return headings;
}

std::vector<std::string> TextExtractor::extract_links(const std::shared_ptr<HtmlElement>& element) const {
    std::vector<std::string> links;
    
    if (!element) return links;
    
    std::function<void(const std::shared_ptr<HtmlElement>&)> find_links = 
        [&](const std::shared_ptr<HtmlElement>& elem) {
            if (!elem) return;
            
            if (elem->tag == "a" && elem->has_attribute("href")) {
                std::string href = elem->get_attribute("href");
                if (!href.empty()) {
                    links.push_back(href);
                }
            }
            
            for (const auto& child : elem->children) {
                find_links(child);
            }
        };
    
    find_links(element);
    return links;
}

void TextExtractor::calculate_quality_metrics(TextExtractionResult& result) const {
    result.text_ratio = result.original_html_length > 0 ? 
        double(result.extracted_text_length) / result.original_html_length : 0.0;
    
    // Count paragraphs (rough estimate)
    result.paragraph_count = std::count(result.extracted_text.begin(), 
                                      result.extracted_text.end(), '\n');
    
    // Calculate link density
    if (result.extracted_text_length > 0) {
        result.link_density = (double(result.link_count) / result.extracted_text_length) * 100.0;
    }
}

TextExtractionResult TextExtractor::extract(const std::string& html, const std::string& url) const {
    TextExtractionResult result;
    result.url = url;
    result.original_html_length = html.length();
    
    // Parse HTML
    HtmlParser parser(html);
    auto root = parser.parse();
    
    if (!root) {
        return result; // Failed to parse
    }
    
    // Extract metadata
    result.title = text_cleaner_->clean_text(HtmlParser::extract_title(html));
    result.language = HtmlParser::extract_meta_content(html, "language");
    
    // Remove boilerplate
    if (config_.extract_main_content) {
        root = boilerplate_remover_->remove_boilerplate(root);
        auto content_elements = boilerplate_remover_->find_main_content(root);
        
        if (!content_elements.empty()) {
            // Use the highest-scoring content element
            root = content_elements[0];
        }
    }
    
    // Extract text content
    std::string raw_text = extract_text_from_element(root);
    result.extracted_text = text_cleaner_->clean_text(raw_text);
    result.extracted_text_length = result.extracted_text.length();
    
    // Extract additional information
    if (config_.preserve_headings) {
        result.headings = extract_headings(root);
    }
    
    if (config_.preserve_links) {
        result.links = extract_links(root);
    }
    
    result.link_count = extract_links(root).size();
    
    // Calculate quality metrics
    calculate_quality_metrics(result);
    
    return result;
}

std::vector<TextExtractionResult> TextExtractor::extract_batch(
    const std::vector<std::string>& html_documents,
    const std::vector<std::string>& urls
) const {
    return extract_batch(html_documents, nullptr, urls);
}

std::vector<TextExtractionResult> TextExtractor::extract_batch(
    const std::vector<std::string>& html_documents,
    std::function<void(size_t, size_t, const std::string&)> progress_callback,
    const std::vector<std::string>& urls
) const {
    std::vector<TextExtractionResult> results;
    results.reserve(html_documents.size());
    
    for (size_t i = 0; i < html_documents.size(); ++i) {
        if (progress_callback) {
            progress_callback(i + 1, html_documents.size(), "Extracting text");
        }
        
        std::string url = (i < urls.size()) ? urls[i] : "";
        results.push_back(extract(html_documents[i], url));
    }
    
    return results;
}

std::string TextExtractor::extract_text_simple(const std::string& html) {
    TextExtractor extractor;
    auto result = extractor.extract(html);
    return result.extracted_text;
}

std::string TextExtractor::extract_title(const std::string& html) {
    return HtmlParser::extract_title(html);
}

bool TextExtractor::is_likely_html(const std::string& content) {
    // Simple heuristic to detect HTML content
    return content.find("<html") != std::string::npos ||
           content.find("<!DOCTYPE") != std::string::npos ||
           content.find("<head>") != std::string::npos ||
           content.find("<body>") != std::string::npos ||
           (content.find('<') != std::string::npos && content.find('>') != std::string::npos);
}

} // namespace rapidsift 