#include "test_framework.hpp"
#include "../include/rapidsift/common.hpp"
#include <fstream>
#include <sstream>

using namespace rapidsift;
using namespace test_framework;

void test_text_normalization() {
    std::string input = "  Hello   World  \n\t ";
    std::string normalized = text_utils::normalize_text(input);
    
    ASSERT_STREQ("hello world", normalized);
}

void test_text_tokenization() {
    std::string input = "Hello, world! How are you?";
    auto tokens = text_utils::tokenize(input);
    
    ASSERT_EQ(4, tokens.size());
    ASSERT_STREQ("hello", tokens[0]);
    ASSERT_STREQ("world", tokens[1]);
    ASSERT_STREQ("how", tokens[2]);
    ASSERT_STREQ("are", tokens[3]);
}

void test_ngram_generation() {
    std::string input = "hello";
    auto ngrams = text_utils::generate_ngrams(input, 3);
    
    ASSERT_EQ(3, ngrams.size());
    ASSERT_STREQ("hel", ngrams[0]);
    ASSERT_STREQ("ell", ngrams[1]);
    ASSERT_STREQ("llo", ngrams[2]);
}

void test_to_lowercase() {
    std::string input = "HeLLo WoRLd";
    std::string result = text_utils::to_lowercase(input);
    
    ASSERT_STREQ("hello world", result);
}

void test_remove_whitespace() {
    std::string input = "hello world\t\n";
    std::string result = text_utils::remove_whitespace(input);
    
    ASSERT_STREQ("helloworld", result);
}

void test_hash_functions() {
    std::string test_text = "Hello, World!";
    
    // Test that different algorithms produce different hashes
    Hash xxhash_result = hash_utils::xxhash64(test_text);
    Hash md5_result = hash_utils::md5_hash(test_text);
    Hash sha1_result = hash_utils::sha1_hash(test_text);
    Hash sha256_result = hash_utils::sha256_hash(test_text);
    
    // All should be non-zero
    ASSERT_NE(0, xxhash_result);
    ASSERT_NE(0, md5_result);
    ASSERT_NE(0, sha1_result);
    ASSERT_NE(0, sha256_result);
    
    // All should be different
    ASSERT_NE(xxhash_result, md5_result);
    ASSERT_NE(xxhash_result, sha1_result);
    ASSERT_NE(xxhash_result, sha256_result);
}

void test_hash_consistency() {
    std::string test_text = "Consistent hash test";
    
    // Same input should produce same hash
    Hash hash1 = hash_utils::xxhash64(test_text);
    Hash hash2 = hash_utils::xxhash64(test_text);
    
    ASSERT_EQ(hash1, hash2);
}

void test_document_creation() {
    Document doc1;
    ASSERT_TRUE(doc1.empty());
    ASSERT_EQ(0, doc1.size());
    ASSERT_EQ(0, doc1.id());
    
    Document doc2("Hello World", 42);
    ASSERT_FALSE(doc2.empty());
    ASSERT_EQ(11, doc2.size());
    ASSERT_EQ(42, doc2.id());
    ASSERT_STREQ("Hello World", doc2.text());
}

void test_deduplication_result() {
    DeduplicationResult result;
    
    // Test initial state
    ASSERT_EQ(0, result.original_count());
    ASSERT_EQ(0, result.unique_count());
    ASSERT_EQ(0, result.duplicates_removed());
    ASSERT_EQ(0.0, result.reduction_percentage());
    
    // Add some documents
    result.set_original_count(10);
    result.add_unique_document(Document("Doc 1", 0), 0);
    result.add_unique_document(Document("Doc 2", 1), 2);
    
    ASSERT_EQ(10, result.original_count());
    ASSERT_EQ(2, result.unique_count());
    ASSERT_EQ(8, result.duplicates_removed());
    ASSERT_NEAR(80.0, result.reduction_percentage(), 0.1);
}

void test_timer() {
    Timer timer;
    
    // Sleep for a small amount
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto elapsed = timer.elapsed();
    ASSERT_GT(elapsed.count(), 5);  // At least 5ms
    ASSERT_LT(elapsed.count(), 100); // Less than 100ms
    
    double seconds = timer.elapsed_seconds();
    ASSERT_GT(seconds, 0.005);  // At least 5ms in seconds
}

void test_file_io_txt() {
    // Create a temporary test file
    std::string test_filename = "test_documents.txt";
    std::ofstream outfile(test_filename);
    outfile << "First document\n";
    outfile << "Second document\n";
    outfile << "Third document\n";
    outfile.close();
    
    // Test loading
    auto documents = io_utils::load_documents_from_txt(test_filename);
    
    ASSERT_EQ(3, documents.size());
    ASSERT_STREQ("First document", documents[0].text());
    ASSERT_STREQ("Second document", documents[1].text());
    ASSERT_STREQ("Third document", documents[2].text());
    
    // Test saving
    std::string output_filename = "test_output.txt";
    io_utils::save_documents_to_file(documents, output_filename);
    
    // Verify saved file
    auto reloaded = io_utils::load_documents_from_txt(output_filename);
    ASSERT_EQ(3, reloaded.size());
    ASSERT_STREQ("First document", reloaded[0].text());
    
    // Clean up
    std::remove(test_filename.c_str());
    std::remove(output_filename.c_str());
}

void test_file_io_csv() {
    // Create a temporary CSV test file
    std::string test_filename = "test_documents.csv";
    std::ofstream outfile(test_filename);
    outfile << "text,category\n";
    outfile << "\"First document\",A\n";
    outfile << "\"Second document\",B\n";
    outfile << "\"Third document\",A\n";
    outfile.close();
    
    // Test loading
    auto documents = io_utils::load_documents_from_csv(test_filename, "text");
    
    ASSERT_EQ(3, documents.size());
    ASSERT_STREQ("First document", documents[0].text());
    ASSERT_STREQ("Second document", documents[1].text());
    ASSERT_STREQ("Third document", documents[2].text());
    
    // Clean up
    std::remove(test_filename.c_str());
}

void test_edge_cases() {
    // Test empty string normalization
    std::string empty_normalized = text_utils::normalize_text("");
    ASSERT_STREQ("", empty_normalized);
    
    // Test short n-grams
    auto short_ngrams = text_utils::generate_ngrams("hi", 5);
    ASSERT_EQ(1, short_ngrams.size());
    ASSERT_STREQ("hi", short_ngrams[0]);
    
    // Test hash of empty string
    Hash empty_hash = hash_utils::xxhash64("");
    ASSERT_NE(0, empty_hash);  // Should still produce a valid hash
}

int main() {
    std::cout << "🛠️ RapidSift Utilities Test Suite" << std::endl;
    std::cout << "===================================" << std::endl << std::endl;
    
    TestSuite suite("Utilities");
    
    // Text processing tests
    suite.add_test("Text normalization", test_text_normalization);
    suite.add_test("Text tokenization", test_text_tokenization);
    suite.add_test("N-gram generation", test_ngram_generation);
    suite.add_test("To lowercase", test_to_lowercase);
    suite.add_test("Remove whitespace", test_remove_whitespace);
    
    // Hash function tests
    suite.add_test("Hash functions", test_hash_functions);
    suite.add_test("Hash consistency", test_hash_consistency);
    
    // Core class tests
    suite.add_test("Document creation", test_document_creation);
    suite.add_test("Deduplication result", test_deduplication_result);
    suite.add_test("Timer", test_timer);
    
    // I/O tests
    suite.add_test("File I/O TXT", test_file_io_txt);
    suite.add_test("File I/O CSV", test_file_io_csv);
    
    // Edge cases
    suite.add_test("Edge cases", test_edge_cases);
    
    suite.run_all();
    
    return 0;
} 