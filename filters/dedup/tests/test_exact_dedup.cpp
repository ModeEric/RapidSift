#include "test_framework.hpp"
#include "../include/rapidsift/exact_dedup.hpp"
#include "../include/rapidsift/common.hpp"
#include <vector>
#include <string>
#include <unordered_set>
#include <fstream>
#include <sstream>

using namespace rapidsift;
using namespace test_framework;

class ExactDedupTests {
public:
    static void run_all_tests() {
        TestSuite suite("Exact Deduplication");
        
        // Basic functionality tests
        suite.add_test("Empty input", test_empty_input);
        suite.add_test("Single document", test_single_document);
        suite.add_test("No duplicates", test_no_duplicates);
        suite.add_test("Simple duplicates", test_simple_duplicates);
        suite.add_test("Multiple duplicate groups", test_multiple_groups);
        
        // Hash algorithm tests
        suite.add_test("xxHash algorithm", test_xxhash_algorithm);
        suite.add_test("MD5 algorithm", test_md5_algorithm);
        suite.add_test("SHA1 algorithm", test_sha1_algorithm);
        suite.add_test("SHA256 algorithm", test_sha256_algorithm);
        
        // Configuration tests
        suite.add_test("Keep first policy", test_keep_first_policy);
        suite.add_test("Keep last policy", test_keep_last_policy);
        suite.add_test("Parallel processing", test_parallel_processing);
        
        // Edge cases
        suite.add_test("Empty strings", test_empty_strings);
        suite.add_test("Whitespace differences", test_whitespace_differences);
        suite.add_test("Unicode text", test_unicode_text);
        suite.add_test("Very long documents", test_long_documents);
        
        // Performance tests
        suite.add_test("Large dataset", test_large_dataset);
        suite.add_test("Hash collision simulation", test_hash_collisions);
        
        // I/O and streaming tests
        suite.add_test("Find duplicate groups", test_find_duplicate_groups);
        suite.add_test("Statistics accuracy", test_statistics);
        
        suite.run_all();
    }

private:
    // Basic functionality tests
    static void test_empty_input() {
        ExactDeduplicator deduplicator;
        std::vector<Document> empty_docs;
        
        auto result = deduplicator.deduplicate(empty_docs);
        
        ASSERT_EQ(0, result.original_count());
        ASSERT_EQ(0, result.unique_count());
        ASSERT_EQ(0, result.duplicates_removed());
        ASSERT_TRUE(result.unique_documents().empty());
    }
    
    static void test_single_document() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("Single document", 0)
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(1, result.original_count());
        ASSERT_EQ(1, result.unique_count());
        ASSERT_EQ(0, result.duplicates_removed());
        ASSERT_STREQ("Single document", result.unique_documents()[0].text());
    }
    
    static void test_no_duplicates() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("First document", 0),
            Document("Second document", 1),
            Document("Third document", 2)
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(3, result.original_count());
        ASSERT_EQ(3, result.unique_count());
        ASSERT_EQ(0, result.duplicates_removed());
        ASSERT_EQ(0.0, result.reduction_percentage());
    }
    
    static void test_simple_duplicates() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("Duplicate text", 0),
            Document("Unique text", 1),
            Document("Duplicate text", 2)  // Exact duplicate
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(3, result.original_count());
        ASSERT_EQ(2, result.unique_count());
        ASSERT_EQ(1, result.duplicates_removed());
    }
    
    static void test_multiple_groups() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("Group A text", 0),
            Document("Group B text", 1),
            Document("Group A text", 2),  // Duplicate of group A
            Document("Group B text", 3),  // Duplicate of group B
            Document("Unique text", 4),
            Document("Group A text", 5)   // Another duplicate of group A
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(6, result.original_count());
        ASSERT_EQ(3, result.unique_count());
        ASSERT_EQ(3, result.duplicates_removed());
        ASSERT_EQ(2, result.duplicate_groups().size());  // Two groups with duplicates
    }
    
    // Hash algorithm tests
    static void test_xxhash_algorithm() {
        ExactDedupConfig config;
        config.algorithm = HashAlgorithm::XXHASH64;
        ExactDeduplicator deduplicator(config);
        
        std::vector<Document> docs = {
            Document("Test document", 0),
            Document("Test document", 1)  // Duplicate
        };
        
        auto result = deduplicator.deduplicate(docs);
        ASSERT_EQ(1, result.unique_count());
    }
    
    static void test_md5_algorithm() {
        ExactDedupConfig config;
        config.algorithm = HashAlgorithm::MD5;
        ExactDeduplicator deduplicator(config);
        
        std::vector<Document> docs = {
            Document("Test document", 0),
            Document("Test document", 1)  // Duplicate
        };
        
        auto result = deduplicator.deduplicate(docs);
        ASSERT_EQ(1, result.unique_count());
    }
    
    static void test_sha1_algorithm() {
        ExactDedupConfig config;
        config.algorithm = HashAlgorithm::SHA1;
        ExactDeduplicator deduplicator(config);
        
        std::vector<Document> docs = {
            Document("Test document", 0),
            Document("Test document", 1)  // Duplicate
        };
        
        auto result = deduplicator.deduplicate(docs);
        ASSERT_EQ(1, result.unique_count());
    }
    
    static void test_sha256_algorithm() {
        ExactDedupConfig config;
        config.algorithm = HashAlgorithm::SHA256;
        ExactDeduplicator deduplicator(config);
        
        std::vector<Document> docs = {
            Document("Test document", 0),
            Document("Test document", 1)  // Duplicate
        };
        
        auto result = deduplicator.deduplicate(docs);
        ASSERT_EQ(1, result.unique_count());
    }
    
    // Configuration tests
    static void test_keep_first_policy() {
        ExactDedupConfig config;
        config.keep_first = true;
        ExactDeduplicator deduplicator(config);
        
        std::vector<Document> docs = {
            Document("First occurrence", 0),
            Document("Different text", 1),
            Document("First occurrence", 2)  // Duplicate
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(2, result.unique_count());
        ASSERT_EQ(0, result.original_indices()[0]);  // Should keep first occurrence
    }
    
    static void test_keep_last_policy() {
        ExactDedupConfig config;
        config.keep_first = false;  // Keep last
        ExactDeduplicator deduplicator(config);
        
        std::vector<Document> docs = {
            Document("Last occurrence", 0),
            Document("Different text", 1),
            Document("Last occurrence", 2)  // Duplicate - should be kept
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(2, result.unique_count());
        // Should contain document from original index 2 (last occurrence)
        std::unordered_set<DocumentId> kept_indices(result.original_indices().begin(), 
                                                    result.original_indices().end());
        ASSERT_TRUE(kept_indices.count(2) > 0);  // Should keep last occurrence
        ASSERT_TRUE(kept_indices.count(1) > 0);  // Should keep unique document
    }
    
    static void test_parallel_processing() {
        ExactDedupConfig config;
        config.parallel = true;
        ExactDeduplicator deduplicator(config);
        
        // Create a moderately sized dataset to test parallel processing
        std::vector<Document> docs;
        for (int i = 0; i < 1000; ++i) {
            docs.emplace_back("Document " + std::to_string(i % 100), i);  // 10x duplicates
        }
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(1000, result.original_count());
        ASSERT_EQ(100, result.unique_count());  // Should have 100 unique documents
        ASSERT_EQ(900, result.duplicates_removed());
    }
    
    // Edge case tests
    static void test_empty_strings() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("", 0),
            Document("", 1),  // Duplicate empty string
            Document("Non-empty", 2),
            Document("", 3)   // Another duplicate empty string
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(4, result.original_count());
        ASSERT_EQ(2, result.unique_count());  // Empty string and "Non-empty"
        ASSERT_EQ(2, result.duplicates_removed());
    }
    
    static void test_whitespace_differences() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("Text with spaces", 0),
            Document("Text with  spaces", 1),    // Different spacing - not duplicate
            Document("Text with spaces", 2),     // Exact duplicate
            Document("Text with spaces ", 3),    // Trailing space - not duplicate
            Document(" Text with spaces", 4)     // Leading space - not duplicate
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(5, result.original_count());
        ASSERT_EQ(4, result.unique_count());  // Only one exact duplicate
        ASSERT_EQ(1, result.duplicates_removed());
    }
    
    static void test_unicode_text() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("Hello 世界", 0),
            Document("Здравствуй мир", 1),
            Document("Hello 世界", 2)  // Duplicate Unicode
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(3, result.original_count());
        ASSERT_EQ(2, result.unique_count());
        ASSERT_EQ(1, result.duplicates_removed());
    }
    
    static void test_long_documents() {
        ExactDeduplicator deduplicator;
        
        // Create very long documents (10KB each)
        std::string long_text(10000, 'A');
        std::string long_text_2(10000, 'B');
        
        std::vector<Document> docs = {
            Document(long_text, 0),
            Document(long_text_2, 1),
            Document(long_text, 2),  // Duplicate long document
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(3, result.original_count());
        ASSERT_EQ(2, result.unique_count());
        ASSERT_EQ(1, result.duplicates_removed());
    }
    
    // Performance tests
    static void test_large_dataset() {
        ExactDeduplicator deduplicator;
        
        // Create large dataset with known duplicate pattern
        const int total_docs = 10000;
        const int unique_docs = 1000;
        
        std::vector<Document> docs;
        for (int i = 0; i < total_docs; ++i) {
            docs.emplace_back("Document " + std::to_string(i % unique_docs), i);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        auto result = deduplicator.deduplicate(docs);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        ASSERT_EQ(total_docs, result.original_count());
        ASSERT_EQ(unique_docs, result.unique_count());
        ASSERT_EQ(total_docs - unique_docs, result.duplicates_removed());
        
        // Performance assertion - should complete in reasonable time
        ASSERT_LT(duration_ms.count(), 1000);  // Less than 1 second
    }
    
    static void test_hash_collisions() {
        // Test with documents that might cause hash collisions
        ExactDeduplicator deduplicator;
        
        std::vector<Document> docs = {
            Document("aa", 0),
            Document("bb", 1),
            Document("aa", 2),  // True duplicate
            Document("aaa", 3), // Different length
            Document("AB", 4),  // Different case
            Document("ba", 5),  // Different order
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        ASSERT_EQ(6, result.original_count());
        ASSERT_EQ(5, result.unique_count());  // Only "aa" should be deduplicated
        ASSERT_EQ(1, result.duplicates_removed());
    }
    
    // Analysis tests
    static void test_find_duplicate_groups() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("Group A", 0),
            Document("Group B", 1),
            Document("Group A", 2),
            Document("Unique", 3),
            Document("Group B", 4),
            Document("Group A", 5)
        };
        
        auto groups = deduplicator.find_duplicate_groups(docs);
        
        ASSERT_EQ(2, groups.size());  // Two groups with duplicates
        
        // Check that groups have correct sizes
        for (const auto& [hash, group] : groups) {
            if (group.size() == 3) {
                // Group A should have 3 members (indices 0, 2, 5)
                ASSERT_TRUE(std::find(group.begin(), group.end(), 0) != group.end());
                ASSERT_TRUE(std::find(group.begin(), group.end(), 2) != group.end());
                ASSERT_TRUE(std::find(group.begin(), group.end(), 5) != group.end());
            } else if (group.size() == 2) {
                // Group B should have 2 members (indices 1, 4)
                ASSERT_TRUE(std::find(group.begin(), group.end(), 1) != group.end());
                ASSERT_TRUE(std::find(group.begin(), group.end(), 4) != group.end());
            } else {
                throw std::runtime_error("Unexpected group size: " + std::to_string(group.size()));
            }
        }
    }
    
    static void test_statistics() {
        ExactDeduplicator deduplicator;
        std::vector<Document> docs = {
            Document("A", 0), Document("B", 1), Document("A", 2),
            Document("C", 3), Document("B", 4), Document("A", 5)
        };
        
        auto result = deduplicator.deduplicate(docs);
        
        // Verify statistics
        ASSERT_EQ(6, result.original_count());
        ASSERT_EQ(3, result.unique_count());
        ASSERT_EQ(3, result.duplicates_removed());
        ASSERT_NEAR(50.0, result.reduction_percentage(), 0.1);
        ASSERT_GT(result.processing_time().count(), 0);  // Should have some processing time
        
        // Check duplicate groups
        ASSERT_EQ(2, result.duplicate_groups().size());
        
        // Verify group sizes
        auto groups = result.duplicate_groups();
        std::vector<size_t> group_sizes;
        for (const auto& group : groups) {
            group_sizes.push_back(group.size());
        }
        std::sort(group_sizes.begin(), group_sizes.end());
        
        ASSERT_EQ(2, group_sizes[0]);  // Group B has 2 duplicates
        ASSERT_EQ(3, group_sizes[1]);  // Group A has 3 duplicates
    }
};

int main() {
    std::cout << "🚀 RapidSift Exact Deduplication Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl << std::endl;
    
    ExactDedupTests::run_all_tests();
    
    return 0;
} 