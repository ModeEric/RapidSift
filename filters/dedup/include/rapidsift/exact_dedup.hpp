#pragma once

#include "common.hpp"
#include <unordered_map>

namespace rapidsift {

/**
 * @brief High-performance exact deduplication using hash-based matching
 * 
 * This class identifies documents that are byte-for-byte identical by computing
 * cryptographic hashes and removing all but one copy of each unique document.
 * 
 * Features:
 * - Multiple hash algorithms (MD5, SHA1, SHA256, xxHash)
 * - Parallel processing support
 * - Memory-efficient streaming for large datasets
 * - Configurable duplicate retention policy
 */
class ExactDeduplicator {
public:
    /**
     * @brief Construct exact deduplicator with configuration
     */
    explicit ExactDeduplicator(const ExactDedupConfig& config = ExactDedupConfig{});
    
    /**
     * @brief Deduplicate a collection of documents
     * @param documents Input documents to deduplicate
     * @param progress_callback Optional progress callback function
     * @return Deduplication result with unique documents and statistics
     */
    DeduplicationResult deduplicate(
        const std::vector<Document>& documents,
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Find duplicate groups without removing them
     * @param documents Input documents to analyze
     * @return Map of hash values to document groups
     */
    std::unordered_map<Hash, std::vector<DocumentId>> find_duplicate_groups(
        const std::vector<Document>& documents,
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Streaming deduplication for large datasets
     * @param input_stream Input stream of documents
     * @param output_stream Output stream for unique documents
     * @param batch_size Number of documents to process at once
     */
    void deduplicate_stream(
        std::istream& input_stream,
        std::ostream& output_stream,
        size_t batch_size = 10000
    );
    
    /**
     * @brief Get/set configuration
     */
    const ExactDedupConfig& config() const { return config_; }
    void set_config(const ExactDedupConfig& config) { config_ = config; }
    
    /**
     * @brief Get statistics from last operation
     */
    size_t total_processed() const { return total_processed_; }
    size_t unique_found() const { return unique_found_; }
    size_t duplicates_removed() const { return duplicates_removed_; }
    std::chrono::milliseconds last_processing_time() const { return last_processing_time_; }

private:
    ExactDedupConfig config_;
    
    // Statistics from last operation
    size_t total_processed_ = 0;
    size_t unique_found_ = 0;
    size_t duplicates_removed_ = 0;
    std::chrono::milliseconds last_processing_time_{0};
    
    /**
     * @brief Compute hash for a single document
     */
    Hash compute_document_hash(const Document& doc) const;
    
    /**
     * @brief Parallel hash computation
     */
    std::vector<Hash> compute_hashes_parallel(const std::vector<Document>& documents) const;
    
    /**
     * @brief Sequential hash computation
     */
    std::vector<Hash> compute_hashes_sequential(const std::vector<Document>& documents) const;
    
    /**
     * @brief Group documents by hash
     */
    std::unordered_map<Hash, std::vector<DocumentId>> group_by_hash(
        const std::vector<Document>& documents,
        const std::vector<Hash>& hashes
    ) const;
    
    /**
     * @brief Select unique documents from groups
     */
    std::vector<DocumentId> select_unique_documents(
        const std::unordered_map<Hash, std::vector<DocumentId>>& groups
    ) const;
};

} // namespace rapidsift 