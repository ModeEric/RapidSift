#pragma once

#include "quality_common.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace rapidsift {
namespace quality {

// Forward declarations
class LanguageModel;
class QualityClassifier;

// Model types for quality assessment
enum class ModelType {
    PERPLEXITY,
    FASTTEXT_CLASSIFIER,
    BERT_CLASSIFIER, 
    TRANSFORMER_LM,
    CUSTOM
};

// Quality prediction result
struct QualityPrediction {
    double quality_score;  // 0.0 (low quality) to 1.0 (high quality)
    double confidence;     // Model confidence in prediction
    double perplexity;     // Language model perplexity (if applicable)
    std::string model_name;
    std::unordered_map<std::string, double> feature_scores;
    
    QualityPrediction() : quality_score(0.0), confidence(0.0), perplexity(0.0) {}
};

// Configuration for model-based filtering
struct ModelFilterConfig {
    // Perplexity filtering
    struct PerplexityConfig {
        std::string model_path;
        double max_perplexity = 50.0;      // Threshold for rejection
        double warning_perplexity = 30.0;  // Threshold for warning
        size_t max_sequence_length = 512;
        bool normalize_by_length = true;
        bool use_sliding_window = false;
        size_t window_size = 256;
        size_t window_stride = 128;
    } perplexity_config;
    
    // FastText classifier configuration  
    struct FastTextConfig {
        std::string model_path;
        double quality_threshold = 0.7;    // Minimum quality score to keep
        size_t max_ngram_length = 3;
        double min_confidence = 0.5;
        bool use_subwords = true;
        std::vector<std::string> quality_labels = {"__label__high_quality"};
    } fasttext_config;
    
    // BERT-based classifier configuration
    struct BERTConfig {
        std::string model_path;
        std::string tokenizer_path;
        double quality_threshold = 0.8;
        size_t max_sequence_length = 512;
        size_t batch_size = 8;
        bool use_gpu = false;
        std::string device = "cpu";
    } bert_config;
    
    // Multi-stage filtering configuration
    struct MultiStageConfig {
        bool enabled = true;
        std::vector<ModelType> stage_order = {ModelType::FASTTEXT_CLASSIFIER, ModelType::PERPLEXITY};
        std::vector<double> stage_thresholds = {0.3, 0.7};  // Early rejection thresholds
        bool short_circuit = true;  // Stop processing if early stage rejects
    } multistage_config;
    
    // General model settings
    bool parallel_processing = true;
    size_t batch_size = 32;
    size_t num_threads = 0;  // 0 = auto-detect
    bool cache_predictions = true;
    std::string cache_directory = "./model_cache";
    
    // Scoring weights for ensemble
    double perplexity_weight = 1.0;
    double fasttext_weight = 1.5;
    double bert_weight = 2.0;
    
    // Feature extraction
    bool extract_linguistic_features = true;
    bool extract_statistical_features = true;
    bool extract_semantic_features = false;  // Requires embeddings
};

// Statistics for model-based filtering
struct ModelFilterStats {
    size_t total_processed = 0;
    size_t kept = 0;
    size_t rejected = 0;
    
    // Per-model statistics
    std::unordered_map<std::string, size_t> model_predictions;
    std::unordered_map<std::string, double> average_scores;
    std::unordered_map<std::string, double> processing_times;
    
    // Perplexity statistics
    double average_perplexity = 0.0;
    double min_perplexity = std::numeric_limits<double>::max();
    double max_perplexity = 0.0;
    size_t high_perplexity_count = 0;
    
    // Quality score distribution
    std::vector<size_t> score_histogram;  // Binned quality scores
    
    double get_rejection_rate() const {
        return total_processed > 0 ? static_cast<double>(rejected) / total_processed : 0.0;
    }
    
    double get_average_quality_score() const {
        double total = 0.0;
        size_t count = 0;
        for (const auto& [model, score] : average_scores) {
            total += score;
            count++;
        }
        return count > 0 ? total / count : 0.0;
    }
};

// Base class for quality models
class QualityModel {
public:
    virtual ~QualityModel() = default;
    virtual QualityPrediction predict(const std::string& text) const = 0;
    virtual std::vector<QualityPrediction> predict_batch(const std::vector<std::string>& texts) const = 0;
    virtual std::string get_model_name() const = 0;
    virtual ModelType get_model_type() const = 0;
    virtual bool is_loaded() const = 0;
    virtual void load_model(const std::string& model_path) = 0;
    virtual void unload_model() = 0;
};

// Perplexity-based language model
class PerplexityModel : public QualityModel {
public:
    PerplexityModel() = default;
    explicit PerplexityModel(const std::string& model_path);
    ~PerplexityModel() override;
    
    QualityPrediction predict(const std::string& text) const override;
    std::vector<QualityPrediction> predict_batch(const std::vector<std::string>& texts) const override;
    std::string get_model_name() const override { return "PerplexityModel"; }
    ModelType get_model_type() const override { return ModelType::PERPLEXITY; }
    bool is_loaded() const override { return model_loaded_; }
    void load_model(const std::string& model_path) override;
    void unload_model() override;
    
    // Perplexity-specific methods
    double calculate_perplexity(const std::string& text) const;
    std::vector<double> calculate_sliding_window_perplexity(const std::string& text, 
                                                           size_t window_size, 
                                                           size_t stride) const;
    void set_config(const ModelFilterConfig::PerplexityConfig& config) { perplexity_config_ = config; }

private:
    std::unique_ptr<LanguageModel> language_model_;
    ModelFilterConfig::PerplexityConfig perplexity_config_;
    bool model_loaded_ = false;
    
    double perplexity_to_quality_score(double perplexity) const;
};

// FastText-based quality classifier
class FastTextQualityModel : public QualityModel {
public:
    FastTextQualityModel() = default;
    explicit FastTextQualityModel(const std::string& model_path);
    ~FastTextQualityModel() override;
    
    QualityPrediction predict(const std::string& text) const override;
    std::vector<QualityPrediction> predict_batch(const std::vector<std::string>& texts) const override;
    std::string get_model_name() const override { return "FastTextQualityModel"; }
    ModelType get_model_type() const override { return ModelType::FASTTEXT_CLASSIFIER; }
    bool is_loaded() const override { return model_loaded_; }
    void load_model(const std::string& model_path) override;
    void unload_model() override;
    
    void set_config(const ModelFilterConfig::FastTextConfig& config) { fasttext_config_ = config; }

private:
    std::unique_ptr<QualityClassifier> classifier_;
    ModelFilterConfig::FastTextConfig fasttext_config_;
    bool model_loaded_ = false;
    
    std::string preprocess_text(const std::string& text) const;
};

// BERT-based quality classifier  
class BERTQualityModel : public QualityModel {
public:
    BERTQualityModel() = default;
    explicit BERTQualityModel(const std::string& model_path);
    ~BERTQualityModel() override;
    
    QualityPrediction predict(const std::string& text) const override;
    std::vector<QualityPrediction> predict_batch(const std::vector<std::string>& texts) const override;
    std::string get_model_name() const override { return "BERTQualityModel"; }
    ModelType get_model_type() const override { return ModelType::BERT_CLASSIFIER; }
    bool is_loaded() const override { return model_loaded_; }
    void load_model(const std::string& model_path) override;
    void unload_model() override;
    
    void set_config(const ModelFilterConfig::BERTConfig& config) { bert_config_ = config; }

private:
    std::unique_ptr<QualityClassifier> classifier_;
    ModelFilterConfig::BERTConfig bert_config_;
    bool model_loaded_ = false;
};

// Main model-based quality filter
class ModelQualityFilter : public QualityFilter {
public:
    ModelQualityFilter() = default;
    explicit ModelQualityFilter(const ModelFilterConfig& config);
    ~ModelQualityFilter() override = default;
    
    FilterDecision evaluate(const Document& doc) const override;
    std::string get_name() const override { return "ModelQualityFilter"; }
    void configure(const QualityConfig& config) override;
    
    // Model-specific interface
    void set_model_config(const ModelFilterConfig& config);
    const ModelFilterConfig& get_model_config() const { return model_config_; }
    
    // Model management
    void add_model(std::unique_ptr<QualityModel> model);
    void remove_model(const std::string& model_name);
    void load_all_models();
    void unload_all_models();
    std::vector<std::string> get_loaded_models() const;
    
    // Batch processing
    std::vector<QualityPrediction> predict_quality_batch(const std::vector<Document>& docs) const;
    std::vector<FilterDecision> evaluate_batch(const std::vector<Document>& docs) const;
    
    // Statistics and analysis
    const ModelFilterStats& get_stats() const { return stats_; }
    void reset_stats();
    void print_detailed_stats() const;
    
    // Ensemble methods
    QualityPrediction ensemble_predict(const std::string& text) const;
    double combine_scores(const std::vector<QualityPrediction>& predictions) const;
    
    // Feature extraction
    std::unordered_map<std::string, double> extract_linguistic_features(const std::string& text) const;
    std::unordered_map<std::string, double> extract_statistical_features(const std::string& text) const;
    
private:
    ModelFilterConfig model_config_;
    mutable ModelFilterStats stats_;
    
    std::vector<std::unique_ptr<QualityModel>> models_;
    std::unordered_map<std::string, size_t> model_name_to_index_;
    
    // Multi-stage filtering
    FilterDecision multi_stage_evaluate(const Document& doc) const;
    bool should_short_circuit(const QualityPrediction& prediction, size_t stage_index) const;
    
    // Caching
    mutable std::unordered_map<std::string, QualityPrediction> prediction_cache_;
    std::string get_cache_key(const std::string& text, const std::string& model_name) const;
    
    void update_stats(const QualityPrediction& prediction, FilterResult result) const;
    FilterResult score_to_filter_result(double quality_score, double threshold) const;
};

// Utility functions for model-based filtering
namespace model_utils {
    // Model loading utilities
    std::unique_ptr<QualityModel> create_model(ModelType type, const std::string& model_path);
    bool validate_model_path(const std::string& model_path, ModelType type);
    
    // Preprocessing utilities
    std::string preprocess_for_perplexity(const std::string& text);
    std::string preprocess_for_classification(const std::string& text);
    std::vector<std::string> tokenize_for_bert(const std::string& text, size_t max_length);
    
    // Feature extraction utilities
    double calculate_text_entropy(const std::string& text);
    double calculate_vocabulary_richness(const std::string& text);
    double calculate_syntactic_complexity(const std::string& text);
    double calculate_readability_score(const std::string& text);
    
    // Evaluation utilities
    void evaluate_model_performance(const QualityModel& model, 
                                  const std::vector<Document>& test_docs,
                                  const std::vector<bool>& ground_truth);
    void benchmark_model_speed(const QualityModel& model, 
                              const std::vector<Document>& test_docs);
    
    // Configuration utilities
    ModelFilterConfig create_fast_config();
    ModelFilterConfig create_accurate_config(); 
    ModelFilterConfig create_balanced_config();
    ModelFilterConfig load_config_from_file(const std::string& filename);
    void save_config_to_file(const ModelFilterConfig& config, const std::string& filename);
}

} // namespace quality
} // namespace rapidsift 