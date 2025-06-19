#include "test_framework.hpp"
#include "rapidsift/text_extractor.hpp"
#include <iostream>
#include <sstream>
#include <vector>

// Template specialization for ContentType
namespace test_framework {
    template<>
    std::string to_string_safe<rapidsift::extraction_utils::ContentType>(const rapidsift::extraction_utils::ContentType& value) {
        switch(value) {
            case rapidsift::extraction_utils::ContentType::HTML: return "HTML";
            case rapidsift::extraction_utils::ContentType::XML: return "XML";
            case rapidsift::extraction_utils::ContentType::PLAIN_TEXT: return "PLAIN_TEXT";
            case rapidsift::extraction_utils::ContentType::JSON: return "JSON";
            case rapidsift::extraction_utils::ContentType::UNKNOWN: return "UNKNOWN";
            default: return "UNKNOWN";
        }
    }
}

using namespace rapidsift;

// Test HTML samples
const std::string SIMPLE_HTML = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
    <meta charset="utf-8">
</head>
<body>
    <h1>Main Heading</h1>
    <p>This is a paragraph with some content.</p>
    <p>Another paragraph with more text content here.</p>
</body>
</html>
)";

const std::string COMPLEX_HTML = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <title>Complex Test Page</title>
    <meta charset="utf-8">
    <meta name="description" content="A test page with complex structure">
    <script>console.log('test');</script>
    <style>body { margin: 0; }</style>
</head>
<body>
    <header>
        <nav class="navigation">
            <ul>
                <li><a href="/">Home</a></li>
                <li><a href="/about">About</a></li>
                <li><a href="/contact">Contact</a></li>
            </ul>
        </nav>
    </header>
    
    <main>
        <article class="content">
            <h1>Article Title</h1>
            <p>This is the main content paragraph with substantial text that should be extracted.</p>
            <p>Another content paragraph with more meaningful information about the topic.</p>
            <h2>Subsection</h2>
            <p>More detailed content in this subsection with additional information.</p>
        </article>
        
        <aside class="sidebar">
            <div class="ad">Advertisement content that should be filtered out.</div>
            <div class="widget">
                <h3>Related Links</h3>
                <ul>
                    <li><a href="/link1">Link 1</a></li>
                    <li><a href="/link2">Link 2</a></li>
                </ul>
            </div>
        </aside>
    </main>
    
    <footer>
        <p>&copy; 2024 Test Site. All rights reserved.</p>
        <p>Privacy Policy | Terms of Service</p>
    </footer>
</body>
</html>
)";

const std::string BOILERPLATE_HTML = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Boilerplate Heavy Page</title>
</head>
<body>
    <div class="cookie-banner">
        This site uses cookies. By continuing to browse, you agree to our cookie policy.
        <button>Accept</button>
    </div>
    
    <nav class="main-nav">
        <a href="/">Home</a> | <a href="/about">About</a> | <a href="/contact">Contact</a>
    </nav>
    
    <div class="content">
        <h1>Actual Content Title</h1>
        <p>This is the real content that should be extracted from the page.</p>
        <p>More meaningful content with substantial information for users.</p>
    </div>
    
    <div class="ads">
        <div class="advertisement">
            <h3>Special Offer!</h3>
            <p>Buy now and save 50%! Click here for more details.</p>
        </div>
    </div>
    
    <footer class="site-footer">
        <div class="social-links">
            Follow us on: <a href="#">Facebook</a> | <a href="#">Twitter</a>
        </div>
        <div class="copyright">
            &copy; 2024 Company Name. All rights reserved.
        </div>
    </footer>
</body>
</html>
)";

const std::string MALFORMED_HTML = R"(
<html>
<head>
<title>Malformed Page
<body>
<h1>Missing closing tags
<p>This paragraph has no closing tag
<p>Another paragraph
<div>Unclosed div
<span>Nested unclosed span
Text content mixed in
</body>
)";

void test_html_parser_basic() {
    std::cout << "Testing HTML parser basic functionality..." << std::endl;
    
    HtmlParser parser(SIMPLE_HTML);
    auto root = parser.parse();
    
    ASSERT_TRUE(root != nullptr);
    ASSERT_TRUE(!root->children.empty());
    
    // Find HTML element
    bool found_html = false;
    for (const auto& child : root->children) {
        if (child->tag == "html") {
            found_html = true;
            break;
        }
    }
    ASSERT_TRUE(found_html);
    
    std::cout << "✓ HTML parser basic functionality test passed" << std::endl;
}

void test_title_extraction() {
    std::cout << "Testing title extraction..." << std::endl;
    
    std::string title = HtmlParser::extract_title(SIMPLE_HTML);
    ASSERT_EQ(title, "Test Page");
    
    title = HtmlParser::extract_title(COMPLEX_HTML);
    ASSERT_EQ(title, "Complex Test Page");
    
    // Test missing title
    title = HtmlParser::extract_title("<html><body>No title</body></html>");
    ASSERT_TRUE(title.empty());
    
    std::cout << "✓ Title extraction test passed" << std::endl;
}

void test_meta_extraction() {
    std::cout << "Testing meta tag extraction..." << std::endl;
    
    std::string description = HtmlParser::extract_meta_content(COMPLEX_HTML, "description");
    ASSERT_EQ(description, "A test page with complex structure");
    
    // Test non-existent meta
    std::string keywords = HtmlParser::extract_meta_content(COMPLEX_HTML, "keywords");
    ASSERT_TRUE(keywords.empty());
    
    std::cout << "✓ Meta tag extraction test passed" << std::endl;
}

void test_encoding_detection() {
    std::cout << "Testing encoding detection..." << std::endl;
    
    std::string encoding = HtmlParser::detect_encoding(SIMPLE_HTML);
    ASSERT_EQ(encoding, "utf-8");
    
    // Test with different charset
    std::string iso_html = R"(<meta charset="iso-8859-1">)";
    encoding = HtmlParser::detect_encoding(iso_html);
    ASSERT_EQ(encoding, "iso-8859-1");
    
    std::cout << "✓ Encoding detection test passed" << std::endl;
}

void test_boilerplate_detection() {
    std::cout << "Testing boilerplate detection..." << std::endl;
    
    TextExtractionConfig config;
    BoilerplateRemover remover(config);
    
    // Create test elements
    auto nav_element = std::make_shared<HtmlElement>();
    nav_element->tag = "nav";
    nav_element->text = "Home | About | Contact";
    
    auto content_element = std::make_shared<HtmlElement>();
    content_element->tag = "p";
    content_element->text = "This is main content with substantial information.";
    
    auto ad_element = std::make_shared<HtmlElement>();
    ad_element->tag = "div";
    ad_element->attributes["class"] = "advertisement";
    ad_element->text = "Buy now! Special offer!";
    
    // Test boilerplate detection
    ASSERT_TRUE(remover.is_navigation_element(nav_element));
    ASSERT_FALSE(remover.is_navigation_element(content_element));
    ASSERT_TRUE(remover.is_advertisement_element(ad_element));
    ASSERT_FALSE(remover.is_advertisement_element(content_element));
    
    std::cout << "✓ Boilerplate detection test passed" << std::endl;
}

void test_text_cleaning() {
    std::cout << "Testing text cleaning..." << std::endl;
    
    TextExtractionConfig config;
    TextCleaner cleaner(config);
    
    // Test HTML entity decoding
    std::string html_entities = "Hello &amp; welcome to our site &copy; 2024";
    std::string cleaned = cleaner.decode_html_entities(html_entities);
    ASSERT_TRUE(cleaned.find("&") != std::string::npos);
    ASSERT_TRUE(cleaned.find("©") != std::string::npos);
    ASSERT_TRUE(cleaned.find("&amp;") == std::string::npos);
    
    // Test whitespace normalization
    std::string messy_whitespace = "  Multiple   spaces\t\tand\n\n\ntabs  ";
    cleaned = cleaner.normalize_whitespace(messy_whitespace);
    ASSERT_TRUE(cleaned.find("  ") == std::string::npos); // No double spaces
    
    // Test full cleaning
    std::string dirty_text = "&amp;  Multiple   spaces\n\n\nwith entities &copy;  ";
    cleaned = cleaner.clean_text(dirty_text);
    ASSERT_TRUE(cleaned.find("&amp;") == std::string::npos);
    ASSERT_TRUE(cleaned.find("&") != std::string::npos);
    
    std::cout << "✓ Text cleaning test passed" << std::endl;
}

void test_simple_extraction() {
    std::cout << "Testing simple text extraction..." << std::endl;
    
    TextExtractor extractor;
    auto result = extractor.extract(SIMPLE_HTML, "http://test.com");
    
    ASSERT_TRUE(result.is_valid());
    ASSERT_EQ(result.title, "Test Page");
    ASSERT_EQ(result.url, "http://test.com");
    ASSERT_TRUE(result.extracted_text.find("Main Heading") != std::string::npos);
    ASSERT_TRUE(result.extracted_text.find("This is a paragraph") != std::string::npos);
    ASSERT_TRUE(result.text_ratio > 0.0);
    
    std::cout << "✓ Simple text extraction test passed" << std::endl;
}

void test_complex_extraction() {
    std::cout << "Testing complex text extraction..." << std::endl;
    
    TextExtractionConfig config;
    config.remove_navigation = true;
    config.remove_ads = true;
    config.extract_main_content = true;
    
    TextExtractor extractor(config);
    auto result = extractor.extract(COMPLEX_HTML, "http://complex.com");
    
    ASSERT_TRUE(result.is_valid());
    ASSERT_EQ(result.title, "Complex Test Page");
    ASSERT_TRUE(result.extracted_text.find("Article Title") != std::string::npos);
    ASSERT_TRUE(result.extracted_text.find("main content paragraph") != std::string::npos);
    
    // Should filter out navigation and ads
    ASSERT_TRUE(result.extracted_text.find("Home") == std::string::npos ||
                result.extracted_text.find("About") == std::string::npos);
    ASSERT_TRUE(result.extracted_text.find("Advertisement content") == std::string::npos);
    
    std::cout << "✓ Complex text extraction test passed" << std::endl;
}

void test_boilerplate_filtering() {
    std::cout << "Testing boilerplate filtering..." << std::endl;
    
    TextExtractionConfig config;
    config.remove_navigation = true;
    config.remove_ads = true;
    config.remove_headers_footers = true;
    
    TextExtractor extractor(config);
    auto result = extractor.extract(BOILERPLATE_HTML);
    
    ASSERT_TRUE(result.is_valid());
    ASSERT_TRUE(result.extracted_text.find("Actual Content Title") != std::string::npos);
    ASSERT_TRUE(result.extracted_text.find("real content that should be extracted") != std::string::npos);
    
    // Should filter out boilerplate
    ASSERT_TRUE(result.extracted_text.find("cookie") == std::string::npos);
    ASSERT_TRUE(result.extracted_text.find("Special Offer") == std::string::npos);
    ASSERT_TRUE(result.extracted_text.find("All rights reserved") == std::string::npos);
    
    std::cout << "✓ Boilerplate filtering test passed" << std::endl;
}

void test_malformed_html_handling() {
    std::cout << "Testing malformed HTML handling..." << std::endl;
    
    TextExtractor extractor;
    auto result = extractor.extract(MALFORMED_HTML);
    
    // Should still extract some text even from malformed HTML
    ASSERT_TRUE(result.extracted_text_length > 0);
    ASSERT_TRUE(result.extracted_text.find("Missing closing tags") != std::string::npos);
    ASSERT_TRUE(result.extracted_text.find("Text content mixed in") != std::string::npos);
    
    std::cout << "✓ Malformed HTML handling test passed" << std::endl;
}

void test_quality_metrics() {
    std::cout << "Testing quality metrics calculation..." << std::endl;
    
    TextExtractor extractor;
    
    // High quality content
    auto good_result = extractor.extract(COMPLEX_HTML);
    double good_quality = good_result.quality_score();
    
    // Low quality content (mostly boilerplate)
    std::string poor_html = R"(
    <html><body>
    <nav>Home | About | Contact</nav>
    <div class="ad">Click here! Buy now!</div>
    <footer>&copy; 2024</footer>
    </body></html>
    )";
    
    auto poor_result = extractor.extract(poor_html);
    double poor_quality = poor_result.quality_score();
    
    ASSERT_TRUE(good_quality > poor_quality);
    ASSERT_TRUE(good_quality > 0.3); // Should be reasonably high
    ASSERT_TRUE(poor_quality < 0.7); // Should be lower
    
    std::cout << "✓ Quality metrics calculation test passed" << std::endl;
}

void test_batch_extraction() {
    std::cout << "Testing batch extraction..." << std::endl;
    
    std::vector<std::string> html_docs = {SIMPLE_HTML, COMPLEX_HTML, BOILERPLATE_HTML};
    std::vector<std::string> urls = {"http://simple.com", "http://complex.com", "http://boilerplate.com"};
    
    TextExtractor extractor;
    auto results = extractor.extract_batch(html_docs, urls);
    
    ASSERT_EQ(results.size(), 3);
    
    for (size_t i = 0; i < results.size(); ++i) {
        ASSERT_TRUE(results[i].is_valid());
        ASSERT_EQ(results[i].url, urls[i]);
        ASSERT_TRUE(results[i].extracted_text_length > 0);
    }
    
    std::cout << "✓ Batch extraction test passed" << std::endl;
}

void test_configuration_options() {
    std::cout << "Testing configuration options..." << std::endl;
    
    // Test with minimal cleaning
    TextExtractionConfig minimal_config;
    minimal_config.normalize_whitespace = false;
    minimal_config.decode_html_entities = false;
    minimal_config.remove_navigation = false;
    
    TextExtractor minimal_extractor(minimal_config);
    auto minimal_result = minimal_extractor.extract(COMPLEX_HTML);
    
    // Test with aggressive cleaning
    TextExtractionConfig aggressive_config;
    aggressive_config.remove_navigation = true;
    aggressive_config.remove_ads = true;
    aggressive_config.remove_headers_footers = true;
    aggressive_config.extract_main_content = true;
    aggressive_config.min_paragraph_length = 50;
    
    TextExtractor aggressive_extractor(aggressive_config);
    auto aggressive_result = aggressive_extractor.extract(COMPLEX_HTML);
    
    // Aggressive should produce cleaner, shorter text
    ASSERT_TRUE(aggressive_result.extracted_text_length <= minimal_result.extracted_text_length);
    
    std::cout << "✓ Configuration options test passed" << std::endl;
}

void test_utility_functions() {
    std::cout << "Testing utility functions..." << std::endl;
    
    // Test content type detection
    auto html_type = extraction_utils::detect_content_type(SIMPLE_HTML);
    ASSERT_EQ(html_type, extraction_utils::ContentType::HTML);
    
    auto text_type = extraction_utils::detect_content_type("Just plain text content");
    ASSERT_EQ(text_type, extraction_utils::ContentType::PLAIN_TEXT);
    
    auto json_type = extraction_utils::detect_content_type(R"({"key": "value"})");
    ASSERT_EQ(json_type, extraction_utils::ContentType::JSON);
    
    // Test URL utilities
    std::string domain = extraction_utils::extract_domain("https://example.com/path/to/page");
    ASSERT_EQ(domain, "example.com");
    
    std::string normalized = extraction_utils::normalize_url("HTTPS://EXAMPLE.COM/PATH/?utm_source=test");
    ASSERT_TRUE(normalized.find("utm_source") == std::string::npos);
    ASSERT_TRUE(normalized.find("https://example.com") != std::string::npos);
    
    // Test text quality assessment
    double high_quality = extraction_utils::calculate_text_quality(
        "This is a well-written article with proper sentences. It contains meaningful content "
        "that provides valuable information to readers. The text flows naturally and includes "
        "various topics and ideas that demonstrate good writing quality."
    );
    
    double low_quality = extraction_utils::calculate_text_quality("click here buy now sale");
    
    ASSERT_TRUE(high_quality > low_quality);
    
    std::cout << "✓ Utility functions test passed" << std::endl;
}

void test_edge_cases() {
    std::cout << "Testing edge cases..." << std::endl;
    
    TextExtractor extractor;
    
    // Empty HTML
    auto empty_result = extractor.extract("");
    ASSERT_FALSE(empty_result.is_valid());
    
    // HTML with only whitespace
    auto whitespace_result = extractor.extract("   \n\t   ");
    ASSERT_FALSE(whitespace_result.is_valid());
    
    // HTML with no text content
    auto no_text_result = extractor.extract("<html><head><script>alert('test');</script></head></html>");
    ASSERT_TRUE(no_text_result.extracted_text_length == 0 || !no_text_result.is_valid());
    
    // Very large HTML (simulate)
    std::string large_html = "<html><body>";
    for (int i = 0; i < 1000; ++i) {
        large_html += "<p>Paragraph " + std::to_string(i) + " with content.</p>";
    }
    large_html += "</body></html>";
    
    auto large_result = extractor.extract(large_html);
    ASSERT_TRUE(large_result.is_valid());
    ASSERT_TRUE(large_result.extracted_text_length > 10000);
    
    std::cout << "✓ Edge cases test passed" << std::endl;
}

void test_performance_benchmark() {
    std::cout << "Testing performance benchmark..." << std::endl;
    
    // Create test documents
    std::vector<std::string> test_docs;
    for (int i = 0; i < 100; ++i) {
        test_docs.push_back(COMPLEX_HTML);
    }
    
    TextExtractor extractor;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    auto results = extractor.extract_batch(test_docs);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double docs_per_second = (double(test_docs.size()) / duration.count()) * 1000.0;
    
    ASSERT_EQ(results.size(), test_docs.size());
    ASSERT_TRUE(docs_per_second > 10.0); // Should process at least 10 docs/second
    
    std::cout << "Performance: " << std::fixed << std::setprecision(1) 
              << docs_per_second << " documents/second" << std::endl;
    std::cout << "✓ Performance benchmark test passed" << std::endl;
}

int main() {
    std::cout << "Running Text Extractor Tests" << std::endl;
    std::cout << "============================" << std::endl;
    
    try {
        test_html_parser_basic();
        test_title_extraction();
        test_meta_extraction();
        test_encoding_detection();
        test_boilerplate_detection();
        test_text_cleaning();
        test_simple_extraction();
        test_complex_extraction();
        test_boilerplate_filtering();
        test_malformed_html_handling();
        test_quality_metrics();
        test_batch_extraction();
        test_configuration_options();
        test_utility_functions();
        test_edge_cases();
        test_performance_benchmark();
        
        std::cout << std::endl;
        std::cout << "✅ All text extractor tests passed!" << std::endl;
        std::cout << "Processed various HTML structures successfully:" << std::endl;
        std::cout << "  • Simple HTML documents" << std::endl;
        std::cout << "  • Complex pages with navigation and ads" << std::endl;
        std::cout << "  • Boilerplate-heavy content" << std::endl;
        std::cout << "  • Malformed HTML" << std::endl;
        std::cout << "  • Batch processing with quality metrics" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
} 