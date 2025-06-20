#include "rapidsift/quality_processor.hpp"
#include "rapidsift/quality_common.hpp"
#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <chrono>

using json = nlohmann::json;
using namespace rapidsift::quality;

void print_stats(const QualityStats& stats) {
    std::cout << "\n=== Quality Filtering Statistics ===" << std::endl;
    std::cout << "Total processed: " << stats.total_processed << std::endl;
    std::cout << "Kept: " << stats.kept << " (" << (stats.get_keep_rate() * 100) << "%)" << std::endl;
    std::cout << "Rejected: " << stats.rejected << " (" << (stats.get_rejection_rate() * 100) << "%)" << std::endl;
    
    if (!stats.rejection_counts.empty()) {
        std::cout << "\nRejection reasons:" << std::endl;
        for (const auto& pair : stats.rejection_counts) {
            std::string reason;
            switch (pair.first) {
                case RejectionReason::TOO_SHORT: reason = "Too Short"; break;
                case RejectionReason::TOO_LONG: reason = "Too Long"; break;
                case RejectionReason::GIBBERISH: reason = "Gibberish"; break;
                case RejectionReason::HIGH_REPETITION: reason = "High Repetition"; break;
                case RejectionReason::BOILERPLATE: reason = "Boilerplate"; break;
                case RejectionReason::POOR_FORMATTING: reason = "Poor Formatting"; break;
                case RejectionReason::MACHINE_GENERATED: reason = "Machine Generated"; break;
                case RejectionReason::SUSPICIOUS_URL: reason = "Suspicious URL"; break;
                case RejectionReason::INVALID_METADATA: reason = "Invalid Metadata"; break;
                default: reason = "Other"; break;
            }
            std::cout << "  " << reason << ": " << pair.second << std::endl;
        }
    }
}

std::vector<Document> load_documents_from_file(const std::string& filename) {
    std::vector<Document> documents;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::string line;
    size_t doc_id = 0;
    
    // Simple format: each line is a document
    while (std::getline(file, line)) {
        if (!line.empty()) {
            Document doc("doc_" + std::to_string(doc_id++), line);
            documents.push_back(doc);
        }
    }
    
    return documents;
}

std::vector<Document> load_documents_from_json(const std::string& filename) {
    std::vector<Document> documents;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open JSON file: " + filename);
    }
    
    json j;
    file >> j;
    
    if (j.is_array()) {
        for (const auto& item : j) {
            Document doc;
            doc.id = item.value("id", "");
            doc.text = item.value("text", "");
            doc.url = item.value("url", "");
            doc.domain = item.value("domain", "");
            doc.content_type = item.value("content_type", "");
            
            if (item.contains("metadata") && item["metadata"].is_object()) {
                for (const auto& meta : item["metadata"].items()) {
                    doc.metadata[meta.key()] = meta.value();
                }
            }
            
            documents.push_back(doc);
        }
    }
    
    return documents;
}

void save_filtered_documents(const std::vector<Document>& documents, const std::string& filename, const std::string& format) {
    if (format == "json") {
        json j = json::array();
        for (const auto& doc : documents) {
            json doc_json;
            doc_json["id"] = doc.id;
            doc_json["text"] = doc.text;
            doc_json["url"] = doc.url;
            doc_json["domain"] = doc.domain;
            doc_json["content_type"] = doc.content_type;
            doc_json["metadata"] = doc.metadata;
            j.push_back(doc_json);
        }
        
        std::ofstream file(filename);
        file << j.dump(2);
    } else {
        // Text format: one document per line
        std::ofstream file(filename);
        for (const auto& doc : documents) {
            file << doc.text << std::endl;
        }
    }
}

QualityConfig create_config_from_args(const cxxopts::ParseResult& result) {
    QualityConfig config;
    
    // Length filter settings
    if (result.count("min-words")) {
        config.length_config.min_words = result["min-words"].as<size_t>();
    }
    if (result.count("max-words")) {
        config.length_config.max_words = result["max-words"].as<size_t>();
    }
    if (result.count("min-chars")) {
        config.length_config.min_chars = result["min-chars"].as<size_t>();
    }
    if (result.count("max-chars")) {
        config.length_config.max_chars = result["max-chars"].as<size_t>();
    }
    
    // Gibberish filter settings
    if (result.count("max-non-alpha")) {
        config.gibberish_config.max_non_alpha_ratio = result["max-non-alpha"].as<double>();
    }
    if (result.count("min-entropy")) {
        config.gibberish_config.min_entropy = result["min-entropy"].as<double>();
    }
    
    // General settings
    if (result.count("threads")) {
        config.num_threads = result["threads"].as<size_t>();
    }
    if (result.count("verbose")) {
        config.verbose = true;
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    cxxopts::Options options("quality_filter", "RapidSift Quality Filter - Remove low-quality text documents");
    
    options.add_options()
        ("h,help", "Print usage")
        ("i,input", "Input file", cxxopts::value<std::string>())
        ("o,output", "Output file", cxxopts::value<std::string>())
        ("f,format", "Input/output format (text|json)", cxxopts::value<std::string>()->default_value("text"))
        ("c,config", "Configuration file", cxxopts::value<std::string>())
        ("stats", "Statistics output file", cxxopts::value<std::string>())
        
        // Length filter options
        ("min-words", "Minimum word count", cxxopts::value<size_t>()->default_value("5"))
        ("max-words", "Maximum word count", cxxopts::value<size_t>()->default_value("1000000"))
        ("min-chars", "Minimum character count", cxxopts::value<size_t>()->default_value("20"))
        ("max-chars", "Maximum character count", cxxopts::value<size_t>()->default_value("10000000"))
        
        // Gibberish filter options
        ("max-non-alpha", "Maximum non-alphabetic ratio", cxxopts::value<double>()->default_value("0.3"))
        ("min-entropy", "Minimum character entropy", cxxopts::value<double>()->default_value("2.0"))
        
        // Processing options
        ("t,threads", "Number of threads (0 = auto)", cxxopts::value<size_t>()->default_value("0"))
        ("v,verbose", "Verbose output")
        ("benchmark", "Run benchmark mode")
        ("analyze", "Analyze without filtering");
    
    auto result = options.parse(argc, argv);
    
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }
    
    if (!result.count("input")) {
        std::cerr << "Error: Input file required" << std::endl;
        std::cout << options.help() << std::endl;
        return 1;
    }
    
    try {
        // Load configuration
        QualityConfig config = create_config_from_args(result);
        
        if (result.count("config")) {
            // TODO: Load from JSON file
            std::cout << "Loading configuration from: " << result["config"].as<std::string>() << std::endl;
        }
        
        // Create processor
        QualityProcessor processor(config);
        
        // Load documents
        std::string input_file = result["input"].as<std::string>();
        std::string format = result["format"].as<std::string>();
        
        std::cout << "Loading documents from: " << input_file << std::endl;
        
        std::vector<Document> documents;
        if (format == "json") {
            documents = load_documents_from_json(input_file);
        } else {
            documents = load_documents_from_file(input_file);
        }
        
        std::cout << "Loaded " << documents.size() << " documents" << std::endl;
        
        if (documents.empty()) {
            std::cout << "No documents to process" << std::endl;
            return 0;
        }
        
        // Progress callback
        auto progress_callback = [config](size_t processed, size_t total, const QualityStats& stats) {
            if (config.verbose) {
                std::cout << "\rProcessed: " << processed << "/" << total 
                         << " (" << (processed * 100 / total) << "%) "
                         << "Kept: " << stats.kept << " Rejected: " << stats.rejected << std::flush;
            }
        };
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (result.count("analyze")) {
            // Analysis mode - don't filter, just analyze
            std::cout << "\nRunning analysis mode..." << std::endl;
            auto assessments = processor.assess_documents(documents, progress_callback);
            
            std::cout << "\n\nAnalysis Results:" << std::endl;
            size_t reject_count = 0;
            for (const auto& assessment : assessments) {
                if (assessment.final_result == FilterResult::REJECT) {
                    reject_count++;
                    if (config.verbose) {
                        std::cout << "REJECT: " << assessment.document.id << " - ";
                        for (const auto& decision : assessment.filter_decisions) {
                            if (decision.result == FilterResult::REJECT) {
                                std::cout << decision.details << "; ";
                            }
                        }
                        std::cout << std::endl;
                    }
                }
            }
            std::cout << "Would reject " << reject_count << "/" << documents.size() 
                     << " (" << (reject_count * 100 / documents.size()) << "%) documents" << std::endl;
            
        } else {
            // Filtering mode
            std::cout << "\nFiltering documents..." << std::endl;
            auto filtered_docs = processor.filter_documents(documents, progress_callback);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            std::cout << "\nFiltering completed in " << duration.count() << "ms" << std::endl;
            
            // Save results
            if (result.count("output")) {
                std::string output_file = result["output"].as<std::string>();
                std::cout << "Saving " << filtered_docs.size() << " filtered documents to: " << output_file << std::endl;
                save_filtered_documents(filtered_docs, output_file, format);
            }
        }
        
        // Print statistics
        print_stats(processor.get_stats());
        
        // Save statistics if requested
        if (result.count("stats")) {
            std::string stats_file = result["stats"].as<std::string>();
            std::cout << "Saving statistics to: " << stats_file << std::endl;
            processor.save_stats(stats_file);
        }
        
        // Benchmark mode
        if (result.count("benchmark")) {
            std::cout << "\nRunning benchmark..." << std::endl;
            processor.benchmark_filters(documents);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 