#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

#include "rapidsift/text_extractor.hpp"

using namespace rapidsift;

void print_section_header(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void print_extraction_result(const TextExtractionResult& result, const std::string& scenario) {
    std::cout << "\n--- " << scenario << " ---\n";
    std::cout << "Title: " << (result.title.empty() ? "N/A" : result.title) << "\n";
    std::cout << "Original HTML length: " << result.original_html_length << " chars\n";
    std::cout << "Extracted text length: " << result.extracted_text_length << " chars\n";
    std::cout << "Text ratio: " << std::fixed << std::setprecision(3) << result.text_ratio << "\n";
    std::cout << "Quality score: " << std::fixed << std::setprecision(3) << result.quality_score() << "\n";
    std::cout << "Valid extraction: " << (result.is_valid() ? "Yes" : "No") << "\n";
    
    if (!result.extracted_text.empty()) {
        std::string preview = result.extracted_text.substr(0, 200);
        if (result.extracted_text.length() > 200) {
            preview += "...";
        }
        std::cout << "Text preview: " << preview << "\n";
    }
    
    if (!result.headings.empty()) {
        std::cout << "Headings found: ";
        for (size_t i = 0; i < std::min(result.headings.size(), size_t(3)); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << "\"" << result.headings[i] << "\"";
        }
        if (result.headings.size() > 3) {
            std::cout << " (and " << (result.headings.size() - 3) << " more)";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "RapidSift Text Extraction Examples\n";
    std::cout << "==================================\n";
    
    // Example HTML documents
    
    // 1. Simple clean article
    std::string simple_article = R"(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <title>The Future of Artificial Intelligence</title>
        <meta charset="utf-8">
        <meta name="description" content="An exploration of AI developments">
    </head>
    <body>
        <article>
            <h1>The Future of Artificial Intelligence</h1>
            <p>Artificial Intelligence has been transforming industries at an unprecedented pace. 
               From healthcare to finance, AI systems are becoming increasingly sophisticated.</p>
            
            <h2>Machine Learning Advances</h2>
            <p>Recent breakthroughs in machine learning have enabled computers to process 
               natural language with remarkable accuracy. These developments are opening 
               new possibilities for human-computer interaction.</p>
            
            <h2>Ethical Considerations</h2>
            <p>As AI becomes more powerful, questions about ethics and responsibility become 
               increasingly important. We must ensure that these technologies benefit humanity 
               while minimizing potential risks.</p>
            
            <p>The future of AI promises exciting developments, but requires careful consideration 
               of its implications for society.</p>
        </article>
    </body>
    </html>
    )";
    
    // 2. Complex page with boilerplate
    std::string complex_page = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Tech News - Latest Updates</title>
        <script>
            // Analytics tracking
            console.log('Page loaded');
        </script>
        <style>
            .ad { background: yellow; }
            .nav { margin: 10px; }
        </style>
    </head>
    <body>
        <header>
            <nav class="nav">
                <a href="/">Home</a> | 
                <a href="/tech">Tech</a> | 
                <a href="/business">Business</a> | 
                <a href="/contact">Contact</a>
            </nav>
        </header>
        
        <div class="cookie-banner">
            This website uses cookies to improve your experience. 
            <button>Accept All Cookies</button>
        </div>
        
        <main>
            <article class="content">
                <h1>Revolutionary Quantum Computing Breakthrough</h1>
                <p>Scientists at leading research institutions have achieved a significant 
                   milestone in quantum computing development. This breakthrough could 
                   revolutionize how we approach complex computational problems.</p>
                
                <p>The new quantum processor demonstrates unprecedented stability and 
                   error correction capabilities. Researchers believe this technology 
                   could solve previously intractable problems in cryptography, 
                   drug discovery, and financial modeling.</p>
                
                <h2>Technical Details</h2>
                <p>The quantum system utilizes advanced superconducting qubits arranged 
                   in a novel architecture. This design significantly reduces quantum 
                   decoherence while maintaining computational power.</p>
            </article>
        </main>
        
        <aside class="sidebar">
            <div class="ad">
                <h3>Special Offer!</h3>
                <p>Get 50% off premium subscriptions! Click here to learn more.</p>
                <button>Subscribe Now</button>
            </div>
            
            <div class="related">
                <h3>Related Articles</h3>
                <ul>
                    <li><a href="/article1">AI in Healthcare</a></li>
                    <li><a href="/article2">Blockchain Trends</a></li>
                    <li><a href="/article3">5G Networks</a></li>
                </ul>
            </div>
        </aside>
        
        <footer>
            <p>&copy; 2024 TechNews Corp. All rights reserved.</p>
            <p><a href="/privacy">Privacy Policy</a> | <a href="/terms">Terms of Service</a></p>
            <div class="social">
                Follow us: <a href="#">Facebook</a> | <a href="#">Twitter</a> | <a href="#">LinkedIn</a>
            </div>
        </footer>
    </body>
    </html>
    )";
    
    // 3. E-commerce product page
    std::string ecommerce_page = R"(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Premium Wireless Headphones - TechStore</title>
    </head>
    <body>
        <nav class="main-nav">
            <a href="/">Home</a> | <a href="/electronics">Electronics</a> | 
            <a href="/audio">Audio</a> | <a href="/cart">Cart (0)</a>
        </nav>
        
        <div class="product-page">
            <h1>Premium Wireless Headphones</h1>
            
            <div class="product-details">
                <p>Experience exceptional sound quality with these premium wireless headphones. 
                   Featuring advanced noise cancellation technology and up to 30 hours of 
                   battery life.</p>
                
                <h2>Key Features</h2>
                <ul>
                    <li>Active noise cancellation</li>
                    <li>30-hour battery life</li>
                    <li>Premium leather padding</li>
                    <li>Bluetooth 5.0 connectivity</li>
                    <li>Quick charge capability</li>
                </ul>
                
                <h2>Product Specifications</h2>
                <p>These headphones deliver crisp highs and deep bass across the entire 
                   frequency spectrum. The ergonomic design ensures comfort during extended 
                   listening sessions.</p>
            </div>
            
            <div class="price-section">
                <span class="price">$299.99</span>
                <span class="old-price">$399.99</span>
                <button class="buy-now">Add to Cart</button>
            </div>
        </div>
        
        <div class="recommendations">
            <h3>You might also like:</h3>
            <div class="product-grid">
                <div class="product">Wireless Earbuds - $99</div>
                <div class="product">Gaming Headset - $199</div>
                <div class="product">Bluetooth Speaker - $149</div>
            </div>
        </div>
        
        <footer>
            <p>Free shipping on orders over $50. 30-day return policy.</p>
        </footer>
    </body>
    </html>
    )";
    
    // 4. News article with lots of boilerplate
    std::string news_article = R"(
    <html>
    <head>
        <title>Climate Change Summit Reaches Historic Agreement</title>
    </head>
    <body>
        <div class="breaking-news-banner">BREAKING NEWS</div>
        
        <nav>Home | World | Politics | Business | Sports | Weather</nav>
        
        <div class="social-share">
            Share: Facebook | Twitter | Email | Print
        </div>
        
        <article>
            <h1>Climate Change Summit Reaches Historic Agreement</h1>
            <p class="byline">By Environmental Reporter | Published 2 hours ago</p>
            
            <p>World leaders at the international climate summit have reached a groundbreaking 
               agreement on carbon emission reductions. The accord represents the most ambitious 
               climate action plan in decades.</p>
            
            <p>The agreement includes commitments from over 190 countries to achieve net-zero 
               emissions by 2050. Additionally, developed nations have pledged $100 billion 
               annually to support climate adaptation in developing countries.</p>
            
            <h2>Key Provisions</h2>
            <p>The landmark agreement establishes binding targets for renewable energy adoption 
               and fossil fuel reduction. Countries must submit detailed implementation plans 
               within six months.</p>
            
            <p>Environmental groups have praised the agreement as a crucial step forward, 
               though some critics argue the targets remain insufficient to prevent 
               catastrophic warming.</p>
        </article>
        
        <div class="ad-container">
            <div class="advertisement">
                <h3>Go Green Today!</h3>
                <p>Switch to renewable energy and save money. Get a free quote now!</p>
            </div>
        </div>
        
        <div class="newsletter-signup">
            <h3>Stay Informed</h3>
            <p>Subscribe to our newsletter for the latest environmental news.</p>
            <input type="email" placeholder="Enter your email">
            <button>Subscribe</button>
        </div>
        
        <footer>
            <p>© 2024 Global News Network. All rights reserved.</p>
            <p>Privacy Policy | Terms of Use | Contact Us | Advertise</p>
        </footer>
    </body>
    </html>
    )";
    
    // Example 1: Basic Text Extraction
    print_section_header("Example 1: Basic Text Extraction");
    {
        TextExtractor extractor;
        
        auto result1 = extractor.extract(simple_article, "https://example.com/ai-future");
        print_extraction_result(result1, "Simple Article");
        
        auto result2 = extractor.extract(complex_page, "https://technews.com/quantum");
        print_extraction_result(result2, "Complex Page (Default Settings)");
    }
    
    // Example 2: Aggressive Boilerplate Removal
    print_section_header("Example 2: Aggressive Boilerplate Removal");
    {
        TextExtractionConfig config;
        config.extract_main_content = true;
        config.remove_navigation = true;
        config.remove_ads = true;
        config.remove_headers_footers = true;
        config.remove_forms = true;
        config.min_paragraph_length = 30;
        config.preserve_headings = true;
        
        TextExtractor extractor(config);
        
        auto result = extractor.extract(complex_page, "https://technews.com/quantum");
        print_extraction_result(result, "Aggressive Cleaning");
    }
    
    // Example 3: E-commerce Content Extraction
    print_section_header("Example 3: E-commerce Content Extraction");
    {
        TextExtractionConfig config;
        config.preserve_lists = true;
        config.preserve_headings = true;
        config.min_paragraph_length = 20;
        config.remove_navigation = true;
        
        TextExtractor extractor(config);
        
        auto result = extractor.extract(ecommerce_page, "https://techstore.com/headphones");
        print_extraction_result(result, "Product Page");
    }
    
    // Example 4: News Article Processing
    print_section_header("Example 4: News Article Processing");
    {
        TextExtractionConfig config;
        config.extract_main_content = true;
        config.remove_ads = true;
        config.remove_navigation = true;
        config.preserve_headings = true;
        config.min_text_ratio = 0.2;
        
        TextExtractor extractor(config);
        
        auto result = extractor.extract(news_article, "https://globalnews.com/climate-summit");
        print_extraction_result(result, "News Article");
    }
    
    // Example 5: Batch Processing with Quality Filtering
    print_section_header("Example 5: Batch Processing with Quality Filtering");
    {
        std::vector<std::string> documents = {
            simple_article,
            complex_page,
            ecommerce_page,
            news_article
        };
        
        std::vector<std::string> urls = {
            "https://example.com/ai-future",
            "https://technews.com/quantum",
            "https://techstore.com/headphones",
            "https://globalnews.com/climate-summit"
        };
        
        TextExtractionConfig config;
        config.extract_main_content = true;
        config.extract_main_content = true;
        config.min_text_ratio = 0.1;
        
        TextExtractor extractor(config);
        
        auto results = extractor.extract_batch(documents, urls);
        
        std::cout << "\nBatch Processing Results:\n";
        std::cout << "========================\n";
        
        size_t high_quality = 0;
        size_t medium_quality = 0;
        size_t low_quality = 0;
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& result = results[i];
            double quality = result.quality_score();
            
            std::cout << "Document " << (i + 1) << ": ";
            std::cout << std::fixed << std::setprecision(3) << quality << " quality, ";
            std::cout << result.extracted_text_length << " chars";
            
            if (quality >= 0.7) {
                std::cout << " (HIGH)";
                high_quality++;
            } else if (quality >= 0.4) {
                std::cout << " (MEDIUM)";
                medium_quality++;
            } else {
                std::cout << " (LOW)";
                low_quality++;
            }
            std::cout << "\n";
        }
        
        std::cout << "\nQuality Distribution:\n";
        std::cout << "  High quality (≥0.7): " << high_quality << " documents\n";
        std::cout << "  Medium quality (≥0.4): " << medium_quality << " documents\n";
        std::cout << "  Low quality (<0.4): " << low_quality << " documents\n";
    }
    
    // Example 6: Utility Functions Demonstration
    print_section_header("Example 6: Utility Functions");
    {
        std::cout << "\nContent Type Detection:\n";
        std::cout << "Simple HTML: " << 
            static_cast<int>(extraction_utils::detect_content_type(simple_article)) << " (HTML=1)\n";
        std::cout << "Plain text: " << 
            static_cast<int>(extraction_utils::detect_content_type("Just plain text")) << " (TEXT=3)\n";
        std::cout << "JSON: " << 
            static_cast<int>(extraction_utils::detect_content_type("{\"key\": \"value\"}")) << " (JSON=2)\n";
        
        std::cout << "\nURL Processing:\n";
        std::string test_url = "https://EXAMPLE.COM/Path/To/Page?utm_source=test&ref=social#section";
        std::cout << "Original: " << test_url << "\n";
        std::cout << "Domain: " << extraction_utils::extract_domain(test_url) << "\n";
        std::cout << "Normalized: " << extraction_utils::normalize_url(test_url) << "\n";
        
        std::cout << "\nText Quality Assessment:\n";
        std::string high_quality_text = "This is a well-written article with proper grammar and structure. "
                                      "It contains meaningful information that provides value to readers. "
                                      "The sentences flow naturally and cover various important topics.";
        std::string low_quality_text = "click here buy now sale discount limited time offer";
        
        std::cout << "High quality text score: " << std::fixed << std::setprecision(3) 
                  << extraction_utils::calculate_text_quality(high_quality_text) << "\n";
        std::cout << "Low quality text score: " << std::fixed << std::setprecision(3) 
                  << extraction_utils::calculate_text_quality(low_quality_text) << "\n";
        
        std::cout << "\nBoilerplate Detection:\n";
        std::cout << "Navigation text: " << 
            (extraction_utils::is_navigation_text("Home About Contact Services") ? "Yes" : "No") << "\n";
        std::cout << "Advertisement text: " << 
            (extraction_utils::is_advertisement_text("Buy now! Special offer! Click here!") ? "Yes" : "No") << "\n";
        std::cout << "Copyright text: " << 
            (extraction_utils::is_copyright_text("© 2024 Company. All rights reserved.") ? "Yes" : "No") << "\n";
    }
    
    print_section_header("Summary");
    std::cout << "Text extraction examples completed successfully!\n\n";
    std::cout << "Key capabilities demonstrated:\n";
    std::cout << "✓ HTML parsing and text extraction\n";
    std::cout << "✓ Boilerplate detection and removal\n";
    std::cout << "✓ Content quality assessment\n";
    std::cout << "✓ Configurable extraction settings\n";
    std::cout << "✓ Batch processing with progress tracking\n";
    std::cout << "✓ Title and metadata extraction\n";
    std::cout << "✓ URL processing and normalization\n";
    std::cout << "✓ Text cleaning and normalization\n\n";
    
    std::cout << "This text extraction system is ready for processing web content,\n";
    std::cout << "removing boilerplate, and extracting high-quality natural language text\n";
    std::cout << "suitable for further processing with deduplication and language filtering.\n";
    
    return 0;
} 