#pragma once

#include "common.hpp"
#include <random>
#include <bitset>

namespace rapidsift {

/**
 * @brief MinHash signature for efficient similarity estimation
 */
class MinHashSignature {
public:
    explicit MinHashSignature(size_t num_permutations = 128);
    
    void update(const std::string& element);
    void update(Hash element_hash);
    
    double jaccard_similarity(const MinHashSignature& other) const;
    
    const std::vector<Hash>& signature() const { return signature_; }
    
    void clear();

private:
    std::vector<Hash> signature_;
    std::vector<Hash> hash_functions_a_;
    std::vector<Hash> hash_functions_b_;
    size_t num_permutations_;
    
    Hash compute_min_hash(Hash element_hash, size_t perm_index) const;
};

/**
 * @brief Locality Sensitive Hashing for efficient similarity search
 */
class LSHIndex {
public:
    explicit LSHIndex(size_t num_bands = 16, size_t band_size = 8);
    
    void insert(DocumentId doc_id, const MinHashSignature& signature);
    std::vector<DocumentId> query(const MinHashSignature& signature, double threshold = 0.8) const;
    
    void clear();
    size_t size() const { return doc_count_; }

private:
    size_t num_bands_;
    size_t band_size_;
    size_t doc_count_ = 0;
    
    // Maps band hash to list of document IDs
    std::vector<std::unordered_map<size_t, std::vector<DocumentId>>> band_tables_;
    
    size_t hash_band(const std::vector<Hash>& band) const;
};

/**
 * @brief SimHash implementation for compact document fingerprinting
 */
class SimHashSignature {
public:
    explicit SimHashSignature(size_t num_bits = 64);
    
    void compute(const std::string& text);
    void compute(const std::vector<std::string>& tokens);
    
    size_t hamming_distance(const SimHashSignature& other) const;
    double similarity(const SimHashSignature& other) const;
    
    uint64_t hash_value() const { return hash_value_; }

private:
    size_t num_bits_;
    uint64_t hash_value_ = 0;
    
    Hash token_hash(const std::string& token) const;
};

/**
 * @brief High-performance near-duplicate detection using fuzzy matching
 * 
 * This class identifies documents that are similar but not identical using
 * MinHash and SimHash techniques to catch duplicates with minor differences.
 * 
 * Features:
 * - MinHash with LSH for efficient similarity detection
 * - SimHash for compact fingerprinting
 * - Parallel processing support
 * - Configurable similarity thresholds
 * - Memory-efficient for large datasets
 */
class NearDeduplicator {
public:
    /**
     * @brief Construct near-duplicate deduplicator with configuration
     */
    explicit NearDeduplicator(const NearDedupConfig& config = NearDedupConfig{});
    
    /**
     * @brief Deduplicate documents using near-duplicate detection
     * @param documents Input documents to deduplicate
     * @param progress_callback Optional progress callback function
     * @return Deduplication result with unique documents and statistics
     */
    DeduplicationResult deduplicate(
        const std::vector<Document>& documents,
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Find similar document pairs without removing them
     * @param documents Input documents to analyze
     * @return Vector of similar document pairs with similarity scores
     */
    std::vector<std::tuple<DocumentId, DocumentId, SimilarityScore>> find_similar_pairs(
        const std::vector<Document>& documents,
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Compute MinHash signature for a document
     */
    MinHashSignature compute_minhash_signature(const Document& doc) const;
    
    /**
     * @brief Compute SimHash signature for a document
     */
    SimHashSignature compute_simhash_signature(const Document& doc) const;
    
    /**
     * @brief Get/set configuration
     */
    const NearDedupConfig& config() const { return config_; }
    void set_config(const NearDedupConfig& config) { config_ = config; }

private:
    NearDedupConfig config_;
    
    /**
     * @brief Deduplicate using MinHash + LSH
     */
    DeduplicationResult deduplicate_minhash(
        const std::vector<Document>& documents,
        ProgressCallback progress_callback
    );
    
    /**
     * @brief Deduplicate using SimHash
     */
    DeduplicationResult deduplicate_simhash(
        const std::vector<Document>& documents,
        ProgressCallback progress_callback
    );
    
    /**
     * @brief Generate n-grams from text
     */
    std::vector<std::string> generate_ngrams(const std::string& text, size_t n) const;
    
    /**
     * @brief Parallel MinHash computation
     */
    std::vector<MinHashSignature> compute_minhash_signatures_parallel(
        const std::vector<Document>& documents
    ) const;
    
    /**
     * @brief Parallel SimHash computation
     */
    std::vector<SimHashSignature> compute_simhash_signatures_parallel(
        const std::vector<Document>& documents
    ) const;
    
    /**
     * @brief Find similar documents using LSH
     */
    std::vector<std::vector<DocumentId>> find_similar_groups_lsh(
        const std::vector<Document>& documents,
        const std::vector<MinHashSignature>& signatures
    ) const;
    
    /**
     * @brief Find similar documents using SimHash
     */
    std::vector<std::vector<DocumentId>> find_similar_groups_simhash(
        const std::vector<Document>& documents,
        const std::vector<SimHashSignature>& signatures
    ) const;
};

} // namespace rapidsift 