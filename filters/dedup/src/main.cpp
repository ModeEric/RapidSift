#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

#include "rapidsift/exact_dedup.hpp"
#include "rapidsift/near_dedup.hpp"
#include "rapidsift/language_filter.hpp"
#include "rapidsift/text_extractor.hpp"
#include "rapidsift/common.hpp"

using namespace rapidsift;

void print_banner() {
    std::cout << "RapidSift C++ - High-Performance Text Deduplication\n";
    std::cout << "Version 1.0.0 - Ultra-fast duplicate detection\n";
    std::cout << "==================================================\n\n";
}

void print_help() {
    std::cout << "Usage: rapidsift [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help              Show this help message\n";
    std::cout << "  --mode MODE         Processing mode: exact, near, language, extract, benchmark\n";
    std::cout << "  --input FILE        Input file (TXT or CSV)\n";
    std::cout << "  --output FILE       Output file (optional)\n";
    std::cout << "  --algorithm ALGO    Hash algorithm for exact mode: md5, sha1, sha256, xxhash (default: xxhash)\n";
    std::cout << "  --method METHOD     Method for near mode: minhash, simhash (default: minhash)\n";
    std::cout << "  --threshold FLOAT   Similarity threshold for near mode (default: 0.8)\n";
    std::cout << "\nLanguage Filtering Options:\n";
    std::cout << "  --languages LANGS   Target languages (comma-separated, e.g., en,es,fr)\n";
    std::cout << "  --min-confidence N  Minimum confidence threshold (default: 0.65)\n";
    std::cout << "  --min-length N      Minimum text length for filtering (default: 10)\n";
    std::cout << "  --mixed-languages   Allow mixed-language documents (default: reject)\n";
    std::cout << "  --lang-stats        Show language statistics only\n";
    std::cout << "\nText Extraction Options:\n";
    std::cout << "  --html-input        Input contains HTML documents\n";
    std::cout << "  --extract-title     Extract document titles\n";
    std::cout << "  --remove-boilerplate Remove navigation, ads, footers (default: true)\n";
    std::cout << "  --min-text-ratio N  Minimum text/HTML ratio (default: 0.3)\n";
    std::cout << "  --quality-threshold N Minimum quality score (default: 0.0)\n";
    std::cout << "  --extraction-report FILE Save extraction quality report\n";
    std::cout << "\nExamples:\n";
    std::cout << "  rapidsift --mode exact --input data.txt --output unique.txt\n";
    std::cout << "  rapidsift --mode near --method minhash --threshold 0.8 --input data.txt\n";
    std::cout << "  rapidsift --mode language --languages en --min-confidence 0.7 --input data.txt\n";
    std::cout << "  rapidsift --mode language --lang-stats --input data.txt\n";
    std::cout << "  rapidsift --mode extract --html-input --input pages.txt --output clean.txt\n";
    std::cout << "  rapidsift --mode extract --remove-boilerplate --quality-threshold 0.5 --input web.txt\n";
    std::cout << "  rapidsift --mode benchmark --input data.txt\n";
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

void print_deduplication_stats(const DeduplicationResult& result, const std::string& method) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << method << " Deduplication Results\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Original documents:    " << result.original_count() << "\n";
    std::cout << "Unique documents:      " << result.unique_count() << "\n";
    std::cout << "Duplicates removed:    " << result.duplicates_removed() << "\n";
    std::cout << "Reduction percentage:  " << std::fixed << std::setprecision(1) 
              << result.reduction_percentage() << "%\n";
    std::cout << "Processing time:       " << result.processing_time().count() << " ms\n";
    std::cout << "Duplicate groups:      " << result.duplicate_groups().size() << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

std::string get_arg_value(const std::vector<std::string>& args, const std::string& flag) {
    for (size_t i = 0; i < args.size() - 1; ++i) {
        if (args[i] == flag) {
            return args[i + 1];
        }
    }
    return "";
}

bool has_flag(const std::vector<std::string>& args, const std::string& flag) {
    return std::find(args.begin(), args.end(), flag) != args.end();
}

std::vector<std::string> parse_languages(const std::string& lang_str) {
    std::vector<std::string> languages;
    if (lang_str.empty()) return languages;
    
    std::istringstream iss(lang_str);
    std::string lang;
    
    while (std::getline(iss, lang, ',')) {
        // Trim whitespace
        lang.erase(0, lang.find_first_not_of(" \t"));
        lang.erase(lang.find_last_not_of(" \t") + 1);
        
        if (!lang.empty()) {
            languages.push_back(lang);
        }
    }
    
    return languages;
}

void print_language_filter_stats(const LanguageFilterResult& result) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Language Filtering Results\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Total processed:       " << result.total_processed << "\n";
    std::cout << "Documents kept:        " << result.total_kept << "\n";
    std::cout << "Documents rejected:    " << result.total_rejected << "\n";
    std::cout << "Kept percentage:       " << std::fixed << std::setprecision(1) 
              << result.kept_percentage() << "%\n";
    std::cout << "Average confidence:    " << std::fixed << std::setprecision(3) 
              << result.average_confidence << "\n";
    
    std::cout << "\nLanguage Distribution:\n";
    for (const auto& [lang, count] : result.language_counts) {
        std::string lang_name = language_utils::iso639_1_to_name(lang);
        std::cout << "  " << lang << " (" << lang_name << "): " << count << " documents\n";
    }
    std::cout << std::string(60, '=') << "\n\n";
}

int run_exact_dedup(const std::vector<std::string>& args) {
    std::string input_file = get_arg_value(args, "--input");
    std::string output_file = get_arg_value(args, "--output");
    std::string algorithm = get_arg_value(args, "--algorithm");
    
    if (input_file.empty()) {
        std::cerr << "Error: --input is required for exact mode\n";
        return 1;
    }
    
    if (algorithm.empty()) algorithm = "xxhash";
    
    // Configure exact deduplication
    ExactDedupConfig config;
    config.parallel = true;
    config.keep_first = true;
    
    if (algorithm == "md5") config.algorithm = HashAlgorithm::MD5;
    else if (algorithm == "sha1") config.algorithm = HashAlgorithm::SHA1;
    else if (algorithm == "sha256") config.algorithm = HashAlgorithm::SHA256;
    else if (algorithm == "xxhash") config.algorithm = HashAlgorithm::XXHASH64;
    else {
        std::cerr << "Unknown hash algorithm: " << algorithm << std::endl;
        return 1;
    }
    
    try {
        // Load documents
        std::cout << "Loading documents from: " << input_file << std::endl;
        auto documents = io_utils::load_documents_from_file(input_file);
        std::cout << "Loaded " << documents.size() << " documents\n\n";
        
        // Initialize deduplicator
        ExactDeduplicator deduplicator(config);
        
        // Run deduplication with progress callback
        auto result = deduplicator.deduplicate(documents, print_progress);
        
        // Print statistics
        print_deduplication_stats(result, "Exact");
        
        // Save results
        if (!output_file.empty()) {
            io_utils::save_documents_to_file(result.unique_documents(), output_file);
            std::cout << "Results saved to: " << output_file << std::endl;
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int run_near_dedup(const std::vector<std::string>& args) {
    std::string input_file = get_arg_value(args, "--input");
    std::string output_file = get_arg_value(args, "--output");
    std::string method = get_arg_value(args, "--method");
    std::string threshold_str = get_arg_value(args, "--threshold");
    
    if (input_file.empty()) {
        std::cerr << "Error: --input is required for near mode\n";
        return 1;
    }
    
    if (method.empty()) method = "minhash";
    double threshold = threshold_str.empty() ? 0.8 : std::stod(threshold_str);
    
    // Configure near deduplication
    NearDedupConfig config;
    config.threshold = threshold;
    config.parallel = true;
    
    if (method == "minhash") config.method = NearDedupConfig::Method::MINHASH;
    else if (method == "simhash") config.method = NearDedupConfig::Method::SIMHASH;
    else {
        std::cerr << "Unknown method: " << method << std::endl;
        return 1;
    }
    
    try {
        // Load documents
        std::cout << "Loading documents from: " << input_file << std::endl;
        auto documents = io_utils::load_documents_from_file(input_file);
        std::cout << "Loaded " << documents.size() << " documents\n\n";
        
        // Initialize deduplicator
        NearDeduplicator deduplicator(config);
        
        // Run deduplication with progress callback
        auto result = deduplicator.deduplicate(documents, print_progress);
        
        // Print statistics
        print_deduplication_stats(result, method == "minhash" ? "MinHash" : "SimHash");
        
        // Save results
        if (!output_file.empty()) {
            io_utils::save_documents_to_file(result.unique_documents(), output_file);
            std::cout << "Results saved to: " << output_file << std::endl;
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int run_language_filter(const std::vector<std::string>& args) {
    std::string input_file = get_arg_value(args, "--input");
    std::string output_file = get_arg_value(args, "--output");
    std::string languages_str = get_arg_value(args, "--languages");
    std::string min_confidence_str = get_arg_value(args, "--min-confidence");
    std::string min_length_str = get_arg_value(args, "--min-length");
    bool allow_mixed = has_flag(args, "--mixed-languages");
    bool show_stats_only = has_flag(args, "--lang-stats");
    
    if (input_file.empty()) {
        std::cerr << "Error: --input is required for language mode\n";
        return 1;
    }
    
    // Parse configuration
    LanguageFilterConfig config;
    
    if (!languages_str.empty()) {
        config.target_languages = parse_languages(languages_str);
    }
    
    if (!min_confidence_str.empty()) {
        config.min_confidence = std::stod(min_confidence_str);
    }
    
    if (!min_length_str.empty()) {
        config.min_text_length = std::stoul(min_length_str);
    }
    
    config.remove_mixed_language = !allow_mixed;
    
    try {
        // Load documents
        std::cout << "Loading documents from: " << input_file << std::endl;
        auto documents = io_utils::load_documents_from_file(input_file);
        std::cout << "Loaded " << documents.size() << " documents\n\n";
        
        // Initialize language filter
        LanguageFilter filter(config);
        
        if (!filter.is_ready()) {
            std::cout << "Warning: Using simple language detector (fastText model not found)\n";
        }
        
        if (show_stats_only) {
            // Show language statistics only
            std::cout << "Analyzing language distribution...\n";
            auto stats = filter.get_language_stats(documents);
            
            std::cout << "\n" << std::string(60, '=') << "\n";
            std::cout << "Language Statistics\n";
            std::cout << std::string(60, '=') << "\n";
            std::cout << "Total documents: " << documents.size() << "\n\n";
            
            for (const auto& [lang, count] : stats) {
                std::string lang_name = language_utils::iso639_1_to_name(lang);
                double percentage = (double(count) / documents.size()) * 100.0;
                std::cout << "  " << lang << " (" << lang_name << "): " 
                         << count << " documents (" << std::fixed << std::setprecision(1) 
                         << percentage << "%)\n";
            }
            std::cout << std::string(60, '=') << "\n\n";
        } else {
            // Run language filtering with progress callback
            std::cout << "Filtering documents by language...\n";
            auto result = filter.filter(documents, print_progress);
            
            // Print statistics
            print_language_filter_stats(result);
            
            // Save results
            if (!output_file.empty()) {
                io_utils::save_documents_to_file(result.filtered_documents, output_file);
                std::cout << "Filtered documents saved to: " << output_file << std::endl;
            }
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

void print_extraction_stats(const std::vector<TextExtractionResult>& results) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Text Extraction Results\n";
    std::cout << std::string(60, '=') << "\n";
    
    size_t total_docs = results.size();
    size_t valid_docs = 0;
    size_t total_html_length = 0;
    size_t total_text_length = 0;
    double total_quality = 0.0;
    
    for (const auto& result : results) {
        if (result.is_valid()) {
            valid_docs++;
        }
        total_html_length += result.original_html_length;
        total_text_length += result.extracted_text_length;
        total_quality += result.quality_score();
    }
    
    std::cout << "Total documents:       " << total_docs << "\n";
    std::cout << "Valid extractions:     " << valid_docs << " (" 
              << std::fixed << std::setprecision(1) 
              << (100.0 * valid_docs / total_docs) << "%)\n";
    std::cout << "Total HTML size:       " << total_html_length << " chars\n";
    std::cout << "Total text extracted:  " << total_text_length << " chars\n";
    std::cout << "Overall text ratio:    " << std::fixed << std::setprecision(3)
              << (total_html_length > 0 ? double(total_text_length) / total_html_length : 0.0) << "\n";
    std::cout << "Average quality:       " << std::fixed << std::setprecision(3)
              << (total_docs > 0 ? total_quality / total_docs : 0.0) << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

int run_text_extraction(const std::vector<std::string>& args) {
    std::string input_file = get_arg_value(args, "--input");
    std::string output_file = get_arg_value(args, "--output");
    std::string report_file = get_arg_value(args, "--extraction-report");
    std::string min_ratio_str = get_arg_value(args, "--min-text-ratio");
    std::string quality_threshold_str = get_arg_value(args, "--quality-threshold");
    
    bool html_input = has_flag(args, "--html-input");
    bool extract_titles = has_flag(args, "--extract-title");
    bool remove_boilerplate = !has_flag(args, "--no-remove-boilerplate"); // Default true
    
    if (input_file.empty()) {
        std::cerr << "Error: --input is required for extract mode\n";
        return 1;
    }
    
    // Configure text extraction
    TextExtractionConfig config;
    config.extract_main_content = remove_boilerplate;
    config.remove_navigation = remove_boilerplate;
    config.remove_ads = remove_boilerplate;
    config.remove_headers_footers = remove_boilerplate;
    config.preserve_headings = extract_titles;
    
    if (!min_ratio_str.empty()) {
        config.min_text_ratio = std::stod(min_ratio_str);
    }
    
    double quality_threshold = 0.0;
    if (!quality_threshold_str.empty()) {
        quality_threshold = std::stod(quality_threshold_str);
    }
    
    try {
        std::cout << "Loading documents from: " << input_file << std::endl;
        
        std::vector<std::string> html_documents;
        std::vector<std::string> urls;
        
        if (html_input) {
            // Load documents and extract HTML content
            auto documents = io_utils::load_documents_from_file(input_file);
            for (const auto& doc : documents) {
                html_documents.push_back(doc.text());
                urls.push_back(std::to_string(doc.id())); // Convert ID to string
            }
            std::cout << "Loaded " << html_documents.size() << " HTML documents\n\n";
        } else {
            // Load documents and check if they contain HTML
            auto documents = io_utils::load_documents_from_file(input_file);
            std::cout << "Loaded " << documents.size() << " documents\n";
            
            // Convert to HTML strings and extract URLs if available
            for (const auto& doc : documents) {
                html_documents.push_back(doc.text());
                urls.push_back(std::to_string(doc.id())); // Convert ID to string
            }
            
            // Check if documents are likely HTML
            size_t html_count = 0;
            for (const auto& content : html_documents) {
                if (TextExtractor::is_likely_html(content)) {
                    html_count++;
                }
            }
            
            std::cout << "Detected " << html_count << " HTML documents (" 
                      << std::fixed << std::setprecision(1) 
                      << (100.0 * html_count / html_documents.size()) << "%)\n\n";
        }
        
        // Initialize text extractor
        TextExtractor extractor(config);
        
        // Extract text with progress callback
        std::cout << "Extracting text from documents...\n";
        auto results = extractor.extract_batch(html_documents, print_progress, urls);
        
        // Filter by quality threshold if specified
        if (quality_threshold > 0.0) {
            auto filtered_results = results;
            filtered_results.erase(
                std::remove_if(filtered_results.begin(), filtered_results.end(),
                    [quality_threshold](const TextExtractionResult& result) {
                        return result.quality_score() < quality_threshold;
                    }),
                filtered_results.end()
            );
            
            std::cout << "Quality filtering: kept " << filtered_results.size() 
                      << " of " << results.size() << " documents\n";
            results = std::move(filtered_results);
        }
        
        // Print statistics
        print_extraction_stats(results);
        
        // Save extracted text
        if (!output_file.empty()) {
            std::vector<Document> output_documents;
            for (size_t i = 0; i < results.size(); ++i) {
                const auto& result = results[i];
                std::string doc_id = result.url.empty() ? 
                    "doc_" + std::to_string(i) : result.url;
                
                output_documents.emplace_back(result.extracted_text, 0); // Use default ID
            }
            
            io_utils::save_documents_to_file(output_documents, output_file);
            std::cout << "Extracted text saved to: " << output_file << std::endl;
        }
        
        // Save extraction report
        if (!report_file.empty()) {
            extraction_utils::save_extraction_report(results, report_file);
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int run_benchmark(const std::vector<std::string>& args) {
    std::string input_file = get_arg_value(args, "--input");
    
    if (input_file.empty()) {
        std::cerr << "Error: --input is required for benchmark mode\n";
        return 1;
    }
    
    try {
        // Load documents
        std::cout << "Loading documents for benchmark..." << std::endl;
        auto documents = io_utils::load_documents_from_file(input_file);
        std::cout << "Loaded " << documents.size() << " documents\n\n";
        
        std::cout << "Running comprehensive benchmark...\n";
        std::cout << std::string(80, '=') << "\n\n";
        
        // Benchmark exact deduplication
        {
            ExactDedupConfig config;
            config.algorithm = HashAlgorithm::XXHASH64;
            ExactDeduplicator deduplicator(config);
            
            auto result = deduplicator.deduplicate(documents);
            print_deduplication_stats(result, "Exact (xxHash)");
        }
        
        // Benchmark MinHash
        {
            NearDedupConfig config;
            config.method = NearDedupConfig::Method::MINHASH;
            config.threshold = 0.8;
            NearDeduplicator deduplicator(config);
            
            auto result = deduplicator.deduplicate(documents);
            print_deduplication_stats(result, "Near (MinHash)");
        }
        
        // Benchmark SimHash
        {
            NearDedupConfig config;
            config.method = NearDedupConfig::Method::SIMHASH;
            config.threshold = 0.8;
            NearDeduplicator deduplicator(config);
            
            auto result = deduplicator.deduplicate(documents);
            print_deduplication_stats(result, "Near (SimHash)");
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

int main(int argc, char* argv[]) {
    print_banner();
    
    // Convert to vector for easier handling
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    
    if (args.empty() || has_flag(args, "--help")) {
        print_help();
        return 0;
    }
    
    std::string mode = get_arg_value(args, "--mode");
    
    if (mode.empty()) {
        std::cerr << "Error: --mode is required\n";
        print_help();
        return 1;
    }
    
    if (mode == "exact") {
        return run_exact_dedup(args);
    } else if (mode == "near") {
        return run_near_dedup(args);
    } else if (mode == "language") {
        return run_language_filter(args);
    } else if (mode == "extract") {
        return run_text_extraction(args);
    } else if (mode == "benchmark") {
        return run_benchmark(args);
    } else {
        std::cerr << "Error: Unknown mode '" << mode << "'\n";
        std::cerr << "Available modes: exact, near, language, extract, benchmark\n";
        return 1;
    }
} 