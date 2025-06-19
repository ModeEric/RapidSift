#include <iostream>
#include <vector>
#include <chrono>

#include "rapidsift/common.hpp"
#include "rapidsift/language_filter.hpp"
#include "test_framework.hpp"

using namespace rapidsift;
using namespace test_framework;

void test_simple_language_detector() {
    SimpleLanguageDetector detector;
    
    ASSERT_TRUE(detector.is_ready());
    
    // Test English detection
    LanguageDetection result = detector.detect("The quick brown fox jumps over the lazy dog. This is clearly English text with common words.");
    ASSERT_EQ(result.language, "en");
    ASSERT_GT(result.confidence, 0.0);
    
    // Test Spanish detection
    result = detector.detect("El rápido zorro marrón salta sobre el perro perezoso. Esta es claramente texto en español con palabras comunes.");
    ASSERT_EQ(result.language, "es");
    ASSERT_GT(result.confidence, 0.0);
    
    // Test French detection
    result = detector.detect("Le renard brun rapide saute par-dessus le chien paresseux. C'est clairement du texte français avec des mots communs.");
    ASSERT_EQ(result.language, "fr");
    ASSERT_GT(result.confidence, 0.0);
    
    // Test short text (should be unknown)
    result = detector.detect("Hi");
    ASSERT_EQ(result.language, "unknown");
}

void test_fasttext_detector_fallback() {
    // Test FastText detector when model is not available
    FastTextLanguageDetector detector("");
    
    // Should gracefully handle missing model
    ASSERT_FALSE(detector.is_ready());
    
    LanguageDetection result = detector.detect("This is English text");
    ASSERT_EQ(result.language, "unknown");
    ASSERT_EQ(result.confidence, 0.0);
}

void test_language_filter_config() {
    LanguageFilterConfig config;
    
    // Test default configuration
    ASSERT_EQ(config.min_confidence, 0.65);
    ASSERT_FALSE(config.strict_mode);
    ASSERT_TRUE(config.remove_mixed_language);
    ASSERT_EQ(config.min_text_length, 10);
    
    // Test custom configuration
    config.target_languages = {"en", "es"};
    config.min_confidence = 0.8;
    config.strict_mode = true;
    
    ASSERT_EQ(config.target_languages.size(), 2);
    ASSERT_EQ(config.min_confidence, 0.8);
    ASSERT_TRUE(config.strict_mode);
}

void test_basic_language_filtering() {
    LanguageFilterConfig config;
    config.target_languages = {"en"};
    config.min_confidence = 0.3;  // Lower threshold for testing
    
    LanguageFilter filter(config);
    ASSERT_TRUE(filter.is_ready());
    
    std::vector<Document> documents = {
        Document("The quick brown fox jumps over the lazy dog. This is English.", 1),
        Document("El rápido zorro marrón salta sobre el perro perezoso. Español.", 2),
        Document("Le renard brun rapide saute par-dessus le chien paresseux. Français.", 3),
        Document("Short", 4)  // Too short
    };
    
    LanguageFilterResult result = filter.filter(documents);
    
    ASSERT_EQ(result.total_processed, 4);
    ASSERT_GT(result.total_kept, 0);
    ASSERT_GT(result.total_rejected, 0);
    ASSERT_LE(result.total_kept + result.total_rejected, result.total_processed);
    
    // Check that we have language statistics
    ASSERT_GT(result.language_counts.size(), 0);
    
    // Check percentages
    ASSERT_GE(result.kept_percentage(), 0.0);
    ASSERT_LE(result.kept_percentage(), 100.0);
    ASSERT_GE(result.rejected_percentage(), 0.0);
    ASSERT_LE(result.rejected_percentage(), 100.0);
}

void test_language_detection_accuracy() {
    LanguageFilter filter;
    
    std::vector<std::pair<std::string, std::string>> test_cases = {
        {"This is a clear English sentence with common words like the, and, is, in, to, of, a, that, it, with.", "en"},
        {"Este es un texto en español con palabras comunes como el, la, de, que, y, en, un, es, se, no.", "es"},
        {"Ceci est un texte français avec des mots communs comme le, de, et, un, il, être, et, en, à, avoir.", "fr"},
        {"Dies ist ein deutscher Text mit gemeinsamen Wörtern wie der, die, und, in, den, von, zu, das, mit, sich.", "de"},
        {"Questo è un testo italiano con parole comuni come di, il, la, è, che, un, una, le, in, da.", "it"}
    };
    
    int correct_detections = 0;
    
    for (const auto& [text, expected_lang] : test_cases) {
        Document doc(text, 0);
        LanguageDetection detection = filter.detect_language(doc);
        
        if (detection.language == expected_lang) {
            correct_detections++;
        }
        
        // Should have some confidence for real text
        ASSERT_GT(detection.confidence, 0.0);
    }
    
    // Should get at least some detections correct
    ASSERT_GT(correct_detections, 0);
}

void test_minimum_text_length_filtering() {
    LanguageFilterConfig config;
    config.min_text_length = 20;
    config.target_languages = {"en"};
    config.min_confidence = 0.1;  // Very low threshold
    
    LanguageFilter filter(config);
    
    std::vector<Document> documents = {
        Document("Hi", 1),  // Too short
        Document("Hello world", 2),  // Still too short  
        Document("This is a longer English sentence that should pass the minimum length requirement.", 3)
    };
    
    LanguageFilterResult result = filter.filter(documents);
    
    ASSERT_EQ(result.total_processed, 3);
    ASSERT_LE(result.total_kept, 1);  // At most 1 should pass
    ASSERT_GE(result.total_rejected, 2);  // At least 2 should be rejected for length
}

void test_confidence_threshold_filtering() {
    LanguageFilterConfig config;
    config.target_languages = {"en"};
    config.min_confidence = 0.9;  // Very high threshold
    
    LanguageFilter filter(config);
    
    std::vector<Document> documents = {
        Document("This is a very clear and unambiguous English sentence with many common English words.", 1),
        Document("Ambiguous short text", 2),
        Document("Mixed language text avec des mots français", 3)
    };
    
    LanguageFilterResult result = filter.filter(documents);
    
    ASSERT_EQ(result.total_processed, 3);
    // With high confidence threshold, most should be rejected
    ASSERT_GE(result.total_rejected, 1);
}

void test_multilingual_document_handling() {
    LanguageFilterConfig config;
    config.target_languages = {"en"};
    config.remove_mixed_language = true;
    config.min_confidence = 0.3;
    
    LanguageFilter filter(config);
    
    std::vector<Document> documents = {
        Document("Pure English text with only English words and common patterns.", 1),
        Document("Mixed text with English and español palabras mezcladas together.", 2),
        Document("Another pure English sentence without foreign words.", 3)
    };
    
    LanguageFilterResult result = filter.filter(documents);
    
    ASSERT_EQ(result.total_processed, 3);
    // Mixed language document might be rejected
    ASSERT_GT(result.total_kept, 0);
}

void test_language_statistics() {
    LanguageFilter filter;
    
    std::vector<Document> documents = {
        Document("English text number one with common English words.", 1),
        Document("English text number two with more English content.", 2),
        Document("Texto en español con palabras comunes en español.", 3),
        Document("Texte français avec des mots français communs.", 4)
    };
    
    auto stats = filter.get_language_stats(documents);
    
    ASSERT_GT(stats.size(), 0);
    
    // Should have detected multiple languages
    size_t total_docs = 0;
    for (const auto& [lang, count] : stats) {
        total_docs += count;
        ASSERT_GT(count, 0);
    }
    ASSERT_EQ(total_docs, documents.size());
}

void test_language_utilities() {
    // Test language code validation
    ASSERT_TRUE(language_utils::is_valid_language_code("en"));
    ASSERT_TRUE(language_utils::is_valid_language_code("es"));
    ASSERT_FALSE(language_utils::is_valid_language_code("ENG"));
    ASSERT_FALSE(language_utils::is_valid_language_code("english"));
    ASSERT_FALSE(language_utils::is_valid_language_code("e"));
    
    // Test language code conversion
    ASSERT_EQ(language_utils::iso639_1_to_name("en"), "English");
    ASSERT_EQ(language_utils::iso639_1_to_name("es"), "Spanish");
    ASSERT_EQ(language_utils::iso639_1_to_name("unknown"), "Unknown");
    
    ASSERT_EQ(language_utils::name_to_iso639_1("English"), "en");
    ASSERT_EQ(language_utils::name_to_iso639_1("Spanish"), "es");
    ASSERT_EQ(language_utils::name_to_iso639_1("Unknown Language"), "unknown");
    
    // Test text cleaning
    std::string dirty_text = "  Hello world!  Visit https://example.com or email test@example.com  ";
    std::string cleaned = language_utils::clean_text_for_detection(dirty_text);
    ASSERT_NE(cleaned, dirty_text);
    ASSERT_GT(cleaned.length(), 0);
    
    // Test confidence adjustment
    double adjusted = language_utils::adjust_confidence_for_length(0.8, 10);
    ASSERT_LT(adjusted, 0.8);  // Short text should have reduced confidence
    
    adjusted = language_utils::adjust_confidence_for_length(0.8, 200);
    ASSERT_EQ(adjusted, 0.8);  // Long text should maintain confidence
}

void test_filter_with_progress_callback() {
    LanguageFilterConfig config;
    config.target_languages = {"en"};
    config.min_confidence = 0.3;
    
    LanguageFilter filter(config);
    
    std::vector<Document> documents = {
        Document("English text one", 1),
        Document("English text two", 2),
        Document("English text three", 3)
    };
    
    size_t progress_calls = 0;
    
    auto progress_callback = [&progress_calls](size_t current, size_t total, const std::string& stage) {
        progress_calls++;
        ASSERT_LE(current, total);
        ASSERT_GT(total, 0);
        ASSERT_FALSE(stage.empty());
    };
    
    LanguageFilterResult result = filter.filter(documents, progress_callback);
    
    ASSERT_EQ(result.total_processed, 3);
    ASSERT_EQ(progress_calls, 3);  // Should be called for each document
}

void test_edge_cases() {
    LanguageFilter filter;
    
    // Test empty documents
    std::vector<Document> empty_docs = {};
    LanguageFilterResult result = filter.filter(empty_docs);
    ASSERT_EQ(result.total_processed, 0);
    ASSERT_EQ(result.total_kept, 0);
    ASSERT_EQ(result.total_rejected, 0);
    
    // Test documents with empty content
    std::vector<Document> empty_content = {
        Document("", 1),
        Document("   ", 2),  // Just whitespace
        Document("\n\t", 3)  // Just whitespace chars
    };
    
    result = filter.filter(empty_content);
    ASSERT_EQ(result.total_processed, 3);
    ASSERT_EQ(result.total_rejected, 3);  // All should be rejected for being too short
    
    // Test very long document
    std::string long_text(10000, 'a');  // Very long but not meaningful
    std::vector<Document> long_docs = {Document(long_text, 1)};
    result = filter.filter(long_docs);
    ASSERT_EQ(result.total_processed, 1);
}

void test_performance_benchmark() {
    LanguageFilter filter;
    
    // Generate test documents
    std::vector<Document> documents;
    documents.reserve(1000);
    
    std::vector<std::string> sample_texts = {
        "This is an English document with common English words and phrases.",
        "Este es un documento en español con palabras y frases comunes en español.",
        "Ceci est un document français avec des mots et phrases français communs.",
        "Dies ist ein deutsches Dokument mit gemeinsamen deutschen Wörtern und Phrasen."
    };
    
    for (int i = 0; i < 1000; ++i) {
        documents.emplace_back(sample_texts[i % sample_texts.size()], i);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    LanguageFilterResult result = filter.filter(documents);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_EQ(result.total_processed, 1000);
    ASSERT_LT(duration.count(), 5000);  // Should complete in under 5 seconds
    
    std::cout << "  Language filtering performance: " << documents.size() 
              << " documents in " << duration.count() << "ms" 
              << " (" << (documents.size() * 1000.0 / duration.count()) << " docs/sec)" << std::endl;
}

int main() {
    std::cout << "🌍 RapidSift Language Filter Test Suite" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    TestSuite suite("Language Filter Tests");
    
    suite.add_test("Simple language detector", test_simple_language_detector);
    suite.add_test("FastText detector fallback", test_fasttext_detector_fallback);
    suite.add_test("Language filter configuration", test_language_filter_config);
    suite.add_test("Basic language filtering", test_basic_language_filtering);
    suite.add_test("Language detection accuracy", test_language_detection_accuracy);
    suite.add_test("Minimum text length filtering", test_minimum_text_length_filtering);
    suite.add_test("Confidence threshold filtering", test_confidence_threshold_filtering);
    suite.add_test("Multilingual document handling", test_multilingual_document_handling);
    suite.add_test("Language statistics", test_language_statistics);
    suite.add_test("Language utilities", test_language_utilities);
    suite.add_test("Filter with progress callback", test_filter_with_progress_callback);
    suite.add_test("Edge cases", test_edge_cases);
    suite.add_test("Performance benchmark", test_performance_benchmark);
    
    suite.run_all();
    
    return 0;
} 