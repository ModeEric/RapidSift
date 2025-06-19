#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

// Utility to create test data for RapidSift testing
class TestDataGenerator {
public:
    static void generate_all_test_files() {
        std::cout << "🧪 Generating RapidSift Test Data" << std::endl;
        std::cout << "=================================" << std::endl;
        
        generate_exact_duplicates();
        generate_near_duplicates();
        generate_mixed_content();
        generate_large_dataset();
        generate_unicode_data();
        
        std::cout << "✅ All test data files generated successfully!" << std::endl;
    }

private:
    static void generate_exact_duplicates() {
        std::cout << "Creating exact duplicates test file..." << std::endl;
        
        std::ofstream file("test_exact_duplicates.txt");
        
        // Create documents with exact duplicates
        std::vector<std::string> documents = {
            "The quick brown fox jumps over the lazy dog.",
            "A journey of a thousand miles begins with a single step.",
            "The quick brown fox jumps over the lazy dog.",  // Duplicate
            "To be or not to be, that is the question.",
            "A journey of a thousand miles begins with a single step.",  // Duplicate
            "All that glitters is not gold.",
            "The quick brown fox jumps over the lazy dog.",  // Another duplicate
            "The early bird catches the worm.",
            "Actions speak louder than words.",
            "All that glitters is not gold."  // Duplicate
        };
        
        for (const auto& doc : documents) {
            file << doc << std::endl;
        }
        file.close();
        
        std::cout << "  ✓ Created test_exact_duplicates.txt (10 docs, 4 duplicates)" << std::endl;
    }
    
    static void generate_near_duplicates() {
        std::cout << "Creating near duplicates test file..." << std::endl;
        
        std::ofstream file("test_near_duplicates.txt");
        
        std::vector<std::string> documents = {
            "The quick brown fox jumps over the lazy dog",
            "The quick brown fox leaps over the lazy dog",  // Similar
            "The fast brown fox jumps over the lazy dog",   // Similar
            "A completely different sentence here",
            "The quick brown fox runs over the lazy dog",   // Similar
            "Another totally unrelated text content",
            "The quick brown fox jumps over the sleepy dog", // Similar
            "Yet another different piece of text",
            "The quick brown fox jumps over the lazy cat",  // Similar
            "Completely unrelated content for testing"
        };
        
        for (const auto& doc : documents) {
            file << doc << std::endl;
        }
        file.close();
        
        std::cout << "  ✓ Created test_near_duplicates.txt (10 docs, 5 similar groups)" << std::endl;
    }
    
    static void generate_mixed_content() {
        std::cout << "Creating mixed content test file..." << std::endl;
        
        std::ofstream file("test_mixed_content.txt");
        
        std::vector<std::string> documents = {
            "",  // Empty string
            "   ",  // Whitespace only
            "Single word",
            "Single word",  // Exact duplicate
            "Short text",
            "A much longer piece of text that contains multiple sentences and various punctuation marks, including commas, periods, and exclamation points!",
            "Short text",  // Duplicate
            "UPPERCASE TEXT",
            "lowercase text",
            "Mixed CaSe TeXt",
            "Numbers 123 and symbols !@#$%",
            "Numbers 123 and symbols !@#$%",  // Duplicate
            "Text with\nnewlines\nand\ttabs",
            "Final test document"
        };
        
        for (const auto& doc : documents) {
            file << doc << std::endl;
        }
        file.close();
        
        std::cout << "  ✓ Created test_mixed_content.txt (14 docs, various formats)" << std::endl;
    }
    
    static void generate_large_dataset() {
        std::cout << "Creating large dataset test file..." << std::endl;
        
        std::ofstream file("test_large_dataset.txt");
        
        // Base patterns
        std::vector<std::string> base_patterns = {
            "Pattern A: This is a common document pattern that appears frequently",
            "Pattern B: Another type of document with different content structure",
            "Pattern C: Third variation with unique characteristics and formatting",
            "Pattern D: Fourth pattern type with specific content organization",
            "Pattern E: Fifth pattern variant with distinctive text elements"
        };
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> pattern_dist(0, base_patterns.size() - 1);
        std::uniform_int_distribution<> unique_dist(0, 99);
        
        // Generate 1000 documents
        for (int i = 0; i < 1000; ++i) {
            std::string doc;
            
            if (i % 10 == 0) {
                // 10% completely unique
                doc = "Unique document " + std::to_string(i) + " with special content for ID " + std::to_string(i);
            } else {
                // 90% based on patterns
                int pattern_idx = pattern_dist(gen);
                doc = base_patterns[pattern_idx];
                
                // Add some variation to avoid being too similar
                if (unique_dist(gen) < 30) {  // 30% get variation
                    doc += " - variation " + std::to_string(i % 20);
                }
            }
            
            file << doc << std::endl;
        }
        file.close();
        
        std::cout << "  ✓ Created test_large_dataset.txt (1000 docs, ~10% unique)" << std::endl;
    }
    
    static void generate_unicode_data() {
        std::cout << "Creating Unicode test file..." << std::endl;
        
        std::ofstream file("test_unicode.txt");
        
        std::vector<std::string> documents = {
            "Hello World",  // English
            "Hola Mundo",   // Spanish
            "Bonjour le monde",  // French
            "Hallo Welt",   // German
            "Ciao mondo",   // Italian
            "Olá mundo",    // Portuguese
            "Привет мир",   // Russian
            "你好世界",      // Chinese
            "こんにちは世界", // Japanese
            "안녕하세요 세계", // Korean
            "مرحبا بالعالم",  // Arabic
            "שלום עולם",     // Hebrew
            "Hello World",  // Duplicate (English)
            "Hola Mundo",   // Duplicate (Spanish)
            "Здравствуй мир", // Russian variation
            "नमस्ते दुनिया",   // Hindi
            "Γεια σου κόσμε", // Greek
            "Hej världen",   // Swedish
            "Привет мир"     // Duplicate (Russian)
        };
        
        for (const auto& doc : documents) {
            file << doc << std::endl;
        }
        file.close();
        
        std::cout << "  ✓ Created test_unicode.txt (19 docs, multiple languages)" << std::endl;
    }
};

int main() {
    TestDataGenerator::generate_all_test_files();
    
    std::cout << std::endl;
    std::cout << "📁 Generated test data files:" << std::endl;
    std::cout << "  • test_exact_duplicates.txt  - For testing exact deduplication" << std::endl;
    std::cout << "  • test_near_duplicates.txt   - For testing near-duplicate detection" << std::endl;
    std::cout << "  • test_mixed_content.txt     - For testing various content types" << std::endl;
    std::cout << "  • test_large_dataset.txt     - For performance testing" << std::endl;
    std::cout << "  • test_unicode.txt           - For Unicode/international text testing" << std::endl;
    std::cout << std::endl;
    std::cout << "🚀 Ready to test RapidSift with comprehensive data!" << std::endl;
    
    return 0;
} 