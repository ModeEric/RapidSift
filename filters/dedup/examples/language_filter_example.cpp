#include <iostream>
#include <vector>

#include "rapidsift/common.hpp"
#include "rapidsift/language_filter.hpp"

using namespace rapidsift;

int main() {
    std::cout << "🌍 RapidSift Language Filtering Example" << std::endl;
    std::cout << "=======================================" << std::endl << std::endl;

    // Create sample multilingual documents
    std::vector<Document> documents = {
        Document("The quick brown fox jumps over the lazy dog. This is clearly English text.", 1),
        Document("El rápido zorro marrón salta sobre el perro perezoso. Este es texto en español.", 2),
        Document("Le renard brun rapide saute par-dessus le chien paresseux. C'est du texte français.", 3),
        Document("Der schnelle braune Fuchs springt über den faulen Hund. Das ist deutscher Text.", 4),
        Document("Il veloce volpe marrone salta sopra il cane pigro. Questo è testo italiano.", 5),
        Document("Mixed language text with English and español palabras mezcladas together.", 6),
        Document("Short", 7),  // Too short for reliable detection
        Document("This is another clear English sentence with common words and patterns.", 8),
        Document("Een snelle bruine vos springt over de luie hond. Dit is Nederlandse tekst.", 9),
        Document("Быстрая коричневая лиса прыгает через ленивую собаку. Это русский текст.", 10)
    };

    std::cout << "📚 Sample Documents (" << documents.size() << " total):" << std::endl;
    for (const auto& doc : documents) {
        std::cout << "  [" << doc.id() << "] " << doc.text().substr(0, 50) << "..." << std::endl;
    }
    std::cout << std::endl;

    // Example 1: Language statistics
    std::cout << "🔍 Example 1: Language Statistics" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    LanguageFilter filter;
    auto stats = filter.get_language_stats(documents);
    
    std::cout << "Language distribution:" << std::endl;
    for (const auto& [lang, count] : stats) {
        std::string lang_name = language_utils::iso639_1_to_name(lang);
        double percentage = (double(count) / documents.size()) * 100.0;
        std::cout << "  " << lang << " (" << lang_name << "): " 
                 << count << " documents (" << std::fixed << std::setprecision(1) 
                 << percentage << "%)" << std::endl;
    }
    std::cout << std::endl;

    // Example 2: Filter for English only
    std::cout << "🇺🇸 Example 2: Filter English Documents (min confidence 0.3)" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    LanguageFilterConfig english_config;
    english_config.target_languages = {"en"};
    english_config.min_confidence = 0.3;
    english_config.min_text_length = 10;
    
    LanguageFilter english_filter(english_config);
    auto english_result = english_filter.filter(documents);
    
    std::cout << "Results:" << std::endl;
    std::cout << "  Total processed: " << english_result.total_processed << std::endl;
    std::cout << "  English documents: " << english_result.total_kept << std::endl;
    std::cout << "  Rejected: " << english_result.total_rejected << std::endl;
    std::cout << "  Success rate: " << std::fixed << std::setprecision(1) 
              << english_result.kept_percentage() << "%" << std::endl;
    
    std::cout << "\nKept English documents:" << std::endl;
    for (const auto& doc : english_result.filtered_documents) {
        std::cout << "  [" << doc.id() << "] " << doc.text().substr(0, 60) << "..." << std::endl;
    }
    std::cout << std::endl;

    // Example 3: Filter multiple languages
    std::cout << "🌍 Example 3: Filter European Languages (EN, ES, FR, DE, IT)" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    LanguageFilterConfig european_config;
    european_config.target_languages = {"en", "es", "fr", "de", "it"};
    european_config.min_confidence = 0.2;  // Lower threshold for demonstration
    european_config.remove_mixed_language = true;
    
    LanguageFilter european_filter(european_config);
    auto european_result = european_filter.filter(documents);
    
    std::cout << "Results:" << std::endl;
    std::cout << "  Total processed: " << european_result.total_processed << std::endl;
    std::cout << "  European language documents: " << european_result.total_kept << std::endl;
    std::cout << "  Rejected: " << european_result.total_rejected << std::endl;
    std::cout << "  Success rate: " << std::fixed << std::setprecision(1) 
              << european_result.kept_percentage() << "%" << std::endl;
    
    std::cout << "\nLanguage breakdown:" << std::endl;
    for (const auto& [lang, count] : european_result.language_counts) {
        if (count > 0) {
            std::string lang_name = language_utils::iso639_1_to_name(lang);
            std::cout << "  " << lang << " (" << lang_name << "): " << count << " documents" << std::endl;
        }
    }
    std::cout << std::endl;

    // Example 4: High-confidence filtering
    std::cout << "🎯 Example 4: High-Confidence Filtering (min confidence 0.8)" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    LanguageFilterConfig strict_config;
    strict_config.min_confidence = 0.8;  // Very strict
    strict_config.min_text_length = 20;   // Longer minimum length
    
    LanguageFilter strict_filter(strict_config);
    auto strict_result = strict_filter.filter(documents);
    
    std::cout << "Results with strict filtering:" << std::endl;
    std::cout << "  Total processed: " << strict_result.total_processed << std::endl;
    std::cout << "  High-confidence documents: " << strict_result.total_kept << std::endl;
    std::cout << "  Rejected: " << strict_result.total_rejected << std::endl;
    std::cout << "  Average confidence: " << std::fixed << std::setprecision(3) 
              << strict_result.average_confidence << std::endl;
    std::cout << std::endl;

    // Example 5: Language utilities demonstration
    std::cout << "🛠️ Example 5: Language Utilities" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    std::vector<std::string> test_codes = {"en", "es", "fr", "de", "it", "unknown"};
    std::cout << "Language code conversion:" << std::endl;
    for (const auto& code : test_codes) {
        std::string name = language_utils::iso639_1_to_name(code);
        std::cout << "  " << code << " -> " << name << std::endl;
    }
    
    std::cout << "\nText cleaning example:" << std::endl;
    std::string dirty_text = "  Hello world!  Visit https://example.com or email test@example.com  ";
    std::string cleaned = language_utils::clean_text_for_detection(dirty_text);
    std::cout << "  Before: \"" << dirty_text << "\"" << std::endl;
    std::cout << "  After:  \"" << cleaned << "\"" << std::endl;
    
    std::cout << "\nConfidence adjustment for text length:" << std::endl;
    double base_confidence = 0.8;
    std::vector<size_t> lengths = {5, 20, 50, 100, 200};
    for (size_t len : lengths) {
        double adjusted = language_utils::adjust_confidence_for_length(base_confidence, len);
        std::cout << "  " << len << " chars: " << std::fixed << std::setprecision(3) 
                 << base_confidence << " -> " << adjusted << std::endl;
    }
    std::cout << std::endl;

    std::cout << "✅ Language filtering examples completed!" << std::endl;
    std::cout << std::endl;
    std::cout << "🚀 Integration Tips:" << std::endl;
    std::cout << "  • Use --mode language for CLI language filtering" << std::endl;
    std::cout << "  • Combine with deduplication: filter by language first, then deduplicate" << std::endl;
    std::cout << "  • Adjust confidence thresholds based on your data quality needs" << std::endl;
    std::cout << "  • Use --lang-stats to analyze your dataset's language distribution" << std::endl;
    std::cout << "  • For production: install fastText language model for better accuracy" << std::endl;
    
    return 0;
} 