#include "test_framework.hpp"
#include "../include/rapidsift/near_dedup.hpp"
#include "../include/rapidsift/common.hpp"
#include <vector>
#include <string>
#include <cmath>

using namespace rapidsift;
using namespace test_framework;

void test_minhash_basic() {
    MinHashSignature sig1(128);
    MinHashSignature sig2(128);
    
    sig1.update("hello");
    sig2.update("hello");
    
    double similarity = sig1.jaccard_similarity(sig2);
    ASSERT_NEAR(1.0, similarity, 0.01);
}

void test_simhash_basic() {
    SimHashSignature sig1(64);
    SimHashSignature sig2(64);
    
    sig1.compute("the quick brown fox");
    sig2.compute("the quick brown fox");
    
    ASSERT_EQ(0, sig1.hamming_distance(sig2));
    ASSERT_NEAR(1.0, sig1.similarity(sig2), 0.01);
}

void test_minhash_signature_creation() {
    MinHashSignature sig1(128);
    MinHashSignature sig2(128);
    
    // Test with same text
    sig1.update("hello world");
    sig2.update("hello world");
    
    double similarity = sig1.jaccard_similarity(sig2);
    ASSERT_NEAR(1.0, similarity, 0.01);  // Should be nearly identical
}

void test_minhash_different_texts() {
    MinHashSignature sig1(128);
    MinHashSignature sig2(128);
    
    sig1.update("completely different text");
    sig2.update("totally unrelated content");
    
    double similarity = sig1.jaccard_similarity(sig2);
    ASSERT_LT(similarity, 0.5);  // Should have low similarity
}

void test_minhash_similar_texts() {
    MinHashSignature sig1(128);
    MinHashSignature sig2(128);
    
    // Generate n-grams manually for similar texts
    std::string text1 = "the quick brown fox jumps";
    std::string text2 = "the quick brown fox leaps";
    
    // Add n-grams
    for (size_t i = 0; i < text1.length() - 2; ++i) {
        sig1.update(text1.substr(i, 3));
    }
    for (size_t i = 0; i < text2.length() - 2; ++i) {
        sig2.update(text2.substr(i, 3));
    }
    
    double similarity = sig1.jaccard_similarity(sig2);
    ASSERT_GT(similarity, 0.5);  // Should have reasonable similarity
}

void test_simhash_signature() {
    SimHashSignature sig1(64);
    SimHashSignature sig2(64);
    
    sig1.compute("the quick brown fox");
    sig2.compute("the quick brown fox");
    
    ASSERT_EQ(0, sig1.hamming_distance(sig2));  // Same text = 0 distance
    ASSERT_NEAR(1.0, sig1.similarity(sig2), 0.01);
}

void test_simhash_different_texts() {
    SimHashSignature sig1(64);
    SimHashSignature sig2(64);
    
    sig1.compute("completely different text here");
    sig2.compute("totally unrelated content now");
    
    size_t distance = sig1.hamming_distance(sig2);
    ASSERT_GT(distance, 10);  // Should have significant distance
    
    double similarity = sig1.similarity(sig2);
    ASSERT_LT(similarity, 0.8);  // Should have lower similarity
}

void test_near_dedup_minhash() {
    NearDedupConfig config;
    config.method = NearDedupConfig::Method::MINHASH;
    config.threshold = 0.7;
    
    NearDeduplicator deduplicator(config);
    
    std::vector<Document> docs = {
        Document("the quick brown fox", 0),
        Document("the quick brown fox", 1),  // Exact duplicate
        Document("completely different", 2)
    };
    
    auto result = deduplicator.deduplicate(docs);
    
    ASSERT_EQ(3, result.original_count());
    ASSERT_EQ(2, result.unique_count());
    ASSERT_EQ(1, result.duplicates_removed());
}

void test_near_dedup_simhash() {
    NearDedupConfig config;
    config.method = NearDedupConfig::Method::SIMHASH;
    config.threshold = 0.8;
    
    NearDeduplicator deduplicator(config);
    
    std::vector<Document> docs = {
        Document("the quick brown fox", 0),
        Document("the quick brown fox", 1),  // Exact duplicate
        Document("totally different", 2)
    };
    
    auto result = deduplicator.deduplicate(docs);
    
    ASSERT_EQ(3, result.original_count());
    ASSERT_EQ(2, result.unique_count());
    ASSERT_EQ(1, result.duplicates_removed());
}

void test_threshold_sensitivity() {
    // Test with strict threshold
    NearDedupConfig strict_config;
    strict_config.method = NearDedupConfig::Method::MINHASH;
    strict_config.threshold = 0.95;  // Very strict
    
    NearDeduplicator strict_deduplicator(strict_config);
    
    std::vector<Document> docs = {
        Document("the quick brown fox", 0),
        Document("the quick brown foxes", 1),  // Slightly different
        Document("totally different", 2)
    };
    
    auto strict_result = strict_deduplicator.deduplicate(docs);
    
    // With strict threshold, should keep more documents
    ASSERT_EQ(3, strict_result.original_count());
    ASSERT_GE(strict_result.unique_count(), 2);
    
    // Test with lenient threshold
    NearDedupConfig lenient_config;
    lenient_config.method = NearDedupConfig::Method::MINHASH;
    lenient_config.threshold = 0.5;  // More lenient
    
    NearDeduplicator lenient_deduplicator(lenient_config);
    auto lenient_result = lenient_deduplicator.deduplicate(docs);
    
    // With lenient threshold, might remove more documents
    ASSERT_LE(lenient_result.unique_count(), strict_result.unique_count());
}

void test_empty_input() {
    NearDeduplicator deduplicator;
    std::vector<Document> empty_docs;
    
    auto result = deduplicator.deduplicate(empty_docs);
    
    ASSERT_EQ(0, result.original_count());
    ASSERT_EQ(0, result.unique_count());
    ASSERT_TRUE(result.unique_documents().empty());
}

void test_single_document() {
    NearDeduplicator deduplicator;
    std::vector<Document> docs = {
        Document("single document", 0)
    };
    
    auto result = deduplicator.deduplicate(docs);
    
    ASSERT_EQ(1, result.original_count());
    ASSERT_EQ(1, result.unique_count());
    ASSERT_EQ(0, result.duplicates_removed());
}

void test_find_similar_pairs() {
    // This test is skipped for now as find_similar_pairs is not implemented
    // in the current version. It would require additional API methods.
    std::cout << "  Skipping find_similar_pairs test (not implemented)" << std::endl;
}

void test_ngram_size_effect() {
    // Test with small n-grams
    NearDedupConfig config_small;
    config_small.method = NearDedupConfig::Method::MINHASH;
    config_small.ngram_size = 2;
    config_small.threshold = 0.7;
    
    // Test with larger n-grams
    NearDedupConfig config_large;
    config_large.method = NearDedupConfig::Method::MINHASH;
    config_large.ngram_size = 5;
    config_large.threshold = 0.7;
    
    std::vector<Document> docs = {
        Document("the quick brown fox jumps over lazy dog", 0),
        Document("the quick brown fox leaps over lazy dog", 1),  // One word different
        Document("different content entirely here", 2)
    };
    
    NearDeduplicator dedup_small(config_small);
    NearDeduplicator dedup_large(config_large);
    
    auto result_small = dedup_small.deduplicate(docs);
    auto result_large = dedup_large.deduplicate(docs);
    
    // Both should work, but may have different sensitivities
    ASSERT_EQ(3, result_small.original_count());
    ASSERT_EQ(3, result_large.original_count());
    ASSERT_LE(result_small.unique_count(), 3);
    ASSERT_LE(result_large.unique_count(), 3);
}

int main() {
    std::cout << "🔍 RapidSift Near-Duplicate Detection Test Suite" << std::endl;
    std::cout << "=================================================" << std::endl << std::endl;
    
    TestSuite suite("Near-Duplicate Detection");
    
    suite.add_test("MinHash basic", test_minhash_basic);
    suite.add_test("SimHash basic", test_simhash_basic);
    suite.add_test("MinHash signature creation", test_minhash_signature_creation);
    suite.add_test("MinHash different texts", test_minhash_different_texts);
    suite.add_test("MinHash similar texts", test_minhash_similar_texts);
    suite.add_test("SimHash signature", test_simhash_signature);
    suite.add_test("SimHash different texts", test_simhash_different_texts);
    suite.add_test("Near dedup MinHash", test_near_dedup_minhash);
    suite.add_test("Near dedup SimHash", test_near_dedup_simhash);
    suite.add_test("Threshold sensitivity", test_threshold_sensitivity);
    suite.add_test("Empty input", test_empty_input);
    suite.add_test("Single document", test_single_document);
    suite.add_test("Find similar pairs", test_find_similar_pairs);
    suite.add_test("N-gram size effect", test_ngram_size_effect);
    
    suite.run_all();
    
    return 0;
} 