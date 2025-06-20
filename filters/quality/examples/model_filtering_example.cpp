#include "rapidsift/model_quality_filter.hpp"
#include "rapidsift/quality_common.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>
#include <chrono>

using namespace rapidsift::quality;

int main() {
    std::cout << "RapidSift Model-Based Quality Filtering Example\n";
    std::cout << "===============================================\n\n";
    
    // Create test documents with varying quality levels
    std::vector<Document> test_documents = {
        // High-quality academic text
        Document("doc1", 
            "This research investigates the methodology used in natural language processing "
            "applications. The study provides evidence for the effectiveness of transformer "
            "architectures in various linguistic tasks. Our findings suggest that attention "
            "mechanisms significantly improve model performance across multiple domains.", 
            "https://academic-journal.org/paper1"),
        
        // Low-quality clickbait text
        Document("doc2", 
            "You won't believe this amazing trick! Doctors hate this simple secret that "
            "will make you lose weight instantly! Click here now for shocking results! "
            "This easy method guarantees instant success with no effort required!",
            "https://clickbait-site.com/amazing-trick"),
        
        // Medium-quality informational text
        Document("doc3",
            "Python is a popular programming language. It is used for web development, "
            "data analysis, and machine learning. Many companies use Python for their "
            "software projects. It has a simple syntax that makes it easy to learn.",
            "https://programming-tutorial.com/python-intro"),
        
        // Low-quality repetitive text
        Document("doc4",
            "The quick brown fox jumps over the lazy dog. The quick brown fox jumps "
            "over the lazy dog. The quick brown fox jumps over the lazy dog.",
            "https://random-text.net/repetitive"),
        
        // High-quality technical documentation
        Document("doc5",
            "The implementation utilizes advanced algorithms for efficient data processing. "
            "Performance optimization techniques include memory pooling, vectorization, "
            "and parallel computing strategies. The architecture follows established design "
            "patterns for scalability and maintainability.",
            "https://tech-docs.company.com/architecture")
    };
    
    std::cout << "Creating model-based quality filter configurations...\n\n";
    
    // 1. Fast configuration (FastText only)
    std::cout << "=== Fast Configuration Demo ===\n";
    auto fast_config = model_utils::create_fast_config();
    ModelQualityFilter fast_filter(fast_config);
    
    // Add FastText model
    fast_filter.add_model(model_utils::create_model(ModelType::FASTTEXT_CLASSIFIER, "models/quality_fasttext.bin"));
    fast_filter.load_all_models();
    
    std::cout << "Loaded models: ";
    for (const auto& model_name : fast_filter.get_loaded_models()) {
        std::cout << model_name << " ";
    }
    std::cout << "\n\n";
    
    std::cout << "Processing documents with fast configuration:\n";
    for (size_t i = 0; i < test_documents.size(); ++i) {
        const auto& doc = test_documents[i];
        auto decision = fast_filter.evaluate(doc);
        
        std::cout << "Doc " << (i + 1) << " (" << doc.id << "): ";
        std::cout << (decision.result == FilterResult::KEEP ? "KEEP" : "REJECT");
        std::cout << " (score: " << std::fixed << std::setprecision(3) 
                  << decision.metrics.at("quality_score") << ")\n";
    }
    
    // 2. Demonstrate batch processing
    std::cout << "\n=== Batch Processing Demo ===\n";
    auto batch_predictions = fast_filter.predict_quality_batch(test_documents);
    
    std::cout << "Batch quality predictions:\n";
    for (size_t i = 0; i < batch_predictions.size(); ++i) {
        const auto& pred = batch_predictions[i];
        std::cout << "Doc " << (i + 1) << ": "
                  << "Quality=" << std::fixed << std::setprecision(3) << pred.quality_score
                  << ", Confidence=" << std::fixed << std::setprecision(3) << pred.confidence
                  << " (" << pred.model_name << ")\n";
    }
    
    // 3. Demonstrate feature extraction
    std::cout << "\n=== Feature Extraction Demo ===\n";
    const auto& sample_doc = test_documents[0]; // High-quality academic text
    
    auto linguistic_features = fast_filter.extract_linguistic_features(sample_doc.text);
    auto statistical_features = fast_filter.extract_statistical_features(sample_doc.text);
    
    std::cout << "Features for high-quality document:\n";
    std::cout << "Linguistic features:\n";
    for (const auto& [feature, value] : linguistic_features) {
        std::cout << "  " << feature << ": " << std::fixed << std::setprecision(3) << value << "\n";
    }
    
    std::cout << "Statistical features:\n";
    for (const auto& [feature, value] : statistical_features) {
        std::cout << "  " << feature << ": " << std::fixed << std::setprecision(3) << value << "\n";
    }
    
    std::cout << "\nModel-based quality filtering demo completed!\n";
    std::cout << "\nKey Features Demonstrated:\n";
    std::cout << "- FastText-style quality classification\n";
    std::cout << "- Multi-stage filtering capabilities\n";
    std::cout << "- Ensemble model predictions\n";
    std::cout << "- Feature extraction for quality assessment\n";
    std::cout << "- Batch processing capabilities\n";
    
    return 0;
} 