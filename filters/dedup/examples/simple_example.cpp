#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "../include/rapidsift/exact_dedup.hpp"
#include "../include/rapidsift/near_dedup.hpp"

using namespace rapidsift;

void demonstrate_exact_deduplication() {
    std::cout << "=== Exact Deduplication Demo ===\n\n";
    
    // Create sample documents with duplicates
    std::vector<Document> documents = {
        Document("This is a sample document.", 0),
        Document("Another document with different content.", 1),
        Document("This is a sample document.", 2),  // Exact duplicate
        Document("Yet another unique document.", 3),
        Document("Another document with different content.", 4),  // Exact duplicate
        Document("Final document in the collection.", 5)
    };
    
    std::cout << "Original documents: " << documents.size() << "\n";
    for (size_t i = 0; i < documents.size(); ++i) {
        std::cout << i << ": " << documents[i].text() << "\n";
    }
    
    // Configure and run exact deduplication
    ExactDedupConfig config;
    config.algorithm = HashAlgorithm::XXHASH64;
    config.parallel = true;
    
    ExactDeduplicator deduplicator(config);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = deduplicator.deduplicate(documents);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nResults:\n";
    std::cout << "Unique documents: " << result.unique_count() << "\n";
    std::cout << "Duplicates removed: " << result.duplicates_removed() << "\n";
    std::cout << "Processing time: " << duration.count() << " microseconds\n\n";
    
    std::cout << "Unique documents:\n";
    for (size_t i = 0; i < result.unique_documents().size(); ++i) {
        std::cout << i << ": " << result.unique_documents()[i].text() << "\n";
    }
    std::cout << "\n";
}

void demonstrate_near_deduplication() {
    std::cout << "=== Near-Duplicate Detection Demo ===\n\n";
    
    // Create sample documents with near duplicates
    std::vector<Document> documents = {
        Document("The quick brown fox jumps over the lazy dog.", 0),
        Document("The quick brown fox jumped over the lazy dog.", 1),  // Near duplicate
        Document("A completely different sentence about cats.", 2),
        Document("The quick brown fox leaps over the lazy dog.", 3),   // Near duplicate
        Document("Another sentence about cats and dogs.", 4),
        Document("The weather is nice today.", 5)
    };
    
    std::cout << "Original documents: " << documents.size() << "\n";
    for (size_t i = 0; i < documents.size(); ++i) {
        std::cout << i << ": " << documents[i].text() << "\n";
    }
    
    // Configure and run near deduplication using MinHash
    NearDedupConfig config;
    config.method = NearDedupConfig::Method::MINHASH;
    config.threshold = 0.7;  // 70% similarity threshold
    config.num_permutations = 128;
    
    NearDeduplicator deduplicator(config);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = deduplicator.deduplicate(documents);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nMinHash Results (threshold: " << config.threshold << "):\n";
    std::cout << "Unique documents: " << result.unique_count() << "\n";
    std::cout << "Duplicates removed: " << result.duplicates_removed() << "\n";
    std::cout << "Processing time: " << duration.count() << " microseconds\n\n";
    
    std::cout << "Unique documents:\n";
    for (size_t i = 0; i < result.unique_documents().size(); ++i) {
        std::cout << i << ": " << result.unique_documents()[i].text() << "\n";
    }
    
    std::cout << "\nDuplicate groups found:\n";
    for (size_t i = 0; i < result.duplicate_groups().size(); ++i) {
        std::cout << "Group " << i << ": ";
        for (DocumentId doc_id : result.duplicate_groups()[i]) {
            std::cout << doc_id << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void demonstrate_performance_comparison() {
    std::cout << "=== Performance Comparison ===\n\n";
    
    // Generate larger test dataset
    std::vector<Document> large_dataset;
    std::vector<std::string> base_texts = {
        "The quick brown fox jumps over the lazy dog.",
        "A journey of a thousand miles begins with a single step.",
        "To be or not to be, that is the question.",
        "All that glitters is not gold.",
        "The early bird catches the worm."
    };
    
    // Create dataset with duplicates and variations
    for (int i = 0; i < 1000; ++i) {
        std::string text = base_texts[i % base_texts.size()];
        if (i % 3 == 0) {
            text += " Additional text to make it slightly different.";
        }
        large_dataset.emplace_back(text, i);
    }
    
    std::cout << "Testing with " << large_dataset.size() << " documents\n\n";
    
    // Test exact deduplication
    {
        ExactDedupConfig config;
        config.algorithm = HashAlgorithm::XXHASH64;
        ExactDeduplicator deduplicator(config);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto result = deduplicator.deduplicate(large_dataset);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Exact Deduplication (xxHash):\n";
        std::cout << "  Unique documents: " << result.unique_count() << "\n";
        std::cout << "  Reduction: " << std::fixed << std::setprecision(1) 
                  << result.reduction_percentage() << "%\n";
        std::cout << "  Time: " << duration.count() << " ms\n\n";
    }
    
    // Test MinHash near deduplication
    {
        NearDedupConfig config;
        config.method = NearDedupConfig::Method::MINHASH;
        config.threshold = 0.8;
        NearDeduplicator deduplicator(config);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto result = deduplicator.deduplicate(large_dataset);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Near Deduplication (MinHash):\n";
        std::cout << "  Unique documents: " << result.unique_count() << "\n";
        std::cout << "  Reduction: " << std::fixed << std::setprecision(1) 
                  << result.reduction_percentage() << "%\n";
        std::cout << "  Time: " << duration.count() << " ms\n\n";
    }
}

int main() {
    std::cout << "RapidSift C++ - High-Performance Text Deduplication\n";
    std::cout << "===================================================\n\n";
    
    try {
        demonstrate_exact_deduplication();
        demonstrate_near_deduplication();
        demonstrate_performance_comparison();
        
        std::cout << "Demo completed successfully!\n";
        std::cout << "\nTo build and run:\n";
        std::cout << "  cd cpp && mkdir build && cd build\n";
        std::cout << "  cmake .. && make\n";
        std::cout << "  ./examples/simple_example\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 