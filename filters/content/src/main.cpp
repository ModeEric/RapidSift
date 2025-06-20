#include "rapidsift/content_processor.hpp"
#include "rapidsift/content_common.hpp"
#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <chrono>

using json = nlohmann::json;
using namespace rapidsift::content;

void print_stats(const ContentStats& stats) {
    std::cout << "\n=== Content Filtering Statistics ===" << std::endl;
    std::cout << "Total processed: " << stats.total_processed << std::endl;
    std::cout << "Kept: " << stats.kept << " (" << (stats.get_keep_rate() * 100) << "%)" << std::endl;
    std::cout << "Rejected: " << stats.rejected << " (" << (stats.get_rejection_rate() * 100) << "%)" << std::endl;
    std::cout << "Sanitized: " << stats.sanitized << " (" << (stats.get_sanitization_rate() * 100) << "%)" << std::endl;
    
    if (!stats.action_counts.empty()) {
        std::cout << "\nAction reasons:" << std::endl;
        for (const auto& pair : stats.action_counts) {
            std::string reason;
            switch (pair.first) {
                case ActionReason::BLOCKED_DOMAIN: reason = "Blocked Domain"; break;
                case ActionReason::SUSPICIOUS_URL: reason = "Suspicious URL"; break;
                case ActionReason::TOXICITY_HIGH: reason = "High Toxicity"; break;
                case ActionReason::HATE_SPEECH: reason = "Hate Speech"; break;
                case ActionReason::NSFW_CONTENT: reason = "NSFW Content"; break;
                case ActionReason::PII_DETECTED: reason = "PII Detected"; break;
                case ActionReason::PRIVACY_VIOLATION: reason = "Privacy Violation"; break;
                default: reason = "Other"; break;
            }
            std::cout << "  " << reason << ": " << pair.second << std::endl;
        }
    }
    
    if (stats.emails_removed > 0 || stats.phones_removed > 0 || stats.addresses_removed > 0) {
        std::cout << "\nPII Removal Statistics:" << std::endl;
        std::cout << "  Emails removed: " << stats.emails_removed << std::endl;
        std::cout << "  Phones removed: " << stats.phones_removed << std::endl;
        std::cout << "  Addresses removed: " << stats.addresses_removed << std::endl;
        std::cout << "  Names removed: " << stats.names_removed << std::endl;
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
            doc.source_ip = item.value("source_ip", "");
            
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
            doc_json["source_ip"] = doc.source_ip;
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

ContentConfig create_config_from_args(const cxxopts::ParseResult& result) {
    ContentConfig config;
    
    // Domain filter settings
    if (result.count("blocked-domains")) {
        auto domains = result["blocked-domains"].as<std::vector<std::string>>();
        for (const auto& domain : domains) {
            config.domain_config.blocked_domains.insert(domain);
        }
    }
    
    if (result.count("allowed-domains")) {
        auto domains = result["allowed-domains"].as<std::vector<std::string>>();
        for (const auto& domain : domains) {
            config.domain_config.allowed_domains.insert(domain);
        }
    }
    
    if (result.count("block-ip-urls")) {
        config.domain_config.block_ip_addresses = true;
    }
    
    // Toxicity filter settings
    if (result.count("toxicity-threshold")) {
        config.toxicity_config.toxicity_threshold = result["toxicity-threshold"].as<double>();
    }
    
    if (result.count("hate-threshold")) {
        config.toxicity_config.hate_speech_threshold = result["hate-threshold"].as<double>();
    }
    
    // PII filter settings
    if (result.count("remove-emails")) {
        config.pii_config.remove_emails = true;
    }
    
    if (result.count("remove-phones")) {
        config.pii_config.remove_phones = true;
    }
    
    if (result.count("remove-ssn")) {
        config.pii_config.remove_ssn = true;
    }
    
    if (result.count("sanitize-mode")) {
        config.sanitize_mode = true;
    }
    
    if (result.count("strict-mode")) {
        config.strict_mode = true;
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
    cxxopts::Options options("content_filter", "RapidSift Content Filter - Remove unsafe, toxic, and private content");
    
    options.add_options()
        ("h,help", "Print usage")
        ("i,input", "Input file", cxxopts::value<std::string>())
        ("o,output", "Output file", cxxopts::value<std::string>())
        ("f,format", "Input/output format (text|json)", cxxopts::value<std::string>()->default_value("text"))
        ("c,config", "Configuration file", cxxopts::value<std::string>())
        ("stats", "Statistics output file", cxxopts::value<std::string>())
        
        // Domain filtering options
        ("blocked-domains", "Comma-separated list of blocked domains", cxxopts::value<std::vector<std::string>>())
        ("allowed-domains", "Comma-separated list of allowed domains", cxxopts::value<std::vector<std::string>>())
        ("block-ip-urls", "Block URLs with IP addresses")
        ("domain-list", "Load domain blocklist from file", cxxopts::value<std::string>())
        
        // Toxicity filtering options
        ("toxicity-threshold", "Toxicity rejection threshold (0-1)", cxxopts::value<double>()->default_value("0.7"))
        ("hate-threshold", "Hate speech threshold (0-1)", cxxopts::value<double>()->default_value("0.8"))
        ("nsfw-threshold", "NSFW content threshold (0-1)", cxxopts::value<double>()->default_value("0.8"))
        ("enable-context", "Enable context-aware toxicity detection")
        
        // PII filtering options
        ("remove-emails", "Remove email addresses")
        ("remove-phones", "Remove phone numbers")
        ("remove-ssn", "Remove social security numbers")
        ("remove-addresses", "Remove physical addresses")
        ("remove-names", "Remove person names")
        ("use-placeholders", "Use placeholders instead of removing PII")
        
        // Processing modes
        ("sanitize-mode", "Sanitize content instead of rejecting")
        ("strict-mode", "Strict filtering mode")
        ("privacy-mode", "Focus on privacy protection")
        
        // Processing options
        ("t,threads", "Number of threads (0 = auto)", cxxopts::value<size_t>()->default_value("0"))
        ("v,verbose", "Verbose output")
        ("analyze", "Analyze without filtering")
        ("benchmark", "Run benchmark mode");
    
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
        ContentConfig config = create_config_from_args(result);
        
        if (result.count("config")) {
            // TODO: Load from JSON file
            std::cout << "Loading configuration from: " << result["config"].as<std::string>() << std::endl;
        }
        
        // Create processor
        ContentProcessor processor(config);
        
        // Load domain lists if specified
        if (result.count("domain-list")) {
            processor.load_blocked_domains(result["domain-list"].as<std::string>());
        }
        
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
        auto progress_callback = [config](size_t processed, size_t total, const ContentStats& stats) {
            if (config.verbose) {
                std::cout << "\rProcessed: " << processed << "/" << total 
                         << " (" << (processed * 100 / total) << "%) "
                         << "Kept: " << stats.kept 
                         << " Rejected: " << stats.rejected 
                         << " Sanitized: " << stats.sanitized << std::flush;
            }
        };
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (result.count("analyze")) {
            // Analysis mode - don't filter, just analyze
            std::cout << "\nRunning analysis mode..." << std::endl;
            auto assessments = processor.assess_documents(documents, progress_callback);
            
            std::cout << "\n\nAnalysis Results:" << std::endl;
            size_t reject_count = 0;
            size_t sanitize_count = 0;
            
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
                } else if (assessment.final_result == FilterResult::SANITIZE) {
                    sanitize_count++;
                    if (config.verbose) {
                        std::cout << "SANITIZE: " << assessment.document.id << " - ";
                        for (const auto& decision : assessment.filter_decisions) {
                            if (decision.result == FilterResult::SANITIZE) {
                                std::cout << decision.details << "; ";
                            }
                        }
                        std::cout << std::endl;
                    }
                }
            }
            
            std::cout << "Would reject " << reject_count << "/" << documents.size() 
                     << " (" << (reject_count * 100 / documents.size()) << "%) documents" << std::endl;
            std::cout << "Would sanitize " << sanitize_count << "/" << documents.size() 
                     << " (" << (sanitize_count * 100 / documents.size()) << "%) documents" << std::endl;
            
        } else {
            // Filtering mode
            std::cout << "\nFiltering documents..." << std::endl;
            
            std::vector<Document> processed_docs;
            if (config.sanitize_mode) {
                processed_docs = processor.sanitize_documents(documents, progress_callback);
            } else {
                processed_docs = processor.filter_documents(documents, progress_callback);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            
            std::cout << "\nFiltering completed in " << duration.count() << "ms" << std::endl;
            
            // Save results
            if (result.count("output")) {
                std::string output_file = result["output"].as<std::string>();
                std::cout << "Saving " << processed_docs.size() << " processed documents to: " << output_file << std::endl;
                save_filtered_documents(processed_docs, output_file, format);
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