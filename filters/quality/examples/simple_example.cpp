#include "rapidsift/quality_processor.hpp"
#include "rapidsift/quality_common.hpp"
#include <iostream>
#include <vector>

using namespace rapidsift::quality;

int main() {
    std::cout << "=== RapidSift Quality Filter - Simple Example ===" << std::endl;
    
    // Create sample documents with various quality issues
    std::vector<Document> documents = {
        Document("doc1", "This is a high-quality document with proper grammar, reasonable length, and meaningful content. It provides valuable information and demonstrates good writing practices."),
        Document("doc2", "Hi"),  // Too short
        Document("doc3", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),  // Gibberish
        Document("doc4", "Click here! Click here! Click here! Click here! Click here! Click here! Click here! Click here! Click here! Click here!"),  // High repetition
        Document("doc5", "<html><body><div><p>Some content</p><p>More content</p></div></body></html>"),  // Mostly HTML
        Document("doc6", "function test() { return 42; } class MyClass { public: void method() { std::cout << \"Hello\"; } };"),  // Code
        Document("doc7", "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris."),  // Good quality
        Document("doc8", "SPAM SPAM SPAM BUY NOW VIAGRA CIALIS CHEAP LOANS MAKE MONEY FAST!!!", "https://spam-site.com/ad", "spam-site.com"),  // Spam
    };
    
    std::cout << "Created " << documents.size() << " test documents" << std::endl;
    
    // Create processor with default configuration
    QualityConfig config;
    config.verbose = true;
    QualityProcessor processor(config);
    
    std::cout << "\n=== Analyzing Documents ===" << std::endl;
    
    // Assess each document individually
    for (size_t i = 0; i < documents.size(); ++i) {
        const auto& doc = documents[i];
        auto assessment = processor.assess_document(doc);
        
        std::cout << "\nDocument " << doc.id << ":" << std::endl;
        std::cout << "  Text: \"" << (doc.text.length() > 50 ? doc.text.substr(0, 50) + "..." : doc.text) << "\"" << std::endl;
        std::cout << "  Result: " << (assessment.final_result == FilterResult::KEEP ? "KEEP" : "REJECT") << std::endl;
        std::cout << "  Overall Score: " << assessment.overall_score << std::endl;
        
        // Show individual filter decisions
        for (const auto& decision : assessment.filter_decisions) {
            if (decision.result == FilterResult::REJECT) {
                std::string reason;
                switch (decision.reason) {
                    case RejectionReason::TOO_SHORT: reason = "Too Short"; break;
                    case RejectionReason::TOO_LONG: reason = "Too Long"; break;
                    case RejectionReason::GIBBERISH: reason = "Gibberish"; break;
                    case RejectionReason::HIGH_REPETITION: reason = "High Repetition"; break;
                    case RejectionReason::POOR_FORMATTING: reason = "Poor Formatting"; break;
                    case RejectionReason::SUSPICIOUS_URL: reason = "Suspicious URL"; break;
                    default: reason = "Other"; break;
                }
                std::cout << "    Rejected by: " << reason << " (" << decision.details << ")" << std::endl;
            }
        }
        
        // Show some metrics
        if (!assessment.feature_scores.empty()) {
            std::cout << "  Metrics:" << std::endl;
            for (const auto& metric : assessment.feature_scores) {
                std::cout << "    " << metric.first << ": " << metric.second << std::endl;
            }
        }
    }
    
    std::cout << "\n=== Batch Processing ===" << std::endl;
    
    // Filter documents in batch
    auto filtered_documents = processor.filter_documents(documents);
    
    std::cout << "Input documents: " << documents.size() << std::endl;
    std::cout << "Filtered documents: " << filtered_documents.size() << std::endl;
    std::cout << "Rejection rate: " << (100.0 * (documents.size() - filtered_documents.size()) / documents.size()) << "%" << std::endl;
    
    std::cout << "\n=== High-Quality Documents ===" << std::endl;
    for (const auto& doc : filtered_documents) {
        std::cout << "  " << doc.id << ": \"" << (doc.text.length() > 60 ? doc.text.substr(0, 60) + "..." : doc.text) << "\"" << std::endl;
    }
    
    // Show statistics
    std::cout << "\n=== Statistics ===" << std::endl;
    const auto& stats = processor.get_stats();
    std::cout << "Total processed: " << stats.total_processed << std::endl;
    std::cout << "Kept: " << stats.kept << " (" << (stats.get_keep_rate() * 100) << "%)" << std::endl;
    std::cout << "Rejected: " << stats.rejected << " (" << (stats.get_rejection_rate() * 100) << "%)" << std::endl;
    
    if (!stats.rejection_counts.empty()) {
        std::cout << "\nRejection reasons:" << std::endl;
        for (const auto& pair : stats.rejection_counts) {
            std::string reason;
            switch (pair.first) {
                case RejectionReason::TOO_SHORT: reason = "Too Short"; break;
                case RejectionReason::TOO_LONG: reason = "Too Long"; break;
                case RejectionReason::GIBBERISH: reason = "Gibberish"; break;
                case RejectionReason::HIGH_REPETITION: reason = "High Repetition"; break;
                case RejectionReason::POOR_FORMATTING: reason = "Poor Formatting"; break;
                case RejectionReason::SUSPICIOUS_URL: reason = "Suspicious URL"; break;
                default: reason = "Other"; break;
            }
            std::cout << "  " << reason << ": " << pair.second << std::endl;
        }
    }
    
    std::cout << "\n=== Custom Configuration Example ===" << std::endl;
    
    // Create a more strict configuration
    QualityConfig strict_config;
    strict_config.length_config.min_words = 10;  // Require at least 10 words
    strict_config.gibberish_config.min_entropy = 3.0;  // Higher entropy requirement
    strict_config.repetition_config.min_unique_word_ratio = 0.5;  // 50% unique words
    
    QualityProcessor strict_processor(strict_config);
    auto strict_filtered = strict_processor.filter_documents(documents);
    
    std::cout << "Strict filtering kept: " << strict_filtered.size() << "/" << documents.size() 
              << " documents (" << (100.0 * strict_filtered.size() / documents.size()) << "%)" << std::endl;
    
    return 0;
} 