#include "rapidsift/exact_dedup.hpp"
#include "rapidsift/common.hpp"
#include <xxhash.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace rapidsift {

ExactDeduplicator::ExactDeduplicator(const ExactDedupConfig& config)
    : config_(config) {}

DeduplicationResult ExactDeduplicator::deduplicate(
    const std::vector<Document>& documents,
    ProgressCallback progress_callback) {
    
    Timer timer;
    DeduplicationResult result;
    
    if (documents.empty()) {
        return result;
    }
    
    total_processed_ = documents.size();
    result.set_original_count(total_processed_);
    
    if (progress_callback) {
        progress_callback(0, total_processed_, "Computing hashes");
    }
    
    // Compute hashes for all documents
    std::vector<Hash> hashes;
    if (config_.parallel) {
        hashes = compute_hashes_parallel(documents);
    } else {
        hashes = compute_hashes_sequential(documents);
    }
    
    if (progress_callback) {
        progress_callback(total_processed_, total_processed_, "Grouping duplicates");
    }
    
    // Group documents by hash
    auto groups = group_by_hash(documents, hashes);
    
    // Select unique documents
    auto unique_indices = select_unique_documents(groups);
    
    // Build result
    for (DocumentId idx : unique_indices) {
        result.add_unique_document(documents[idx], idx);
    }
    
    // Track duplicate groups for statistics
    for (const auto& [hash, doc_indices] : groups) {
        if (doc_indices.size() > 1) {
            result.add_duplicate_group(doc_indices);
        }
    }
    
    unique_found_ = result.unique_count();
    duplicates_removed_ = total_processed_ - unique_found_;
    last_processing_time_ = timer.elapsed();
    result.set_processing_time(last_processing_time_);
    
    if (progress_callback) {
        progress_callback(total_processed_, total_processed_, "Complete");
    }
    
    return result;
}

std::unordered_map<Hash, std::vector<DocumentId>> ExactDeduplicator::find_duplicate_groups(
    const std::vector<Document>& documents,
    ProgressCallback progress_callback) {
    
    if (documents.empty()) {
        return {};
    }
    
    if (progress_callback) {
        progress_callback(0, documents.size(), "Computing hashes");
    }
    
    std::vector<Hash> hashes;
    if (config_.parallel) {
        hashes = compute_hashes_parallel(documents);
    } else {
        hashes = compute_hashes_sequential(documents);
    }
    
    if (progress_callback) {
        progress_callback(documents.size(), documents.size(), "Grouping documents");
    }
    
    auto groups = group_by_hash(documents, hashes);
    
    // Filter to only groups with duplicates
    std::unordered_map<Hash, std::vector<DocumentId>> duplicate_groups;
    for (const auto& [hash, doc_indices] : groups) {
        if (doc_indices.size() > 1) {
            duplicate_groups[hash] = doc_indices;
        }
    }
    
    return duplicate_groups;
}

Hash ExactDeduplicator::compute_document_hash(const Document& doc) const {
    return hash_utils::compute_hash(doc.text(), config_.algorithm);
}

std::vector<Hash> ExactDeduplicator::compute_hashes_parallel(
    const std::vector<Document>& documents) const {
    
    std::vector<Hash> hashes(documents.size());
    
#ifdef USE_OPENMP
    #pragma omp parallel for
    for (size_t i = 0; i < documents.size(); ++i) {
        hashes[i] = compute_document_hash(documents[i]);
    }
#else
    // Fallback to sequential if OpenMP not available
    return compute_hashes_sequential(documents);
#endif
    
    return hashes;
}

std::vector<Hash> ExactDeduplicator::compute_hashes_sequential(
    const std::vector<Document>& documents) const {
    
    std::vector<Hash> hashes;
    hashes.reserve(documents.size());
    
    for (const auto& doc : documents) {
        hashes.push_back(compute_document_hash(doc));
    }
    
    return hashes;
}

std::unordered_map<Hash, std::vector<DocumentId>> ExactDeduplicator::group_by_hash(
    const std::vector<Document>& documents,
    const std::vector<Hash>& hashes) const {
    
    std::unordered_map<Hash, std::vector<DocumentId>> groups;
    
    for (size_t i = 0; i < documents.size(); ++i) {
        groups[hashes[i]].push_back(static_cast<DocumentId>(i));
    }
    
    return groups;
}

std::vector<DocumentId> ExactDeduplicator::select_unique_documents(
    const std::unordered_map<Hash, std::vector<DocumentId>>& groups) const {
    
    std::vector<DocumentId> unique_indices;
    unique_indices.reserve(groups.size());
    
    for (const auto& [hash, doc_indices] : groups) {
        if (config_.keep_first) {
            unique_indices.push_back(doc_indices.front());
        } else {
            unique_indices.push_back(doc_indices.back());
        }
    }
    
    // Sort to maintain relative order if requested
    if (config_.keep_first) {
        std::sort(unique_indices.begin(), unique_indices.end());
    }
    
    return unique_indices;
}

void ExactDeduplicator::deduplicate_stream(
    std::istream& input_stream,
    std::ostream& output_stream,
    size_t batch_size) {
    
    std::unordered_set<Hash> seen_hashes;
    std::string line;
    size_t processed = 0;
    
    while (std::getline(input_stream, line)) {
        if (line.empty()) continue;
        
        Document doc(line, processed++);
        Hash doc_hash = compute_document_hash(doc);
        
        if (seen_hashes.find(doc_hash) == seen_hashes.end()) {
            seen_hashes.insert(doc_hash);
            output_stream << line << '\n';
        }
        
        // Periodic memory cleanup for very large streams
        if (processed % (batch_size * 10) == 0) {
            // Could implement LRU cache here if memory becomes an issue
        }
    }
}

} // namespace rapidsift

// Hash utility implementations
namespace rapidsift::hash_utils {

Hash compute_hash(const std::string& text, HashAlgorithm algorithm) {
    switch (algorithm) {
        case HashAlgorithm::MD5:
            return md5_hash(text);
        case HashAlgorithm::SHA1:
            return sha1_hash(text);
        case HashAlgorithm::SHA256:
            return sha256_hash(text);
        case HashAlgorithm::XXHASH64:
            return xxhash64(text);
        default:
            return xxhash64(text);
    }
}

Hash xxhash64(const std::string& text) {
            return hash_utils::xxhash64(text);
}

Hash md5_hash(const std::string& text) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(text.c_str()), text.size(), digest);
    
    // Convert first 8 bytes to uint64_t
    Hash result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | digest[i];
    }
    return result;
}

Hash sha1_hash(const std::string& text) {
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(text.c_str()), text.size(), digest);
    
    // Convert first 8 bytes to uint64_t
    Hash result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | digest[i];
    }
    return result;
}

Hash sha256_hash(const std::string& text) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(text.c_str()), text.size(), digest);
    
    // Convert first 8 bytes to uint64_t
    Hash result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 8) | digest[i];
    }
    return result;
}

} // namespace rapidsift::hash_utils 