#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

#ifdef USE_OPENMP
#include <omp.h>
#endif

namespace rapidsift {

// Type aliases for better readability
using DocumentId = size_t;
using Hash = uint64_t;
using Weight = double;
using SimilarityScore = double;

// Forward declarations
class Document;
class DeduplicationResult;

/**
 * @brief Represents a text document with metadata
 */
class Document {
public:
    Document() = default;
    Document(const std::string& text, DocumentId id = 0) 
        : text_(text), id_(id) {}
    
    const std::string& text() const { return text_; }
    DocumentId id() const { return id_; }
    
    void set_text(const std::string& text) { text_ = text; }
    void set_id(DocumentId id) { id_ = id; }
    
    bool empty() const { return text_.empty(); }
    size_t size() const { return text_.size(); }

private:
    std::string text_;
    DocumentId id_ = 0;
};

/**
 * @brief Stores the results of deduplication operations
 */
class DeduplicationResult {
public:
    DeduplicationResult() = default;
    
    void add_unique_document(const Document& doc, DocumentId original_id) {
        unique_documents_.push_back(doc);
        original_indices_.push_back(original_id);
    }
    
    void set_weights(const std::vector<Weight>& weights) {
        weights_ = weights;
    }
    
    void add_duplicate_group(const std::vector<DocumentId>& group) {
        duplicate_groups_.push_back(group);
    }
    
    const std::vector<Document>& unique_documents() const { return unique_documents_; }
    const std::vector<DocumentId>& original_indices() const { return original_indices_; }
    const std::vector<Weight>& weights() const { return weights_; }
    const std::vector<std::vector<DocumentId>>& duplicate_groups() const { return duplicate_groups_; }
    
    size_t original_count() const { return original_count_; }
    size_t unique_count() const { return unique_documents_.size(); }
    size_t duplicates_removed() const { return original_count_ - unique_count(); }
    double reduction_percentage() const { 
        return original_count_ > 0 ? (double(duplicates_removed()) / original_count_) * 100.0 : 0.0;
    }
    
    void set_original_count(size_t count) { original_count_ = count; }
    void set_processing_time(std::chrono::milliseconds time) { processing_time_ = time; }
    std::chrono::milliseconds processing_time() const { return processing_time_; }

private:
    std::vector<Document> unique_documents_;
    std::vector<DocumentId> original_indices_;
    std::vector<Weight> weights_;
    std::vector<std::vector<DocumentId>> duplicate_groups_;
    size_t original_count_ = 0;
    std::chrono::milliseconds processing_time_{0};
};

/**
 * @brief Hash algorithm types
 */
enum class HashAlgorithm {
    MD5,
    SHA1,
    SHA256,
    XXHASH64
};

/**
 * @brief Configuration for exact deduplication
 */
struct ExactDedupConfig {
    HashAlgorithm algorithm = HashAlgorithm::XXHASH64;
    bool keep_first = true;
    bool parallel = true;
};

/**
 * @brief Configuration for near-duplicate detection
 */
struct NearDedupConfig {
    enum class Method { MINHASH, SIMHASH };
    
    Method method = Method::MINHASH;
    double threshold = 0.8;
    size_t num_permutations = 128;
    size_t ngram_size = 5;
    size_t simhash_bits = 64;
    bool parallel = true;
};

/**
 * @brief Configuration for semantic deduplication
 */
struct SemanticDedupConfig {
    enum class Method { TFIDF_COSINE, WORD2VEC_SIMILARITY };
    
    Method method = Method::TFIDF_COSINE;
    double threshold = 0.85;
    size_t min_doc_frequency = 2;
    size_t max_features = 10000;
    bool parallel = true;
};

/**
 * @brief Configuration for soft deduplication
 */
struct SoftDedupConfig {
    enum class Method { HASH, MINHASH, NGRAM };
    
    Method method = Method::HASH;
    Weight min_weight = 0.1;
    double decay_factor = 0.5;
    double similarity_threshold = 0.8;
    bool parallel = true;
};

/**
 * @brief Progress callback function type
 */
using ProgressCallback = std::function<void(size_t current, size_t total, const std::string& stage)>;

/**
 * @brief Utility class for performance timing
 */
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    std::chrono::milliseconds elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
    }
    
    double elapsed_seconds() const {
        return elapsed().count() / 1000.0;
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

/**
 * @brief Thread pool for parallel processing
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    template<typename F>
    void enqueue(F&& f);
    
    void wait_all();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

/**
 * @brief Text preprocessing utilities
 */
namespace text_utils {
    
    std::string normalize_text(const std::string& text);
    std::vector<std::string> tokenize(const std::string& text);
    std::vector<std::string> generate_ngrams(const std::string& text, size_t n);
    std::string remove_whitespace(const std::string& text);
    std::string to_lowercase(const std::string& text);
    
} // namespace text_utils

/**
 * @brief Hash function utilities
 */
namespace hash_utils {
    
    Hash compute_hash(const std::string& text, HashAlgorithm algorithm);
    Hash md5_hash(const std::string& text);
    Hash sha1_hash(const std::string& text);
    Hash sha256_hash(const std::string& text);
    Hash xxhash64(const std::string& text);
    
} // namespace hash_utils

/**
 * @brief File I/O utilities
 */
namespace io_utils {
    
    std::vector<Document> load_documents_from_file(const std::string& filename);
    std::vector<Document> load_documents_from_txt(const std::string& filename);
    std::vector<Document> load_documents_from_csv(const std::string& filename, const std::string& text_column = "text");
    
    void save_documents_to_file(const std::vector<Document>& documents, const std::string& filename);
    void save_results_to_csv(const DeduplicationResult& result, const std::string& filename);
    
} // namespace io_utils

/**
 * @brief Statistics and reporting utilities
 */
namespace stats_utils {
    
    void print_deduplication_stats(const DeduplicationResult& result);
    void print_progress(size_t current, size_t total, const std::string& stage);
    
} // namespace stats_utils

} // namespace rapidsift 