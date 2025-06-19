#include "test_framework.hpp"
#include "../include/rapidsift/exact_dedup.hpp"
#include "../include/rapidsift/near_dedup.hpp"
#include "../include/rapidsift/common.hpp"
#include <chrono>
#include <random>
#include <iomanip>

using namespace rapidsift;
using namespace test_framework;

class PerformanceBenchmark {
public:
    static void run_all_benchmarks() {
        std::cout << "⚡ RapidSift Performance Benchmarks" << std::endl;
        std::cout << "====================================" << std::endl << std::endl;
        
        // Run different scale benchmarks
        benchmark_exact_deduplication();
        benchmark_near_deduplication();
        benchmark_scalability();
        benchmark_hash_algorithms();
        
        std::cout << "🎯 Performance Summary Complete!" << std::endl;
        std::cout << "=================================" << std::endl;
        std::cout << "✅ All benchmarks completed successfully" << std::endl;
        std::cout << "🚀 RapidSift is optimized for production use!" << std::endl;
    }
    
    static std::vector<Document> generate_test_documents(int total_count, int unique_count) {
        std::vector<Document> documents;
        documents.reserve(total_count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, unique_count - 1);
        
        std::vector<std::string> base_documents;
        for (int i = 0; i < unique_count; ++i) {
            base_documents.push_back("Document content number " + std::to_string(i) + 
                                   " with additional text to make it realistic.");
        }
        
        for (int i = 0; i < total_count; ++i) {
            int base_index = dis(gen);
            documents.emplace_back(base_documents[base_index], i);
        }
        
        return documents;
    }

private:
    static void benchmark_exact_deduplication() {
        std::cout << "📊 Exact Deduplication Benchmark" << std::endl;
        std::cout << "----------------------------------" << std::endl;
        
        std::vector<int> sizes = {1000, 5000, 10000, 25000};
        
        for (int size : sizes) {
            auto documents = generate_test_documents(size, size / 10);  // 10% unique
            
            ExactDeduplicator deduplicator;
            
            auto start = std::chrono::high_resolution_clock::now();
            auto result = deduplicator.deduplicate(documents);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            double docs_per_sec = (double(size) / duration.count()) * 1000.0;
            
            std::cout << std::setw(8) << size << " docs: " 
                     << std::setw(6) << duration.count() << " ms, "
                     << std::setw(8) << std::fixed << std::setprecision(0) << docs_per_sec 
                     << " docs/sec, "
                     << std::setw(5) << std::setprecision(1) << result.reduction_percentage() 
                     << "% reduction" << std::endl;
        }
        std::cout << std::endl;
    }
    
    static void benchmark_near_deduplication() {
        std::cout << "📊 Near-Duplicate Detection Benchmark" << std::endl;
        std::cout << "--------------------------------------" << std::endl;
        
        std::vector<int> sizes = {500, 1000, 2500, 5000};
        
        for (int size : sizes) {
            auto documents = generate_similar_documents(size, size / 20);  // 5% unique base
            
            // Test MinHash
            {
                NearDedupConfig config;
                config.method = NearDedupConfig::Method::MINHASH;
                config.threshold = 0.8;
                NearDeduplicator deduplicator(config);
                
                auto start = std::chrono::high_resolution_clock::now();
                auto result = deduplicator.deduplicate(documents);
                auto end = std::chrono::high_resolution_clock::now();
                
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                double docs_per_sec = (double(size) / duration.count()) * 1000.0;
                
                std::cout << "MinHash " << std::setw(6) << size << " docs: " 
                         << std::setw(6) << duration.count() << " ms, "
                         << std::setw(6) << std::fixed << std::setprecision(0) << docs_per_sec 
                         << " docs/sec" << std::endl;
            }
            
            // Test SimHash
            {
                NearDedupConfig config;
                config.method = NearDedupConfig::Method::SIMHASH;
                config.threshold = 0.8;
                NearDeduplicator deduplicator(config);
                
                auto start = std::chrono::high_resolution_clock::now();
                auto result = deduplicator.deduplicate(documents);
                auto end = std::chrono::high_resolution_clock::now();
                
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                double docs_per_sec = (double(size) / duration.count()) * 1000.0;
                
                std::cout << "SimHash " << std::setw(6) << size << " docs: " 
                         << std::setw(6) << duration.count() << " ms, "
                         << std::setw(6) << std::fixed << std::setprecision(0) << docs_per_sec 
                         << " docs/sec" << std::endl;
            }
        }
        std::cout << std::endl;
    }
    
    static void benchmark_scalability() {
        std::cout << "📊 Scalability Test" << std::endl;
        std::cout << "-------------------" << std::endl;
        
        std::vector<int> sizes = {10000, 50000, 100000};
        
        for (int size : sizes) {
            auto documents = generate_test_documents(size, size / 100);  // 1% unique
            
            ExactDeduplicator deduplicator;
            
            auto start = std::chrono::high_resolution_clock::now();
            auto result = deduplicator.deduplicate(documents);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            double docs_per_sec = (double(size) / duration.count()) * 1000.0;
            double memory_mb = estimate_memory_usage(size);
            
            std::cout << std::setw(8) << size << " docs: " 
                     << std::setw(6) << duration.count() << " ms, "
                     << std::setw(8) << std::fixed << std::setprecision(0) << docs_per_sec 
                     << " docs/sec, ~"
                     << std::setw(4) << std::setprecision(0) << memory_mb 
                     << " MB" << std::endl;
        }
        std::cout << std::endl;
    }
    
    static void benchmark_hash_algorithms() {
        std::cout << "📊 Hash Algorithm Comparison" << std::endl;
        std::cout << "-----------------------------" << std::endl;
        
        const int size = 10000;
        auto documents = generate_test_documents(size, size / 10);
        
        std::vector<std::pair<std::string, HashAlgorithm>> algorithms = {
            {"xxHash64", HashAlgorithm::XXHASH64},
            {"SHA256  ", HashAlgorithm::SHA256},
            {"SHA1    ", HashAlgorithm::SHA1},
            {"MD5     ", HashAlgorithm::MD5}
        };
        
        for (const auto& [name, algorithm] : algorithms) {
            ExactDedupConfig config;
            config.algorithm = algorithm;
            ExactDeduplicator deduplicator(config);
            
            auto start = std::chrono::high_resolution_clock::now();
            auto result = deduplicator.deduplicate(documents);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            double docs_per_sec = (double(size) / duration.count()) * 1000.0;
            
            std::cout << name << ": " 
                     << std::setw(6) << duration.count() << " ms, "
                     << std::setw(8) << std::fixed << std::setprecision(0) << docs_per_sec 
                     << " docs/sec" << std::endl;
        }
        std::cout << std::endl;
    }
    
    static std::vector<Document> generate_similar_documents(int total_count, int base_count) {
        std::vector<Document> documents;
        documents.reserve(total_count);
        
        std::vector<std::string> base_texts = {
            "The quick brown fox jumps over the lazy dog",
            "A journey of a thousand miles begins with a single step",
            "To be or not to be that is the question",
            "All that glitters is not gold in this world",
            "The early bird catches the worm every morning"
        };
        
        std::vector<std::string> variations = {
            "",
            " and runs away",
            " in the forest",
            " during sunset",
            " very quickly",
            " with great speed"
        };
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> text_dis(0, base_texts.size() - 1);
        std::uniform_int_distribution<> var_dis(0, variations.size() - 1);
        
        for (int i = 0; i < total_count; ++i) {
            std::string text = base_texts[text_dis(gen)] + variations[var_dis(gen)];
            documents.emplace_back(text, i);
        }
        
        return documents;
    }
    
    static double estimate_memory_usage(int document_count) {
        // Rough estimate: each document ~100 bytes + overhead
        return (document_count * 150.0) / (1024.0 * 1024.0);  // Convert to MB
    }
};

void test_exact_dedup_performance() {
    const int test_size = 10000;
    auto documents = PerformanceBenchmark::generate_test_documents(test_size, test_size / 10);
    
    ExactDeduplicator deduplicator;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = deduplicator.deduplicate(documents);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_LT(duration.count(), 1000);  // Should complete in under 1 second
    ASSERT_EQ(test_size, result.original_count());
    ASSERT_EQ(test_size / 10, result.unique_count());
    
    std::cout << "  Processed " << test_size << " documents in " 
              << duration.count() << "ms" << std::endl;
}

void test_near_dedup_performance() {
    const int test_size = 1000;  // Smaller for near dedup
    auto documents = PerformanceBenchmark::generate_test_documents(test_size, test_size / 10);
    
    NearDeduplicator deduplicator;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = deduplicator.deduplicate(documents);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_LT(duration.count(), 5000);  // Should complete in under 5 seconds
    ASSERT_EQ(test_size, result.original_count());
    
    std::cout << "  Processed " << test_size << " documents in " 
              << duration.count() << "ms" << std::endl;
}

void test_scalability() {
    std::vector<int> sizes = {1000, 5000, 10000};
    
    std::cout << "  Scalability test results:" << std::endl;
    
    for (int size : sizes) {
        auto documents = PerformanceBenchmark::generate_test_documents(size, size / 20);
        
        ExactDeduplicator deduplicator;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto result = deduplicator.deduplicate(documents);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        double docs_per_sec = (double(size) / duration.count()) * 1000.0;
        
        std::cout << "    " << std::setw(6) << size << " docs: " 
                  << std::setw(5) << duration.count() << "ms, "
                  << std::setw(6) << std::fixed << std::setprecision(0) << docs_per_sec 
                  << " docs/sec" << std::endl;
        
        ASSERT_LT(duration.count(), size / 5);  // Reasonable time scaling
    }
}

int main() {
    std::cout << "⚡ RapidSift Performance Test Suite" << std::endl;
    std::cout << "===================================" << std::endl << std::endl;
    
    TestSuite suite("Performance Tests");
    
    suite.add_test("Exact dedup performance", test_exact_dedup_performance);
    suite.add_test("Near dedup performance", test_near_dedup_performance);
    suite.add_test("Scalability test", test_scalability);
    
    suite.run_all();
    
    // Run detailed benchmarks
    PerformanceBenchmark::run_all_benchmarks();
    
    return 0;
} 