#include "rapidsift/content_processor.hpp"
#include "rapidsift/content_common.hpp"
#include <iostream>
#include <vector>

using namespace rapidsift::content;

int main() {
    std::cout << "RapidSift Content Filter Example\n";
    std::cout << "================================\n\n";
    
    // Create sample documents with various content issues
    std::vector<Document> test_documents = {
        // Safe document
        Document("doc1", "This is a normal, safe document about technology trends.", "https://techblog.com/article1"),
        
        // Document with PII
        Document("doc2", "Contact us at john.doe@company.com or call 555-123-4567 for more information.", "https://business-site.com/contact"),
        
        // Document from blocked domain
        Document("doc3", "This content comes from a suspicious domain.", "https://spam-site.com/fake-news"),
        
        // Document with suspicious URL patterns
        Document("doc4", "Click here for amazing deals!", "https://click-farm.info/redirect?track=12345"),
        
        // Document with mild profanity (placeholder)
        Document("doc5", "This document contains some mild offensive language that should be flagged.", "https://forum.com/post123"),
        
        // Document with multiple PII types
        Document("doc6", "My SSN is 123-45-6789 and my credit card is 4532-1234-5678-9012. I live at 123 Main Street.", "https://personal-blog.net/diary"),
        
        // Document with IP-based URL
        Document("doc7", "Visit our site for more info.", "http://192.168.1.100/malicious-content"),
        
        // Safe document with example email
        Document("doc8", "For examples, you can use test@example.com as a placeholder email address.", "https://tutorial-site.edu/email-guide")
    };
    
    // Configure content filtering
    ContentConfig config;
    
    // Domain filtering settings
    config.domain_config.blocked_domains = {"spam-site.com", "click-farm.info", "malware-host.net"};
    config.domain_config.allowed_domains = {"techblog.com", "tutorial-site.edu"};
    config.domain_config.block_ip_addresses = true;
    config.domain_config.check_url_shorteners = true;
    
    // Toxicity filtering settings
    config.toxicity_config.toxicity_threshold = 0.7;
    config.toxicity_config.use_keyword_filter = true;
    config.toxicity_config.context_aware = true;
    config.toxicity_config.profanity_keywords = {"offensive", "inappropriate"};
    
    // PII filtering settings
    config.pii_config.remove_emails = true;
    config.pii_config.remove_phones = true;
    config.pii_config.remove_ssn = true;
    config.pii_config.remove_credit_cards = true;
    config.pii_config.remove_addresses = true;
    config.pii_config.use_placeholders = true;
    config.pii_config.anonymize_instead_of_remove = true;
    
    // Processing settings
    config.sanitize_mode = true;  // Try to clean instead of rejecting
    config.verbose = true;
    
    // Create processor
    ContentProcessor processor(config);
    
    std::cout << "Processing " << test_documents.size() << " test documents...\n\n";
    
    // Process each document individually to show detailed results
    for (size_t i = 0; i < test_documents.size(); ++i) {
        const auto& doc = test_documents[i];
        
        std::cout << "Document " << (i + 1) << " (" << doc.id << "):\n";
        std::cout << "URL: " << doc.url << "\n";
        std::cout << "Original: " << doc.text.substr(0, 100);
        if (doc.text.length() > 100) std::cout << "...";
        std::cout << "\n";
        
        // Assess the document
        auto assessment = processor.assess_document(doc);
        
        std::cout << "Result: ";
        switch (assessment.final_result) {
            case FilterResult::KEEP:
                std::cout << "KEEP";
                break;
            case FilterResult::REJECT:
                std::cout << "REJECT";
                break;
            case FilterResult::SANITIZE:
                std::cout << "SANITIZE";
                break;
            default:
                std::cout << "UNKNOWN";
                break;
        }
        std::cout << "\n";
        
        std::cout << "Safety Score: " << assessment.overall_safety_score << "\n";
        
        // Show filter decisions
        if (!assessment.filter_decisions.empty()) {
            std::cout << "Filter Results:\n";
            for (const auto& decision : assessment.filter_decisions) {
                std::cout << "  - " << decision.details;
                if (decision.confidence > 0) {
                    std::cout << " (confidence: " << decision.confidence << ")";
                }
                std::cout << "\n";
            }
        }
        
        // Show sanitized text if applicable
        if (!assessment.final_sanitized_text.empty() && 
            assessment.final_sanitized_text != doc.text) {
            std::cout << "Sanitized: " << assessment.final_sanitized_text.substr(0, 100);
            if (assessment.final_sanitized_text.length() > 100) std::cout << "...";
            std::cout << "\n";
        }
        
        std::cout << std::string(50, '-') << "\n\n";
    }
    
    // Show overall statistics
    const auto& stats = processor.get_stats();
    std::cout << "Overall Statistics:\n";
    std::cout << "==================\n";
    std::cout << "Total processed: " << stats.total_processed << "\n";
    std::cout << "Kept: " << stats.kept << " (" << (stats.get_keep_rate() * 100) << "%)\n";
    std::cout << "Rejected: " << stats.rejected << " (" << (stats.get_rejection_rate() * 100) << "%)\n";
    std::cout << "Sanitized: " << stats.sanitized << " (" << (stats.get_sanitization_rate() * 100) << "%)\n";
    
    if (stats.emails_removed > 0 || stats.phones_removed > 0) {
        std::cout << "\nPII Removal:\n";
        std::cout << "Emails removed: " << stats.emails_removed << "\n";
        std::cout << "Phones removed: " << stats.phones_removed << "\n";
        std::cout << "Addresses removed: " << stats.addresses_removed << "\n";
    }
    
    // Demonstrate bulk processing
    std::cout << "\nBulk Processing Example:\n";
    std::cout << "========================\n";
    
    auto filtered_docs = processor.sanitize_documents(test_documents);
    
    std::cout << "Bulk processing results:\n";
    std::cout << "Input documents: " << test_documents.size() << "\n";
    std::cout << "Output documents: " << filtered_docs.size() << "\n";
    
    for (size_t i = 0; i < filtered_docs.size(); ++i) {
        std::cout << "Doc " << (i + 1) << ": ";
        if (filtered_docs[i].text != test_documents[i].text) {
            std::cout << "MODIFIED\n";
        } else {
            std::cout << "UNCHANGED\n";
        }
    }
    
    return 0;
} 