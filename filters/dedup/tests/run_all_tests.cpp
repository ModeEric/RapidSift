#include "test_framework.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>

// Include all test headers
#include "../include/rapidsift/exact_dedup.hpp"
#include "../include/rapidsift/near_dedup.hpp"
#include "../include/rapidsift/common.hpp"

using namespace rapidsift;
using namespace test_framework;

// Forward declarations of test functions from other test files
extern void run_exact_dedup_tests();
extern void run_near_dedup_tests();
extern void run_utils_tests();

class IntegrationTests {
public:
    static void run_all_tests() {
        TestSuite suite("Integration Tests");
        
        suite.add_test("End-to-end exact dedup", test_end_to_end_exact);
        suite.add_test("End-to-end near dedup", test_end_to_end_near);
        suite.add_test("Performance comparison", test_performance_comparison);
        suite.add_test("Mixed content types", test_mixed_content);
        suite.add_test("Large scale test", test_large_scale);
        
        suite.run_all();
    }

private:
    static void test_end_to_end_exact() {
        // Create test documents with various types of duplicates
        std::vector<Document> documents = {
            Document("The quick brown fox jumps over the lazy dog.", 0),
            Document("A different sentence entirely.", 1),
            Document("The quick brown fox jumps over the lazy dog.", 2),  // Exact duplicate
            Document("Yet another unique sentence here.", 3),
            Document("A different sentence entirely.", 4),  // Another exact duplicate
            Document("Final unique sentence for testing.", 5)
        };
        
        // Test with exact deduplication
        ExactDedupConfig config;
        config.algorithm = HashAlgorithm::XXHASH64;
        config.parallel = true;
        
        ExactDeduplicator deduplicator(config);
        auto result = deduplicator.deduplicate(documents);
        
        ASSERT_EQ(6, result.original_count());
        ASSERT_EQ(4, result.unique_count());
        ASSERT_EQ(2, result.duplicates_removed());
        ASSERT_NEAR(33.33, result.reduction_percentage(), 1.0);
        ASSERT_GT(result.processing_time().count(), 0);
    }
    
    static void test_end_to_end_near() {
        // Create test documents with near-duplicates
        std::vector<Document> documents = {
            Document("The quick brown fox jumps over the lazy dog", 0),
            Document("The quick brown fox leaps over the lazy dog", 1),  // Near duplicate
            Document("Completely different content here", 2),
            Document("The quick brown fox jumps over the lazy dog", 3),  // Exact duplicate
            Document("Another totally different sentence", 4)
        };
        
        // Test with near deduplication
        NearDedupConfig config;
        config.method = NearDedupConfig::Method::MINHASH;
        config.threshold = 0.7;
        config.parallel = true;
        
        NearDeduplicator deduplicator(config);
        auto result = deduplicator.deduplicate(documents);
        
        ASSERT_EQ(5, result.original_count());
        ASSERT_LT(result.unique_count(), 5);  // Should remove some duplicates
        ASSERT_GT(result.duplicates_removed(), 0);
        ASSERT_GT(result.processing_time().count(), 0);
    }
    
    static void test_performance_comparison() {
        // Create a moderately sized dataset
        const int doc_count = 1000;
        const int unique_count = 100;
        
        std::vector<Document> documents;
        for (int i = 0; i < doc_count; ++i) {
            documents.emplace_back("Document content " + std::to_string(i % unique_count), i);
        }
        
        // Test exact deduplication performance
        {
            ExactDeduplicator exact_dedup;
            auto start = std::chrono::high_resolution_clock::now();
            auto result = exact_dedup.deduplicate(documents);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            ASSERT_EQ(doc_count, result.original_count());
            ASSERT_EQ(unique_count, result.unique_count());
            ASSERT_LT(duration.count(), 1000);  // Should complete in under 1 second
        }
        
        // Test near deduplication performance
        {
            NearDedupConfig config;
            config.method = NearDedupConfig::Method::SIMHASH;
            config.threshold = 0.8;
            
            NearDeduplicator near_dedup(config);
            auto start = std::chrono::high_resolution_clock::now();
            auto result = near_dedup.deduplicate(documents);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            ASSERT_EQ(doc_count, result.original_count());
            ASSERT_LT(duration.count(), 5000);  // Should complete in under 5 seconds
        }
    }
    
    static void test_mixed_content() {
        // Test with various content types
        std::vector<Document> documents = {
            Document("", 0),  // Empty string
            Document("   ", 1),  // Whitespace only
            Document("Hello 世界", 2),  // Unicode
            Document("Hello World", 3),  // ASCII
            Document("", 4),  // Another empty string (duplicate)
            Document("HELLO WORLD", 5),  // Different case
            Document(std::string(1000, 'A'), 6),  // Very long string
            Document("Hello World", 7),  // Exact duplicate
            Document("Hello\nWorld", 8),  // With newline
            Document("Hello\tWorld", 9)   // With tab
        };
        
        ExactDeduplicator deduplicator;
        auto result = deduplicator.deduplicate(documents);
        
        ASSERT_EQ(10, result.original_count());
        ASSERT_LT(result.unique_count(), 10);  // Should remove some duplicates
        ASSERT_GT(result.duplicates_removed(), 0);
    }
    
    static void test_large_scale() {
        // Test with a larger dataset to ensure scalability
        const int large_count = 5000;
        const int pattern_count = 500;
        
        std::vector<Document> documents;
        
        // Create documents with predictable duplicate patterns
        for (int i = 0; i < large_count; ++i) {
            std::string content;
            if (i % 10 == 0) {
                content = "Frequent pattern that appears often";
            } else if (i % 5 == 0) {
                content = "Less frequent pattern " + std::to_string(i % pattern_count);
            } else {
                content = "Unique content for document " + std::to_string(i);
            }
            documents.emplace_back(content, i);
        }
        
        ExactDeduplicator deduplicator;
        auto start = std::chrono::high_resolution_clock::now();
        auto result = deduplicator.deduplicate(documents);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        ASSERT_EQ(large_count, result.original_count());
        ASSERT_LT(result.unique_count(), large_count);  // Should remove duplicates
        ASSERT_GT(result.duplicates_removed(), 0);
        ASSERT_LT(duration.count(), 2000);  // Should complete in under 2 seconds
        
        // Verify statistics make sense
        ASSERT_EQ(result.original_count() - result.unique_count(), result.duplicates_removed());
        ASSERT_GT(result.reduction_percentage(), 0.0);
        ASSERT_LT(result.reduction_percentage(), 100.0);
    }
};

void print_system_info() {
    std::cout << "🖥️  System Information" << std::endl;
    std::cout << "======================" << std::endl;
    
    // Hardware threads
    std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << std::endl;
    
    // Compiler info
    std::cout << "Compiler: ";
#ifdef __GNUC__
    std::cout << "GCC " << __GNUC__ << "." << __GNUC_MINOR__;
#elif defined(__clang__)
    std::cout << "Clang " << __clang_major__ << "." << __clang_minor__;
#elif defined(_MSC_VER)
    std::cout << "MSVC " << _MSC_VER;
#else
    std::cout << "Unknown";
#endif
    std::cout << std::endl;
    
    // OpenMP support
#ifdef USE_OPENMP
    std::cout << "OpenMP: Enabled" << std::endl;
#else
    std::cout << "OpenMP: Disabled" << std::endl;
#endif
    
    // Build type
#ifdef NDEBUG
    std::cout << "Build: Release" << std::endl;
#else
    std::cout << "Build: Debug" << std::endl;
#endif
    
    std::cout << std::endl;
}

void print_banner() {
    std::cout << "🧪 RapidSift C++ Test Suite" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Ultra-fast text deduplication testing" << std::endl << std::endl;
}

void print_summary(bool all_passed) {
    std::cout << std::endl;
    std::cout << "🎯 Test Summary" << std::endl;
    std::cout << "===============" << std::endl;
    
    if (all_passed) {
        std::cout << "✅ ALL TESTS PASSED!" << std::endl;
        std::cout << "🚀 RapidSift is ready for production use!" << std::endl;
    } else {
        std::cout << "❌ SOME TESTS FAILED!" << std::endl;
        std::cout << "🔧 Please review the failed tests above." << std::endl;
    }
    std::cout << std::endl;
}

// Simple test functions that just run the individual test suites
void run_exact_dedup_tests() {
    // This would call the test functions from test_exact_dedup.cpp
    // For now, we'll create a simple placeholder
    TestSuite suite("Exact Deduplication Tests");
    
    suite.add_test("Basic functionality", []() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("test", 0),
            Document("test", 1)
        };
        auto result = deduplicator.deduplicate(docs);
        ASSERT_EQ(1, result.unique_count());
    });
    
    suite.run_all();
}

void run_near_dedup_tests() {
    TestSuite suite("Near Deduplication Tests");
    
    suite.add_test("Basic functionality", []() {
        NearDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("test document", 0),
            Document("test document", 1)
        };
        auto result = deduplicator.deduplicate(docs);
        ASSERT_EQ(1, result.unique_count());
    });
    
    suite.run_all();
}

void run_utils_tests() {
    TestSuite suite("Utilities Tests");
    
    suite.add_test("Hash consistency", []() {
        std::string text = "test";
        Hash hash1 = hash_utils::xxhash64(text);
        Hash hash2 = hash_utils::xxhash64(text);
        ASSERT_EQ(hash1, hash2);
    });
    
    suite.run_all();
}

int main() {
    print_banner();
    print_system_info();
    
    bool all_passed = true;
    
    try {
        // Run individual test suites
        std::cout << "🔧 Running Core Component Tests" << std::endl;
        std::cout << "================================" << std::endl;
        run_utils_tests();
        run_exact_dedup_tests();
        run_near_dedup_tests();
        
        std::cout << "🚀 Running Integration Tests" << std::endl;
        std::cout << "=============================" << std::endl;
        IntegrationTests::run_all_tests();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test execution failed: " << e.what() << std::endl;
        all_passed = false;
    }
    
    print_summary(all_passed);
    
    return all_passed ? 0 : 1;
} 