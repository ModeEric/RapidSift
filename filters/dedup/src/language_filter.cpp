#include "rapidsift/language_filter.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>
#include <cctype>
#include <fstream>
#include <random>

#ifdef HAVE_FASTTEXT
#include <fasttext.h>
#endif

namespace rapidsift {

// ==============================================================================
// FastText Language Detector Implementation
// ==============================================================================

#ifdef HAVE_FASTTEXT
struct FastTextLanguageDetector::Implementation {
    fasttext::FastText ft;
    bool model_loaded = false;
    double threshold = 0.0;
    int32_t top_k = 1;
    
    Implementation() = default;
};
#else
struct FastTextLanguageDetector::Implementation {
    bool model_loaded = false;
    double threshold = 0.0;
    int32_t top_k = 1;
};
#endif

FastTextLanguageDetector::FastTextLanguageDetector(const std::string& model_path)
    : impl_(std::make_unique<Implementation>()) {
    if (!model_path.empty()) {
        load_model(model_path);
    }
}

FastTextLanguageDetector::~FastTextLanguageDetector() = default;

FastTextLanguageDetector::FastTextLanguageDetector(FastTextLanguageDetector&& other) noexcept
    : impl_(std::move(other.impl_)) {}

FastTextLanguageDetector& FastTextLanguageDetector::operator=(FastTextLanguageDetector&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

bool FastTextLanguageDetector::load_model(const std::string& model_path) {
    if (!impl_) return false;
    
    try {
#ifdef HAVE_FASTTEXT
        impl_->ft.loadModel(model_path);
        impl_->model_loaded = true;
        return true;
#else
        std::cerr << "Warning: FastText support not compiled. Using fallback detector." << std::endl;
        return false;
#endif
    } catch (const std::exception& e) {
        std::cerr << "Error loading FastText model: " << e.what() << std::endl;
        impl_->model_loaded = false;
        return false;
    }
}

LanguageDetection FastTextLanguageDetector::detect(const std::string& text) {
    if (!impl_ || !impl_->model_loaded) {
        return LanguageDetection("unknown", 0.0);
    }
    
#ifdef HAVE_FASTTEXT
    try {
        std::vector<std::pair<fasttext::real, std::string>> predictions;
        std::istringstream iss(text);
        impl_->ft.predictLine(iss, predictions, impl_->top_k, impl_->threshold);
        
        if (predictions.empty()) {
            return LanguageDetection("unknown", 0.0);
        }
        
        // Extract language from fastText label format (__label__en -> en)
        std::string language = predictions[0].second;
        if (language.substr(0, 9) == "__label__") {
            language = language.substr(9);
        }
        
        double confidence = std::exp(predictions[0].first);  // Convert log-prob to probability
        
        return LanguageDetection(language, confidence);
    } catch (const std::exception& e) {
        std::cerr << "Error in language detection: " << e.what() << std::endl;
        return LanguageDetection("unknown", 0.0);
    }
#else
    return LanguageDetection("unknown", 0.0);
#endif
}

std::vector<LanguageDetection> FastTextLanguageDetector::detect_batch(const std::vector<std::string>& texts) {
    std::vector<LanguageDetection> results;
    results.reserve(texts.size());
    
    for (const auto& text : texts) {
        results.push_back(detect(text));
    }
    
    return results;
}

std::vector<std::string> FastTextLanguageDetector::get_supported_languages() const {
    // Common languages supported by fastText language identification model
    return {
        "en", "de", "fr", "es", "pt", "it", "ru", "ja", "ko", "zh",
        "ar", "hi", "th", "vi", "id", "ms", "tl", "sw", "nl", "sv",
        "da", "no", "fi", "pl", "cs", "sk", "hu", "ro", "bg", "hr",
        "sl", "et", "lv", "lt", "el", "tr", "he", "fa", "ur", "bn"
    };
}

bool FastTextLanguageDetector::is_ready() const {
    return impl_ && impl_->model_loaded;
}

void FastTextLanguageDetector::set_threshold(double threshold) {
    if (impl_) {
        impl_->threshold = threshold;
    }
}

void FastTextLanguageDetector::set_top_k(int k) {
    if (impl_) {
        impl_->top_k = k;
    }
}

// ==============================================================================
// Simple Language Detector Implementation
// ==============================================================================

SimpleLanguageDetector::SimpleLanguageDetector() {
    initialize_patterns();
}

void SimpleLanguageDetector::initialize_patterns() {
    // English patterns and stopwords
    language_patterns_["en"] = {
        "the", "and", "is", "in", "to", "of", "a", "that", "it", "with",
        "for", "as", "was", "on", "are", "you", "this", "be", "at", "or"
    };
    
    // Spanish patterns
    language_patterns_["es"] = {
        "el", "la", "de", "que", "y", "en", "un", "es", "se", "no",
        "te", "lo", "le", "da", "su", "por", "son", "con", "para", "al"
    };
    
    // French patterns
    language_patterns_["fr"] = {
        "le", "de", "et", "un", "il", "être", "et", "en", "à", "avoir",
        "que", "pour", "dans", "ce", "son", "une", "sur", "avec", "ne", "se"
    };
    
    // German patterns
    language_patterns_["de"] = {
        "der", "die", "und", "in", "den", "von", "zu", "das", "mit", "sich",
        "des", "auf", "für", "ist", "im", "dem", "nicht", "ein", "eine", "als"
    };
    
    // Italian patterns
    language_patterns_["it"] = {
        "di", "il", "la", "è", "che", "un", "una", "le", "in", "da",
        "per", "con", "non", "del", "si", "al", "lo", "degli", "della", "sulla"
    };
    
    // Portuguese patterns
    language_patterns_["pt"] = {
        "de", "a", "o", "que", "e", "do", "da", "em", "um", "para",
        "é", "com", "não", "uma", "os", "no", "se", "na", "por", "mais"
    };
    
    // Russian patterns (transliterated)
    language_patterns_["ru"] = {
        "и", "в", "не", "на", "я", "быть", "он", "с", "это", "а",
        "по", "все", "она", "так", "его", "но", "да", "ты", "к", "у"
    };
}

double SimpleLanguageDetector::calculate_language_score(const std::string& text, const std::string& language) const {
    auto it = language_patterns_.find(language);
    if (it == language_patterns_.end()) {
        return 0.0;
    }
    
    const auto& patterns = it->second;
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    int matches = 0;
    int total_words = 0;
    
    std::istringstream iss(lower_text);
    std::string word;
    
    while (iss >> word) {
        total_words++;
        // Remove punctuation
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        
        if (std::find(patterns.begin(), patterns.end(), word) != patterns.end()) {
            matches++;
        }
    }
    
    return total_words > 0 ? double(matches) / total_words : 0.0;
}

LanguageDetection SimpleLanguageDetector::detect(const std::string& text) {
    if (text.length() < 10) {
        return LanguageDetection("unknown", 0.0);
    }
    
    std::string best_language = "unknown";
    double best_score = 0.0;
    
    for (const auto& [language, patterns] : language_patterns_) {
        double score = calculate_language_score(text, language);
        if (score > best_score) {
            best_score = score;
            best_language = language;
        }
    }
    
    // Adjust confidence based on score
    double confidence = std::min(1.0, best_score * 2.0);  // Scale up the score
    
    return LanguageDetection(best_language, confidence);
}

std::vector<std::string> SimpleLanguageDetector::get_supported_languages() const {
    std::vector<std::string> languages;
    for (const auto& [lang, patterns] : language_patterns_) {
        languages.push_back(lang);
    }
    return languages;
}

bool SimpleLanguageDetector::is_ready() const {
    return !language_patterns_.empty();
}

// ==============================================================================
// Language Filter Implementation
// ==============================================================================

LanguageFilter::LanguageFilter(const LanguageFilterConfig& config)
    : config_(config) {
    // Try to initialize FastText detector, fallback to simple detector
    auto fasttext_detector = std::make_unique<FastTextLanguageDetector>();
    
    // Try to load default model
    std::string model_path = language_utils::get_default_model_path();
    if (!model_path.empty() && fasttext_detector->load_model(model_path)) {
        detector_ = std::move(fasttext_detector);
    } else {
        // Fallback to simple detector
        detector_ = std::make_unique<SimpleLanguageDetector>();
    }
}

LanguageFilter::LanguageFilter(LanguageFilter&& other) noexcept
    : config_(std::move(other.config_)), detector_(std::move(other.detector_)) {}

LanguageFilter& LanguageFilter::operator=(LanguageFilter&& other) noexcept {
    if (this != &other) {
        config_ = std::move(other.config_);
        detector_ = std::move(other.detector_);
    }
    return *this;
}

void LanguageFilter::set_detector(std::unique_ptr<LanguageDetector> detector) {
    detector_ = std::move(detector);
}

std::string LanguageFilter::preprocess_text(const std::string& text) const {
    if (!config_.enable_preprocessing) {
        return text;
    }
    
    return language_utils::clean_text_for_detection(text);
}

bool LanguageFilter::should_keep_document(const LanguageDetection& detection) const {
    // Check minimum confidence
    if (detection.confidence < config_.min_confidence) {
        return false;
    }
    
    // Check if language is in target languages (if specified)
    if (!config_.target_languages.empty()) {
        return std::find(config_.target_languages.begin(), config_.target_languages.end(), 
                        detection.language) != config_.target_languages.end();
    }
    
    // If no target languages specified, keep if confidence is high enough
    return true;
}

bool LanguageFilter::is_mixed_language(const std::string& text) const {
    if (!config_.remove_mixed_language) {
        return false;
    }
    
    return language_utils::has_mixed_scripts(text);
}

LanguageFilterResult LanguageFilter::filter(const std::vector<Document>& documents) const {
    return filter(documents, nullptr);
}

LanguageFilterResult LanguageFilter::filter(
    const std::vector<Document>& documents,
    std::function<void(size_t, size_t, const std::string&)> progress_callback
) const {
    LanguageFilterResult result;
    result.total_processed = documents.size();
    
    double total_confidence = 0.0;
    size_t valid_detections = 0;
    
    for (size_t i = 0; i < documents.size(); ++i) {
        const auto& doc = documents[i];
        
        if (progress_callback) {
            progress_callback(i + 1, documents.size(), "Language filtering");
        }
        
        // Skip very short documents
        if (doc.text().length() < config_.min_text_length) {
            result.rejected_documents.push_back(doc);
            result.total_rejected++;
            continue;
        }
        
        // Preprocess text
        std::string processed_text = preprocess_text(doc.text());
        
        // Check for mixed languages if enabled
        if (is_mixed_language(processed_text)) {
            result.rejected_documents.push_back(doc);
            result.total_rejected++;
            continue;
        }
        
        // Detect language
        LanguageDetection detection = detector_->detect(processed_text);
        
        // Update statistics
        result.language_counts[detection.language]++;
        if (detection.confidence > 0.0) {
            total_confidence += detection.confidence;
            valid_detections++;
        }
        
        // Decide whether to keep the document
        if (should_keep_document(detection)) {
            result.filtered_documents.push_back(doc);
            result.total_kept++;
        } else {
            result.rejected_documents.push_back(doc);
            result.total_rejected++;
        }
    }
    
    // Calculate average confidence
    result.average_confidence = valid_detections > 0 ? total_confidence / valid_detections : 0.0;
    
    return result;
}

LanguageDetection LanguageFilter::detect_language(const Document& document) const {
    if (!detector_) {
        return LanguageDetection("unknown", 0.0);
    }
    
    std::string processed_text = preprocess_text(document.text());
    return detector_->detect(processed_text);
}

std::unordered_map<std::string, size_t> LanguageFilter::get_language_stats(
    const std::vector<Document>& documents
) const {
    std::unordered_map<std::string, size_t> stats;
    
    for (const auto& doc : documents) {
        LanguageDetection detection = detect_language(doc);
        stats[detection.language]++;
    }
    
    return stats;
}

bool LanguageFilter::is_ready() const {
    return detector_ && detector_->is_ready();
}

std::vector<std::string> LanguageFilter::get_supported_languages() const {
    if (!detector_) {
        return {};
    }
    return detector_->get_supported_languages();
}

// ==============================================================================
// Language Utilities Implementation
// ==============================================================================

namespace language_utils {

std::string iso639_1_to_name(const std::string& code) {
    static const std::unordered_map<std::string, std::string> code_to_name = {
        {"en", "English"}, {"es", "Spanish"}, {"fr", "French"}, {"de", "German"},
        {"it", "Italian"}, {"pt", "Portuguese"}, {"ru", "Russian"}, {"ja", "Japanese"},
        {"ko", "Korean"}, {"zh", "Chinese"}, {"ar", "Arabic"}, {"hi", "Hindi"},
        {"th", "Thai"}, {"vi", "Vietnamese"}, {"id", "Indonesian"}, {"ms", "Malay"},
        {"tl", "Filipino"}, {"sw", "Swahili"}, {"nl", "Dutch"}, {"sv", "Swedish"},
        {"da", "Danish"}, {"no", "Norwegian"}, {"fi", "Finnish"}, {"pl", "Polish"},
        {"cs", "Czech"}, {"sk", "Slovak"}, {"hu", "Hungarian"}, {"ro", "Romanian"}
    };
    
    auto it = code_to_name.find(code);
    return it != code_to_name.end() ? it->second : "Unknown";
}

std::string name_to_iso639_1(const std::string& name) {
    static const std::unordered_map<std::string, std::string> name_to_code = {
        {"English", "en"}, {"Spanish", "es"}, {"French", "fr"}, {"German", "de"},
        {"Italian", "it"}, {"Portuguese", "pt"}, {"Russian", "ru"}, {"Japanese", "ja"},
        {"Korean", "ko"}, {"Chinese", "zh"}, {"Arabic", "ar"}, {"Hindi", "hi"},
        {"Thai", "th"}, {"Vietnamese", "vi"}, {"Indonesian", "id"}, {"Malay", "ms"},
        {"Filipino", "tl"}, {"Swahili", "sw"}, {"Dutch", "nl"}, {"Swedish", "sv"},
        {"Danish", "da"}, {"Norwegian", "no"}, {"Finnish", "fi"}, {"Polish", "pl"},
        {"Czech", "cs"}, {"Slovak", "sk"}, {"Hungarian", "hu"}, {"Romanian", "ro"}
    };
    
    auto it = name_to_code.find(name);
    return it != name_to_code.end() ? it->second : "unknown";
}

bool is_valid_language_code(const std::string& code) {
    return code.length() == 2 && std::all_of(code.begin(), code.end(), ::islower);
}

std::string clean_text_for_detection(const std::string& text) {
    std::string cleaned = text;
    
    // Remove URLs
    std::regex url_regex(R"((https?://[^\s]+)|(www\.[^\s]+))");
    cleaned = std::regex_replace(cleaned, url_regex, " ");
    
    // Remove email addresses
    std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    cleaned = std::regex_replace(cleaned, email_regex, " ");
    
    // Remove excessive whitespace
    std::regex whitespace_regex(R"(\s+)");
    cleaned = std::regex_replace(cleaned, whitespace_regex, " ");
    
    // Trim
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    return cleaned;
}

bool has_mixed_scripts(const std::string& text) {
    // Simple heuristic: check for different script ranges
    bool has_latin = false;
    bool has_cyrillic = false;
    bool has_arabic = false;
    bool has_asian = false;
    
    for (char c : text) {
        unsigned char uc = static_cast<unsigned char>(c);
        
        if ((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z')) {
            has_latin = true;
        } else if (uc >= 0xC0 && uc <= 0xFF) {
            // This is a very rough approximation for extended Latin/Cyrillic
            // In a real implementation, you'd want proper Unicode support
            has_cyrillic = true;
        }
    }
    
    // Count script types
    int script_count = (has_latin ? 1 : 0) + (has_cyrillic ? 1 : 0) + 
                      (has_arabic ? 1 : 0) + (has_asian ? 1 : 0);
    
    return script_count > 1;
}

double adjust_confidence_for_length(double base_confidence, size_t text_length) {
    if (text_length < 20) {
        return base_confidence * 0.5;  // Very short text is unreliable
    } else if (text_length < 50) {
        return base_confidence * 0.7;  // Short text is somewhat unreliable
    } else if (text_length < 100) {
        return base_confidence * 0.9;  // Medium text is mostly reliable
    } else {
        return base_confidence;  // Long text is fully reliable
    }
}

bool download_fasttext_model(const std::string& model_name, const std::string& cache_dir) {
    // This is a placeholder - in a real implementation, you'd download from
    // https://dl.fbaipublicfiles.com/fasttext/supervised-models/lid.176.bin
    std::cerr << "Note: Model download not implemented. Please manually download fastText language model." << std::endl;
    return false;
}

std::string get_default_model_path() {
    // Check common locations for fastText language model
    std::vector<std::string> possible_paths = {
        "./models/lid.176.bin",
        "./lid.176.bin",
        "/usr/local/share/fasttext/lid.176.bin",
        "/opt/fasttext/lid.176.bin"
    };
    
    for (const auto& path : possible_paths) {
        std::ifstream file(path);
        if (file.good()) {
            return path;
        }
    }
    
    return "";  // No model found
}

} // namespace language_utils

} // namespace rapidsift 