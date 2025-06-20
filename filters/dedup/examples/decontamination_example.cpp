#include "rapidsift/decontamination_filter.hpp"
#include "rapidsift/common.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

using namespace rapidsift::dedup;

int main() {
    std::cout << "RapidSift Decontamination Filter Example\n";
    std::cout << "========================================\n\n";
    
    // Create sample training documents
    std::vector<Document> training_documents = {
        // Clean training data
        Document("train1", 
                "Machine learning is a subset of artificial intelligence that focuses on algorithms "
                "that can learn and improve from experience without being explicitly programmed.",
                "https://ml-tutorial.com/basics"),
                
        // Document with potential benchmark contamination (TriviaQA style)
        Document("train2",
                "Question: What is the capital of France? Answer: The capital of France is Paris. "
                "Paris has been the capital since 987 AD and is located in northern France.",
                "https://geography-facts.net/europe"),
                
        // Document with GLUE benchmark contamination
        Document("train3",
                "The movie was terrible. I would not recommend it to anyone. The acting was poor "
                "and the plot made no sense whatsoever. Definitely a waste of time.",
                "https://movie-reviews.com/bad-movie"),
                
        // Document with SQuAD-style contamination
        Document("train4",
                "Natural language processing (NLP) is a subfield of linguistics, computer science, "
                "and artificial intelligence concerned with the interactions between computers and human language. "
                "What is NLP concerned with? The interactions between computers and human language.",
                "https://nlp-guide.org/introduction"),
                
        // Clean scientific content
        Document("train5",
                "Photosynthesis is the process by which plants convert light energy into chemical energy. "
                "This process occurs in chloroplasts and requires carbon dioxide, water, and sunlight.",
                "https://biology-textbook.edu/photosynthesis"),
                
        // Potential math competition contamination
        Document("train6",
                "If a triangle has sides of length 3, 4, and 5, what type of triangle is it? "
                "Since 3² + 4² = 9 + 16 = 25 = 5², this is a right triangle by the Pythagorean theorem.",
                "https://math-problems.net/geometry"),
                
        // Programming content (could be from coding benchmarks)
        Document("train7",
                "def fibonacci(n): if n <= 1: return n; return fibonacci(n-1) + fibonacci(n-2) "
                "This is a recursive implementation of the Fibonacci sequence algorithm.",
                "https://coding-examples.com/algorithms"),
                
        // Clean general knowledge
        Document("train8",
                "The Renaissance was a period of European cultural, artistic, political and economic rebirth "
                "following the Middle Ages, generally spanning from the 14th to the 17th century.",
                "https://history-encyclopedia.org/renaissance")
    };
    
    // Create sample benchmark datasets (these would normally be loaded from files)
    std::vector<std::string> trivia_qa_samples = {
        "What is the capital of France?",
        "The capital of France is Paris",
        "Paris has been the capital since 987 AD",
        "Who wrote Romeo and Juliet?",
        "Shakespeare wrote Romeo and Juliet"
    };
    
    std::vector<std::string> sentiment_analysis_samples = {
        "The movie was terrible",
        "I would not recommend it to anyone",
        "The acting was poor and the plot made no sense",
        "Definitely a waste of time"
    };
    
    std::vector<std::string> squad_samples = {
        "What is NLP concerned with?",
        "The interactions between computers and human language",
        "Natural language processing is a subfield of linguistics",
        "artificial intelligence concerned with the interactions"
    };
    
    std::vector<std::string> math_competition_samples = {
        "If a triangle has sides of length 3, 4, and 5",
        "what type of triangle is it",
        "This is a right triangle by the Pythagorean theorem",
        "3² + 4² = 9 + 16 = 25 = 5²"
    };
    
    std::cout << "=== Testing Different Decontamination Configurations ===\n\n";
    
    // Test 1: Default Configuration
    std::cout << "1. DEFAULT CONFIGURATION\n";
    std::cout << "   - 13-gram overlap detection\n";
    std::cout << "   - Standard contamination threshold\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto default_config = decontamination_utils::create_default_config();
    DecontaminationFilter default_filter(default_config);
    
    // Add benchmark n-grams
    default_filter.add_benchmark_ngrams(trivia_qa_samples, "TriviaQA");
    default_filter.add_benchmark_ngrams(sentiment_analysis_samples, "GLUE-SST");
    default_filter.add_benchmark_ngrams(squad_samples, "SQuAD");
    default_filter.add_benchmark_ngrams(math_competition_samples, "MATH");
    
    std::cout << "Loaded " << default_filter.get_benchmark_ngrams_count() << " benchmark n-grams\n";
    
    size_t default_kept = 0;
    for (const auto& doc : training_documents) {
        auto decision = default_filter.evaluate(doc);
        std::cout << "Doc " << doc.id << ": ";
        
        if (decision.result == FilterResult::KEEP) {
            std::cout << "KEEP - " << decision.details << "\n";
            default_kept++;
        } else {
            std::cout << "REJECT - " << decision.details << "\n";
        }
    }
    
    std::cout << "Default filter kept: " << default_kept << "/" << training_documents.size() << " documents\n\n";
    
    // Test 2: Strict Configuration
    std::cout << "2. STRICT CONFIGURATION\n";
    std::cout << "   - Smaller n-grams (8-gram)\n";
    std::cout << "   - Lower contamination threshold\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto strict_config = decontamination_utils::create_strict_config();
    DecontaminationFilter strict_filter(strict_config);
    
    // Add the same benchmark data
    strict_filter.add_benchmark_ngrams(trivia_qa_samples, "TriviaQA");
    strict_filter.add_benchmark_ngrams(sentiment_analysis_samples, "GLUE-SST");
    strict_filter.add_benchmark_ngrams(squad_samples, "SQuAD");
    strict_filter.add_benchmark_ngrams(math_competition_samples, "MATH");
    
    size_t strict_kept = 0;
    for (const auto& doc : training_documents) {
        auto decision = strict_filter.evaluate(doc);
        std::cout << "Doc " << doc.id << ": ";
        
        if (decision.result == FilterResult::KEEP) {
            std::cout << "KEEP - " << decision.details << "\n";
            strict_kept++;
        } else {
            std::cout << "REJECT - " << decision.details << "\n";
        }
    }
    
    std::cout << "Strict filter kept: " << strict_kept << "/" << training_documents.size() << " documents\n\n";
    
    // Test 3: Fast Configuration
    std::cout << "3. FAST CONFIGURATION\n";
    std::cout << "   - Optimized for speed\n";
    std::cout << "   - Bloom filter enabled\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto fast_config = decontamination_utils::create_fast_config();
    DecontaminationFilter fast_filter(fast_config);
    
    // Add benchmark data
    fast_filter.add_benchmark_ngrams(trivia_qa_samples, "TriviaQA");
    fast_filter.add_benchmark_ngrams(sentiment_analysis_samples, "GLUE-SST");
    fast_filter.add_benchmark_ngrams(squad_samples, "SQuAD");
    fast_filter.add_benchmark_ngrams(math_competition_samples, "MATH");
    
    size_t fast_kept = 0;
    for (const auto& doc : training_documents) {
        auto decision = fast_filter.evaluate(doc);
        std::cout << "Doc " << doc.id << ": ";
        
        if (decision.result == FilterResult::KEEP) {
            std::cout << "KEEP - " << decision.details << "\n";
            fast_kept++;
        } else {
            std::cout << "REJECT - " << decision.details << "\n";
        }
    }
    
    std::cout << "Fast filter kept: " << fast_kept << "/" << training_documents.size() << " documents\n\n";
    
    // Test 4: Detailed Contamination Assessment
    std::cout << "4. DETAILED CONTAMINATION ASSESSMENT\n";
    std::cout << "   - Shows exact matches and sources\n";
    std::cout << std::string(50, '-') << "\n";
    
    for (size_t i = 0; i < std::min(size_t(4), training_documents.size()); ++i) {
        const auto& doc = training_documents[i];
        
        std::cout << "\nAnalyzing Document: " << doc.id << "\n";
        std::cout << "Text: " << doc.text.substr(0, 100) << "...\n";
        
        auto assessment = default_filter.assess_document(doc);
        
        std::cout << "Contamination Analysis:\n";
        std::cout << "  Is Contaminated: " << (assessment.is_contaminated ? "YES" : "NO") << "\n";
        std::cout << "  Contamination Score: " << assessment.contamination_score << "\n";
        std::cout << "  Total N-grams Checked: " << assessment.total_ngrams_checked << "\n";
        std::cout << "  Contaminated N-grams: " << assessment.contaminated_ngrams << "\n";
        
        if (!assessment.most_likely_source.empty()) {
            std::cout << "  Most Likely Source: " << assessment.most_likely_source << "\n";
        }
        
        if (!assessment.matches.empty()) {
            std::cout << "  Contamination Matches:\n";
            for (size_t j = 0; j < std::min(size_t(3), assessment.matches.size()); ++j) {
                const auto& match = assessment.matches[j];
                std::cout << "    - \"" << match.ngram.substr(0, 50) << "...\" from " 
                          << match.source_dataset << "\n";
            }
        }
    }
    
    // Test 5: N-gram Extraction Demo
    std::cout << "\n5. N-GRAM EXTRACTION DEMONSTRATION\n";
    std::cout << std::string(50, '-') << "\n";
    
    std::string sample_text = "The quick brown fox jumps over the lazy dog.";
    std::cout << "Sample text: \"" << sample_text << "\"\n\n";
    
    // Test different n-gram sizes
    for (size_t n = 3; n <= 6; ++n) {
        auto ngrams = default_filter.extract_ngrams(sample_text, n);
        std::cout << n << "-grams (" << ngrams.size() << " total):\n";
        for (size_t i = 0; i < std::min(size_t(5), ngrams.size()); ++i) {
            std::cout << "  \"" << ngrams[i] << "\"\n";
        }
        if (ngrams.size() > 5) {
            std::cout << "  ... and " << (ngrams.size() - 5) << " more\n";
        }
        std::cout << "\n";
    }
    
    // Test 6: Batch Processing
    std::cout << "6. BATCH PROCESSING DEMONSTRATION\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto batch_assessments = default_filter.assess_documents(training_documents);
    std::cout << "Batch processed " << batch_assessments.size() << " documents\n";
    
    size_t contaminated_count = 0;
    for (const auto& assessment : batch_assessments) {
        if (assessment.is_contaminated) {
            contaminated_count++;
        }
    }
    
    std::cout << "Found " << contaminated_count << " contaminated documents out of " 
              << batch_assessments.size() << " total\n";
    
    // Show contamination by dataset
    std::cout << "\nContamination by benchmark dataset:\n";
    std::unordered_map<std::string, size_t> dataset_contamination;
    for (const auto& assessment : batch_assessments) {
        if (!assessment.most_likely_source.empty()) {
            dataset_contamination[assessment.most_likely_source]++;
        }
    }
    
    for (const auto& [dataset, count] : dataset_contamination) {
        std::cout << "  " << dataset << ": " << count << " documents\n";
    }
    
    // Show comprehensive statistics
    std::cout << "\n=== DECONTAMINATION STATISTICS ===\n";
    
    std::cout << "\nDefault Filter Report:\n";
    default_filter.print_contamination_report();
    
    std::cout << "\nStrict Filter Report:\n";
    strict_filter.print_contamination_report();
    
    std::cout << "\nFast Filter Report:\n";
    fast_filter.print_contamination_report();
    
    // Demonstrate utility functions
    std::cout << "\n=== UTILITY FUNCTIONS DEMO ===\n";
    
    // Test n-gram generation utilities
    std::string test_text = "Natural language processing enables machines to understand text.";
    auto utility_ngrams = decontamination_utils::generate_ngrams(test_text, 5, true, true);
    
    std::cout << "Generated " << utility_ngrams.size() << " 5-grams from utility function:\n";
    for (size_t i = 0; i < std::min(size_t(3), utility_ngrams.size()); ++i) {
        std::cout << "  \"" << utility_ngrams[i] << "\"\n";
    }
    
    // Test text normalization
    std::string messy_text = "  This   has    IRREGULAR   spacing!!!  ";
    std::string normalized = decontamination_utils::normalize_text(messy_text, true, true, true);
    std::cout << "\nText normalization demo:\n";
    std::cout << "Original: \"" << messy_text << "\"\n";
    std::cout << "Normalized: \"" << normalized << "\"\n";
    
    // Test tokenization
    auto tokens = decontamination_utils::simple_tokenize("Hello world! How are you today?");
    std::cout << "\nTokenization demo:\n";
    std::cout << "Tokens: ";
    for (const auto& token : tokens) {
        std::cout << "\"" << token << "\" ";
    }
    std::cout << "\n";
    
    std::cout << "\n=== PERFORMANCE ANALYSIS ===\n";
    
    // Compare performance metrics
    const auto& default_stats = default_filter.get_stats();
    const auto& strict_stats = strict_filter.get_stats();
    const auto& fast_stats = fast_filter.get_stats();
    
    std::cout << "Performance Comparison:\n";
    std::cout << "Configuration    | Avg Time (ms) | Contamination Rate | N-grams/sec\n";
    std::cout << "-----------------|---------------|--------------------|-----------\n";
    std::cout << "Default          | " << default_stats.average_processing_time_ms << "         | " 
              << (default_stats.get_contamination_rate() * 100) << "%              | " 
              << (default_stats.total_ngrams_checked / std::max(1.0, default_stats.average_processing_time_ms / 1000)) << "\n";
    std::cout << "Strict           | " << strict_stats.average_processing_time_ms << "         | " 
              << (strict_stats.get_contamination_rate() * 100) << "%              | " 
              << (strict_stats.total_ngrams_checked / std::max(1.0, strict_stats.average_processing_time_ms / 1000)) << "\n";
    std::cout << "Fast             | " << fast_stats.average_processing_time_ms << "         | " 
              << (fast_stats.get_contamination_rate() * 100) << "%              | " 
              << (fast_stats.total_ngrams_checked / std::max(1.0, fast_stats.average_processing_time_ms / 1000)) << "\n";
    
    std::cout << "\n=== BEST PRACTICES SUMMARY ===\n";
    std::cout << "1. Use 13-grams for balance between precision and recall\n";
    std::cout << "2. Enable bloom filters for large-scale processing\n";
    std::cout << "3. Adjust contamination threshold based on use case\n";
    std::cout << "4. Process benchmark datasets in standardized format\n";
    std::cout << "5. Monitor false positive rates and adjust accordingly\n";
    std::cout << "6. Regularly update benchmark datasets as new evaluations emerge\n";
    std::cout << "7. Document which benchmarks were used for decontamination\n";
    std::cout << "8. Consider approximate matching for robustness\n";
    
    return 0;
} 