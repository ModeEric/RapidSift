#include "rapidsift/common.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <filesystem>

namespace rapidsift {

// Text utility implementations
namespace text_utils {

std::string normalize_text(const std::string& text) {
    std::string result = to_lowercase(text);
    
    // Replace multiple whitespaces with single space
    std::regex ws_regex("\\s+");
    result = std::regex_replace(result, ws_regex, " ");
    
    // Trim leading and trailing whitespace
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    
    return result;
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::regex word_regex("\\b\\w+\\b");
    std::sregex_iterator iter(text.begin(), text.end(), word_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        tokens.push_back(to_lowercase(iter->str()));
    }
    
    return tokens;
}

std::vector<std::string> generate_ngrams(const std::string& text, size_t n) {
    std::string normalized = normalize_text(text);
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

std::string remove_whitespace(const std::string& text) {
    std::string result;
    std::copy_if(text.begin(), text.end(), std::back_inserter(result), 
                 [](char c) { return !std::isspace(c); });
    return result;
}

std::string to_lowercase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace text_utils

// I/O utility implementations
namespace io_utils {

std::vector<Document> load_documents_from_file(const std::string& filename) {
    std::filesystem::path filepath(filename);
    std::string extension = filepath.extension().string();
    
    if (extension == ".txt") {
        return load_documents_from_txt(filename);
    } else if (extension == ".csv") {
        return load_documents_from_csv(filename);
    } else {
        throw std::runtime_error("Unsupported file format: " + extension);
    }
}

std::vector<Document> load_documents_from_txt(const std::string& filename) {
    std::vector<Document> documents;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::string line;
    DocumentId id = 0;
    
    while (std::getline(file, line)) {
        if (!line.empty()) {
            documents.emplace_back(line, id++);
        }
    }
    
    return documents;
}

std::vector<Document> load_documents_from_csv(const std::string& filename, const std::string& text_column) {
    std::vector<Document> documents;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    
    std::string line;
    if (!std::getline(file, line)) {
        throw std::runtime_error("Empty CSV file");
    }
    
    // Parse header
    std::vector<std::string> headers;
    std::stringstream ss(line);
    std::string header;
    
    while (std::getline(ss, header, ',')) {
        // Remove quotes if present
        if (header.front() == '"' && header.back() == '"') {
            header = header.substr(1, header.length() - 2);
        }
        headers.push_back(header);
    }
    
    // Find text column index
    auto it = std::find(headers.begin(), headers.end(), text_column);
    if (it == headers.end()) {
        throw std::runtime_error("Text column '" + text_column + "' not found in CSV");
    }
    size_t text_col_index = std::distance(headers.begin(), it);
    
    // Read data rows
    DocumentId id = 0;
    while (std::getline(file, line)) {
        std::vector<std::string> fields;
        std::stringstream row_ss(line);
        std::string field;
        
        while (std::getline(row_ss, field, ',')) {
            // Remove quotes if present
            if (field.front() == '"' && field.back() == '"') {
                field = field.substr(1, field.length() - 2);
            }
            fields.push_back(field);
        }
        
        if (text_col_index < fields.size() && !fields[text_col_index].empty()) {
            documents.emplace_back(fields[text_col_index], id++);
        }
    }
    
    return documents;
}

void save_documents_to_file(const std::vector<Document>& documents, const std::string& filename) {
    std::filesystem::path filepath(filename);
    std::string extension = filepath.extension().string();
    
    if (extension == ".txt") {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not create file: " + filename);
        }
        
        for (const auto& doc : documents) {
            file << doc.text() << '\n';
        }
    }
    else if (extension == ".csv") {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not create file: " + filename);
        }
        
        file << "text\n";
        for (const auto& doc : documents) {
            file << "\"" << doc.text() << "\"\n";
        }
    }
    else {
        throw std::runtime_error("Unsupported output format: " + extension);
    }
}

void save_results_to_csv(const DeduplicationResult& result, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not create file: " + filename);
    }
    
    const auto& weights = result.weights();
    bool has_weights = !weights.empty();
    
    if (has_weights) {
        file << "document,weight\n";
        for (size_t i = 0; i < result.unique_documents().size(); ++i) {
            file << "\"" << result.unique_documents()[i].text() << "\"," 
                 << weights[i] << "\n";
        }
    } else {
        file << "document\n";
        for (const auto& doc : result.unique_documents()) {
            file << "\"" << doc.text() << "\"\n";
        }
    }
}

} // namespace io_utils

// Statistics utility implementations
namespace stats_utils {

void print_deduplication_stats(const DeduplicationResult& result) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Deduplication Results\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Original documents:    " << result.original_count() << "\n";
    std::cout << "Unique documents:      " << result.unique_count() << "\n";
    std::cout << "Duplicates removed:    " << result.duplicates_removed() << "\n";
    std::cout << "Reduction percentage:  " << std::fixed << std::setprecision(1) 
              << result.reduction_percentage() << "%\n";
    std::cout << "Processing time:       " << result.processing_time().count() << " ms\n";
    std::cout << "Duplicate groups:      " << result.duplicate_groups().size() << "\n";
    
    if (!result.duplicate_groups().empty()) {
        size_t max_group_size = 0;
        for (const auto& group : result.duplicate_groups()) {
            max_group_size = std::max(max_group_size, group.size());
        }
        std::cout << "Largest group size:    " << max_group_size << "\n";
    }
    
    std::cout << std::string(60, '=') << "\n\n";
}

void print_progress(size_t current, size_t total, const std::string& stage) {
    if (total == 0) return;
    
    double percentage = (static_cast<double>(current) / total) * 100.0;
    int bar_width = 50;
    int filled = static_cast<int>((percentage / 100.0) * bar_width);
    
    std::cout << "\r" << stage << " [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) std::cout << "█";
        else std::cout << "░";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percentage << "% ";
    std::cout << "(" << current << "/" << total << ")";
    std::cout.flush();
    
    if (current == total) {
        std::cout << "\n";
    }
}

} // namespace stats_utils

} // namespace rapidsift 