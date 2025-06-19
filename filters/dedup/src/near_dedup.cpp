#include "rapidsift/near_dedup.hpp"
#include "rapidsift/common.hpp"
#include <xxhash.h>
#include <algorithm>
#include <random>
#include <regex>

namespace rapidsift {

// MinHashSignature implementation
MinHashSignature::MinHashSignature(size_t num_permutations)
    : num_permutations_(num_permutations) {
    
    signature_.resize(num_permutations_, std::numeric_limits<Hash>::max());
    
    // Generate random hash function parameters
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<Hash> dis;
    
    hash_functions_a_.reserve(num_permutations_);
    hash_functions_b_.reserve(num_permutations_);
    
    for (size_t i = 0; i < num_permutations_; ++i) {
        hash_functions_a_.push_back(dis(gen) | 1); // Ensure odd
        hash_functions_b_.push_back(dis(gen));
    }
}

void MinHashSignature::update(const std::string& element) {
            Hash element_hash = hash_utils::xxhash64(element);
    update(element_hash);
}

void MinHashSignature::update(Hash element_hash) {
    for (size_t i = 0; i < num_permutations_; ++i) {
        Hash perm_hash = compute_min_hash(element_hash, i);
        signature_[i] = std::min(signature_[i], perm_hash);
    }
}

double MinHashSignature::jaccard_similarity(const MinHashSignature& other) const {
    if (signature_.size() != other.signature_.size()) {
        return 0.0;
    }
    
    size_t matches = 0;
    for (size_t i = 0; i < signature_.size(); ++i) {
        if (signature_[i] == other.signature_[i]) {
            ++matches;
        }
    }
    
    return static_cast<double>(matches) / signature_.size();
}

Hash MinHashSignature::compute_min_hash(Hash element_hash, size_t perm_index) const {
    return hash_functions_a_[perm_index] * element_hash + hash_functions_b_[perm_index];
}

void MinHashSignature::clear() {
    std::fill(signature_.begin(), signature_.end(), std::numeric_limits<Hash>::max());
}

// LSHIndex implementation
LSHIndex::LSHIndex(size_t num_bands, size_t band_size)
    : num_bands_(num_bands), band_size_(band_size) {
    band_tables_.resize(num_bands_);
}

void LSHIndex::insert(DocumentId doc_id, const MinHashSignature& signature) {
    const auto& sig = signature.signature();
    
    for (size_t band = 0; band < num_bands_; ++band) {
        size_t start_idx = band * band_size_;
        size_t end_idx = std::min(start_idx + band_size_, sig.size());
        
        std::vector<Hash> band_values(sig.begin() + start_idx, sig.begin() + end_idx);
        size_t band_hash = hash_band(band_values);
        
        band_tables_[band][band_hash].push_back(doc_id);
    }
    
    ++doc_count_;
}

std::vector<DocumentId> LSHIndex::query(const MinHashSignature& signature, double threshold) const {
    const auto& sig = signature.signature();
    std::unordered_set<DocumentId> candidates;
    
    for (size_t band = 0; band < num_bands_; ++band) {
        size_t start_idx = band * band_size_;
        size_t end_idx = std::min(start_idx + band_size_, sig.size());
        
        std::vector<Hash> band_values(sig.begin() + start_idx, sig.begin() + end_idx);
        size_t band_hash = hash_band(band_values);
        
        auto it = band_tables_[band].find(band_hash);
        if (it != band_tables_[band].end()) {
            for (DocumentId doc_id : it->second) {
                candidates.insert(doc_id);
            }
        }
    }
    
    return std::vector<DocumentId>(candidates.begin(), candidates.end());
}

size_t LSHIndex::hash_band(const std::vector<Hash>& band) const {
    size_t result = 0;
    for (Hash h : band) {
        result = result * 31 + std::hash<Hash>{}(h);
    }
    return result;
}

void LSHIndex::clear() {
    for (auto& table : band_tables_) {
        table.clear();
    }
    doc_count_ = 0;
}

// SimHashSignature implementation
SimHashSignature::SimHashSignature(size_t num_bits)
    : num_bits_(num_bits), hash_value_(0) {}

void SimHashSignature::compute(const std::string& text) {
    auto tokens = text_utils::tokenize(text);
    compute(tokens);
}

void SimHashSignature::compute(const std::vector<std::string>& tokens) {
    std::vector<int> bit_vector(num_bits_, 0);
    
    for (const auto& token : tokens) {
        Hash hash_val = token_hash(token);
        
        for (size_t i = 0; i < num_bits_; ++i) {
            if ((hash_val >> i) & 1) {
                ++bit_vector[i];
            } else {
                --bit_vector[i];
            }
        }
    }
    
    hash_value_ = 0;
    for (size_t i = 0; i < num_bits_; ++i) {
        if (bit_vector[i] > 0) {
            hash_value_ |= (1ULL << i);
        }
    }
}

size_t SimHashSignature::hamming_distance(const SimHashSignature& other) const {
    uint64_t xor_result = hash_value_ ^ other.hash_value_;
    return __builtin_popcountll(xor_result);
}

double SimHashSignature::similarity(const SimHashSignature& other) const {
    size_t distance = hamming_distance(other);
    return 1.0 - (static_cast<double>(distance) / num_bits_);
}

Hash SimHashSignature::token_hash(const std::string& token) const {
    return hash_utils::xxhash64(token);
}

// NearDeduplicator implementation
NearDeduplicator::NearDeduplicator(const NearDedupConfig& config)
    : config_(config) {}

DeduplicationResult NearDeduplicator::deduplicate(
    const std::vector<Document>& documents,
    ProgressCallback progress_callback) {
    
    if (config_.method == NearDedupConfig::Method::MINHASH) {
        return deduplicate_minhash(documents, progress_callback);
    } else {
        return deduplicate_simhash(documents, progress_callback);
    }
}

DeduplicationResult NearDeduplicator::deduplicate_minhash(
    const std::vector<Document>& documents,
    ProgressCallback progress_callback) {
    
    Timer timer;
    DeduplicationResult result;
    
    if (documents.empty()) {
        return result;
    }
    
    result.set_original_count(documents.size());
    
    if (progress_callback) {
        progress_callback(0, documents.size(), "Computing MinHash signatures");
    }
    
    // Compute MinHash signatures
    auto signatures = compute_minhash_signatures_parallel(documents);
    
    if (progress_callback) {
        progress_callback(documents.size() / 2, documents.size(), "Building LSH index");
    }
    
    // Find similar groups using LSH
    auto similar_groups = find_similar_groups_lsh(documents, signatures);
    
    if (progress_callback) {
        progress_callback(documents.size(), documents.size(), "Selecting unique documents");
    }
    
    // Track which documents are already in a group
    std::vector<bool> processed(documents.size(), false);
    
    // Add one document from each similar group
    for (const auto& group : similar_groups) {
        if (group.empty()) continue;
        
        // Mark all documents in group as processed
        for (DocumentId doc_id : group) {
            processed[doc_id] = true;
        }
        
        // Add first document from group
        result.add_unique_document(documents[group[0]], group[0]);
        result.add_duplicate_group(group);
    }
    
    // Add documents that weren't in any similar group
    for (size_t i = 0; i < documents.size(); ++i) {
        if (!processed[i]) {
            result.add_unique_document(documents[i], static_cast<DocumentId>(i));
        }
    }
    
    result.set_processing_time(timer.elapsed());
    return result;
}

DeduplicationResult NearDeduplicator::deduplicate_simhash(
    const std::vector<Document>& documents,
    ProgressCallback progress_callback) {
    
    Timer timer;
    DeduplicationResult result;
    
    if (documents.empty()) {
        return result;
    }
    
    result.set_original_count(documents.size());
    
    if (progress_callback) {
        progress_callback(0, documents.size(), "Computing SimHash signatures");
    }
    
    // Compute SimHash signatures
    auto signatures = compute_simhash_signatures_parallel(documents);
    
    if (progress_callback) {
        progress_callback(documents.size() / 2, documents.size(), "Finding similar groups");
    }
    
    // Find similar groups
    auto similar_groups = find_similar_groups_simhash(documents, signatures);
    
    if (progress_callback) {
        progress_callback(documents.size(), documents.size(), "Selecting unique documents");
    }
    
    // Track which documents are already processed
    std::vector<bool> processed(documents.size(), false);
    
    // Add one document from each similar group
    for (const auto& group : similar_groups) {
        if (group.empty()) continue;
        
        for (DocumentId doc_id : group) {
            processed[doc_id] = true;
        }
        
        result.add_unique_document(documents[group[0]], group[0]);
        result.add_duplicate_group(group);
    }
    
    // Add unprocessed documents
    for (size_t i = 0; i < documents.size(); ++i) {
        if (!processed[i]) {
            result.add_unique_document(documents[i], static_cast<DocumentId>(i));
        }
    }
    
    result.set_processing_time(timer.elapsed());
    return result;
}

std::vector<MinHashSignature> NearDeduplicator::compute_minhash_signatures_parallel(
    const std::vector<Document>& documents) const {
    
    std::vector<MinHashSignature> signatures(documents.size(), MinHashSignature(config_.num_permutations));
    
#ifdef USE_OPENMP
    #pragma omp parallel for
    for (size_t i = 0; i < documents.size(); ++i) {
        signatures[i] = compute_minhash_signature(documents[i]);
    }
#else
    for (size_t i = 0; i < documents.size(); ++i) {
        signatures[i] = compute_minhash_signature(documents[i]);
    }
#endif
    
    return signatures;
}

std::vector<SimHashSignature> NearDeduplicator::compute_simhash_signatures_parallel(
    const std::vector<Document>& documents) const {
    
    std::vector<SimHashSignature> signatures(documents.size(), SimHashSignature(config_.simhash_bits));
    
#ifdef USE_OPENMP
    #pragma omp parallel for
    for (size_t i = 0; i < documents.size(); ++i) {
        signatures[i] = compute_simhash_signature(documents[i]);
    }
#else
    for (size_t i = 0; i < documents.size(); ++i) {
        signatures[i] = compute_simhash_signature(documents[i]);
    }
#endif
    
    return signatures;
}

MinHashSignature NearDeduplicator::compute_minhash_signature(const Document& doc) const {
    MinHashSignature signature(config_.num_permutations);
    
    auto ngrams = generate_ngrams(doc.text(), config_.ngram_size);
    for (const auto& ngram : ngrams) {
        signature.update(ngram);
    }
    
    return signature;
}

SimHashSignature NearDeduplicator::compute_simhash_signature(const Document& doc) const {
    SimHashSignature signature(config_.simhash_bits);
    signature.compute(doc.text());
    return signature;
}

std::vector<std::string> NearDeduplicator::generate_ngrams(const std::string& text, size_t n) const {
    std::string normalized = text_utils::normalize_text(text);
    std::vector<std::string> ngrams;
    
    if (normalized.size() < n) {
        ngrams.push_back(normalized);
        return ngrams;
    }
    
    for (size_t i = 0; i <= normalized.size() - n; ++i) {
        ngrams.push_back(normalized.substr(i, n));
    }
    
    return ngrams;
}

std::vector<std::vector<DocumentId>> NearDeduplicator::find_similar_groups_lsh(
    const std::vector<Document>& documents,
    const std::vector<MinHashSignature>& signatures) const {
    
    LSHIndex lsh_index(16, 8); // 16 bands, 8 rows per band
    std::vector<std::vector<DocumentId>> groups;
    std::vector<bool> processed(documents.size(), false);
    
    // Build LSH index
    for (size_t i = 0; i < documents.size(); ++i) {
        lsh_index.insert(static_cast<DocumentId>(i), signatures[i]);
    }
    
    // Find similar groups
    for (size_t i = 0; i < documents.size(); ++i) {
        if (processed[i]) continue;
        
        auto candidates = lsh_index.query(signatures[i], config_.threshold);
        std::vector<DocumentId> similar_docs;
        
        for (DocumentId candidate : candidates) {
            if (processed[candidate]) continue;
            
            double similarity = signatures[i].jaccard_similarity(signatures[candidate]);
            if (similarity >= config_.threshold) {
                similar_docs.push_back(candidate);
                processed[candidate] = true;
            }
        }
        
        if (similar_docs.size() > 1) {
            groups.push_back(similar_docs);
        }
    }
    
    return groups;
}

std::vector<std::vector<DocumentId>> NearDeduplicator::find_similar_groups_simhash(
    const std::vector<Document>& documents,
    const std::vector<SimHashSignature>& signatures) const {
    
    std::vector<std::vector<DocumentId>> groups;
    std::vector<bool> processed(documents.size(), false);
    
    size_t hamming_threshold = static_cast<size_t>((1.0 - config_.threshold) * config_.simhash_bits);
    
    for (size_t i = 0; i < documents.size(); ++i) {
        if (processed[i]) continue;
        
        std::vector<DocumentId> similar_docs;
        similar_docs.push_back(static_cast<DocumentId>(i));
        processed[i] = true;
        
        for (size_t j = i + 1; j < documents.size(); ++j) {
            if (processed[j]) continue;
            
            size_t distance = signatures[i].hamming_distance(signatures[j]);
            if (distance <= hamming_threshold) {
                similar_docs.push_back(static_cast<DocumentId>(j));
                processed[j] = true;
            }
        }
        
        if (similar_docs.size() > 1) {
            groups.push_back(similar_docs);
        }
    }
    
    return groups;
}

} // namespace rapidsift 