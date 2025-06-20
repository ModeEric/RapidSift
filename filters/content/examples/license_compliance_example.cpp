#include "rapidsift/license_filter.hpp"
#include "rapidsift/content_common.hpp"
#include <iostream>
#include <vector>

using namespace rapidsift::content;

int main() {
    std::cout << "RapidSift License Compliance Filter Example\n";
    std::cout << "===========================================\n\n";
    
    // Create test documents with various license scenarios
    std::vector<Document> test_documents = {
        // Open license content
        Document("doc1", 
                "This article is licensed under Creative Commons CC BY 4.0. "
                "You are free to share and adapt this content.",
                "https://creativecommons.org/educational-content"),
                
        // Public domain content
        Document("doc2",
                "This work is in the public domain. No rights reserved. "
                "Free to use for any purpose.",
                "https://archive.org/historical-documents"),
                
        // Content with copyright notice
        Document("doc3",
                "© 2024 Major News Corporation. All rights reserved. "
                "This article may not be reproduced without permission.",
                "https://majornews.com/breaking-story"),
                
        // Paywalled content
        Document("doc4",
                "Subscribe to read the full article. Premium members only. "
                "This content is behind a paywall.",
                "https://premiumjournal.com/subscribers-only"),
                
        // Educational content with CC license
        Document("doc5",
                "MIT OpenCourseWare - Licensed under CC BY-NC-SA. "
                "This educational material is freely available for non-commercial use.",
                "https://ocw.mit.edu/courses/introduction"),
                
        // Government public domain
        Document("doc6",
                "This publication is produced by the US Government and is in the public domain. "
                "NASA scientific data is freely available.",
                "https://nasa.gov/research-findings"),
                
        // Wikipedia content
        Document("doc7",
                "Wikipedia content licensed under CC BY-SA 3.0. "
                "Text available under the Creative Commons Attribution-ShareAlike License.",
                "https://en.wikipedia.org/wiki/science-topic"),
                
        // Commercial content with unclear license
        Document("doc8",
                "This is premium business content from our professional services team. "
                "Contact us for licensing information.",
                "https://businessconsulting.com/insights")
    };
    
    std::cout << "=== Testing Different License Configurations ===\n\n";
    
    // Test 1: Strict Configuration
    std::cout << "1. STRICT CONFIGURATION (Research/Academic Use)\n";
    std::cout << "   - Only allows public domain and CC0\n";
    std::cout << "   - Rejects any unclear licenses\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto strict_config = license_utils::create_strict_config();
    LicenseFilter strict_filter(strict_config);
    
    size_t strict_kept = 0;
    for (const auto& doc : test_documents) {
        auto decision = strict_filter.evaluate(doc);
        std::cout << "Doc " << doc.id << " (" << doc.url.substr(8, 20) << "...): ";
        
        if (decision.result == FilterResult::KEEP) {
            std::cout << "KEEP - " << decision.details << "\n";
            strict_kept++;
        } else {
            std::cout << "REJECT - " << decision.details << "\n";
        }
    }
    
    std::cout << "Strict filter kept: " << strict_kept << "/" << test_documents.size() << " documents\n\n";
    
    // Test 2: Permissive Configuration
    std::cout << "2. PERMISSIVE CONFIGURATION (General Use)\n";
    std::cout << "   - Allows most open licenses\n";
    std::cout << "   - More lenient with unknown licenses\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto permissive_config = license_utils::create_permissive_config();
    LicenseFilter permissive_filter(permissive_config);
    
    size_t permissive_kept = 0;
    for (const auto& doc : test_documents) {
        auto decision = permissive_filter.evaluate(doc);
        std::cout << "Doc " << doc.id << " (" << doc.url.substr(8, 20) << "...): ";
        
        if (decision.result == FilterResult::KEEP) {
            std::cout << "KEEP - " << decision.details << "\n";
            permissive_kept++;
        } else {
            std::cout << "REJECT - " << decision.details << "\n";
        }
    }
    
    std::cout << "Permissive filter kept: " << permissive_kept << "/" << test_documents.size() << " documents\n\n";
    
    // Test 3: Custom Configuration with Domain Allowlist
    std::cout << "3. CUSTOM CONFIGURATION (Domain Allowlist)\n";
    std::cout << "   - Only allows specific trusted domains\n";
    std::cout << "   - Demonstrates opt-out handling\n";
    std::cout << std::string(50, '-') << "\n";
    
    LicenseFilterConfig custom_config;
    custom_config.allowed_domains = {
        "creativecommons.org",
        "archive.org", 
        "nasa.gov",
        "en.wikipedia.org",
        "ocw.mit.edu"
    };
    custom_config.strict_mode = false;
    custom_config.confidence_threshold = 0.6;
    
    LicenseFilter custom_filter(custom_config);
    
    // Add an opt-out request
    custom_filter.add_opt_out_request("majornews.com", "Publisher requested removal");
    
    size_t custom_kept = 0;
    for (const auto& doc : test_documents) {
        auto decision = custom_filter.evaluate(doc);
        std::cout << "Doc " << doc.id << " (" << doc.url.substr(8, 20) << "...): ";
        
        if (decision.result == FilterResult::KEEP) {
            std::cout << "KEEP - " << decision.details << "\n";
            custom_kept++;
        } else {
            std::cout << "REJECT - " << decision.details << "\n";
        }
    }
    
    std::cout << "Custom filter kept: " << custom_kept << "/" << test_documents.size() << " documents\n\n";
    
    // Test 4: License Assessment Detail
    std::cout << "4. DETAILED LICENSE ASSESSMENT\n";
    std::cout << "   - Shows full copyright analysis\n";
    std::cout << std::string(50, '-') << "\n";
    
    for (size_t i = 0; i < std::min(size_t(3), test_documents.size()); ++i) {
        const auto& doc = test_documents[i];
        
        std::cout << "\nDocument: " << doc.id << "\n";
        std::cout << "URL: " << doc.url << "\n";
        std::cout << "Text: " << doc.text.substr(0, 100) << "...\n";
        
        auto assessment = permissive_filter.assess_copyright(doc);
        
        std::cout << "License Analysis:\n";
        std::cout << "  Detected License: " << license_utils::license_type_to_string(assessment.detected_license) << "\n";
        std::cout << "  Has Copyright Notice: " << (assessment.has_copyright_notice ? "Yes" : "No") << "\n";
        std::cout << "  Is Paywalled: " << (assessment.is_paywalled ? "Yes" : "No") << "\n";
        std::cout << "  From Allowed Domain: " << (assessment.is_from_allowed_domain ? "Yes" : "No") << "\n";
        std::cout << "  Compliance Confidence: " << assessment.compliance_confidence << "\n";
        
        if (!assessment.license_indicators.empty()) {
            std::cout << "  License Indicators: ";
            for (const auto& indicator : assessment.license_indicators) {
                std::cout << indicator << " ";
            }
            std::cout << "\n";
        }
        
        if (!assessment.copyright_notices.empty()) {
            std::cout << "  Copyright Notices Found: " << assessment.copyright_notices.size() << "\n";
        }
    }
    
    // Show compliance statistics
    std::cout << "\n=== COMPLIANCE STATISTICS ===\n";
    
    std::cout << "\nStrict Filter Report:\n";
    strict_filter.print_compliance_report();
    
    std::cout << "\nPermissive Filter Report:\n";
    permissive_filter.print_compliance_report();
    
    std::cout << "\nCustom Filter Report:\n";
    custom_filter.print_compliance_report();
    
    // Demonstrate batch processing
    std::cout << "\n=== BATCH PROCESSING DEMO ===\n";
    
    auto batch_decisions = permissive_filter.evaluate_batch(test_documents);
    std::cout << "Batch processed " << batch_decisions.size() << " documents\n";
    
    for (size_t i = 0; i < batch_decisions.size(); ++i) {
        const auto& decision = batch_decisions[i];
        std::cout << "Batch result " << (i+1) << ": " 
                  << (decision.result == FilterResult::KEEP ? "KEEP" : "REJECT")
                  << " (confidence: " << decision.confidence << ")\n";
    }
    
    // Demonstrate domain management
    std::cout << "\n=== DOMAIN MANAGEMENT DEMO ===\n";
    
    LicenseFilter domain_filter;
    
    // Add some allowed domains
    domain_filter.add_allowed_domain("trusted-source.edu");
    domain_filter.add_allowed_domain("open-content.org");
    
    // Add some blocked domains
    domain_filter.add_blocked_domain("sketchy-site.com");
    domain_filter.add_blocked_domain("copyright-troll.net");
    
    std::cout << "Domain management configured successfully\n";
    
    // Test opt-out functionality
    std::cout << "\n=== OPT-OUT MANAGEMENT DEMO ===\n";
    
    LicenseFilter opt_out_filter;
    
    // Add opt-out requests
    opt_out_filter.add_opt_out_request("news-site.com", "Publisher opt-out request");
    opt_out_filter.add_opt_out_request("blog-platform.net", "User privacy request");
    
    // Test if domains are opted out
    std::cout << "news-site.com opted out: " << (opt_out_filter.is_opted_out("news-site.com") ? "Yes" : "No") << "\n";
    std::cout << "safe-site.org opted out: " << (opt_out_filter.is_opted_out("safe-site.org") ? "Yes" : "No") << "\n";
    
    std::cout << "\n=== BEST PRACTICES SUMMARY ===\n";
    std::cout << "1. Use strict configuration for research/academic datasets\n";
    std::cout << "2. Use permissive configuration for general web scraping\n";
    std::cout << "3. Maintain domain allowlists for trusted sources\n";
    std::cout << "4. Respect opt-out requests and removal notices\n";
    std::cout << "5. Regular compliance audits and license validation\n";
    std::cout << "6. Document your filtering criteria for transparency\n";
    
    return 0;
} 