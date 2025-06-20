#include "rapidsift/model_quality_filter.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <regex>

namespace rapidsift {
namespace quality {

// Simple language model implementation for perplexity calculation
class LanguageModel {
public:
    virtual ~LanguageModel() = default;
    virtual double calculate_perplexity(const std::string& text) = 0;
    virtual bool load_model(const std::string& model_path) = 0;
    virtual bool is_loaded() const = 0;
};

// Simple n-gram language model for perplexity calculation
class NGramLanguageModel : public LanguageModel {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, double>> bigram_probs_;
    std::unordered_map<std::string, double> unigram_probs_;
    bool loaded_ = false;
    
public:
    bool load_model(const std::string& model_path) override {
        // For demo purposes, create a simple model
        // In practice, you'd load from a file or use a proper LM library
        loaded_ = true;
        return true;
    }
    
    bool is_loaded() const override { return loaded_; }
    
    double calculate_perplexity(const std::string& text) override {
        if (!loaded_) return 1000.0; // Very high perplexity for unloaded model
        
        // Simple approximation: higher perplexity for text with unusual patterns
        double perplexity = 10.0;
        
        // Add penalty for high digit ratio
        double digit_ratio = 0.0;
        for (char c : text) {
            if (std::isdigit(c)) digit_ratio++;
        }
        digit_ratio /= text.length();
        perplexity += digit_ratio * 50.0;
        
        // Add penalty for high symbol ratio
        double symbol_ratio = 0.0;
        for (char c : text) {
            if (!std::isalnum(c) && !std::isspace(c)) symbol_ratio++;
        }
        symbol_ratio /= text.length();
        perplexity += symbol_ratio * 30.0;
        
        // Add penalty for repetitive patterns
        std::string normalized = text;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        
        // Simple repetition detection
        size_t repetitions = 0;
        for (size_t i = 0; i < normalized.length() - 10; i++) {
            std::string substr = normalized.substr(i, 10);
            size_t pos = normalized.find(substr, i + 1);
            if (pos != std::string::npos) {
                repetitions++;
            }
        }
        perplexity += (repetitions / 10.0) * 20.0;
        
        return perplexity;
    }
};

// Simple quality classifier for FastText-style classification
class QualityClassifier {
public:
    virtual ~QualityClassifier() = default;
    virtual double classify(const std::string& text) = 0;
    virtual bool load_model(const std::string& model_path) = 0;
    virtual bool is_loaded() const = 0;
};

// Simple quality classifier implementation
class SimpleQualityClassifier : public QualityClassifier {
private:
    std::vector<std::string> quality_indicators_;
    std::vector<std::string> low_quality_indicators_;
    bool loaded_ = false;
    
public:
    bool load_model(const std::string& model_path) override {
        // Initialize with common quality indicators
        quality_indicators_ = {
            "analysis", "research", "study", "evidence", "conclusion",
            "methodology", "findings", "results", "discussion", "literature",
            "framework", "approach", "investigation", "experiment"
        };
        
        low_quality_indicators_ = {
            "click", "amazing", "shocking", "unbelievable", "secret",
            "trick", "hack", "instant", "easy", "simple", "quick",
            "guaranteed", "free", "win", "lose", "weight", "money"
        };
        
        loaded_ = true;
        return true;
    }
    
    bool is_loaded() const override { return loaded_; }
    
    double classify(const std::string& text) override {
        if (!loaded_) return 0.0;
        
        std::string lower_text = text;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
        
        double quality_score = 0.5; // Base score
        
        // Count quality indicators
        int quality_count = 0;
        for (const auto& indicator : quality_indicators_) {
            if (lower_text.find(indicator) != std::string::npos) {
                quality_count++;
            }
        }
        
        // Count low quality indicators
        int low_quality_count = 0;
        for (const auto& indicator : low_quality_indicators_) {
            if (lower_text.find(indicator) != std::string::npos) {
                low_quality_count++;
            }
        }
        
        // Adjust score based on indicators
        quality_score += (quality_count * 0.05) - (low_quality_count * 0.1);
        
        // Text length factor
        if (text.length() > 500) quality_score += 0.1;
        if (text.length() < 50) quality_score -= 0.2;
        
        // Sentence structure factor
        size_t sentence_count = std::count(text.begin(), text.end(), '.') + 
                               std::count(text.begin(), text.end(), '!') + 
                               std::count(text.begin(), text.end(), '?');
        if (sentence_count > 0) {
            double avg_sentence_length = text.length() / sentence_count;
            if (avg_sentence_length > 20 && avg_sentence_length < 200) {
                quality_score += 0.1;
            }
        }
        
        return std::clamp(quality_score, 0.0, 1.0);
    }
};

// PerplexityModel implementation
PerplexityModel::PerplexityModel(const std::string& model_path) {
    load_model(model_path);
}

PerplexityModel::~PerplexityModel() = default;

void PerplexityModel::load_model(const std::string& model_path) {
    language_model_ = std::make_unique<NGramLanguageModel>();
    model_loaded_ = language_model_->load_model(model_path);
}

void PerplexityModel::unload_model() {
    language_model_.reset();
    model_loaded_ = false;
}

QualityPrediction PerplexityModel::predict(const std::string& text) const {
    QualityPrediction prediction;
    prediction.model_name = get_model_name();
    
    if (!model_loaded_ || !language_model_) {
        prediction.quality_score = 0.0;
        prediction.confidence = 0.0;
        prediction.perplexity = 1000.0;
        return prediction;
    }
    
    double perplexity = language_model_->calculate_perplexity(text);
    prediction.perplexity = perplexity;
    prediction.quality_score = perplexity_to_quality_score(perplexity);
    prediction.confidence = 0.8; // Static confidence for demo
    
    return prediction;
}

std::vector<QualityPrediction> PerplexityModel::predict_batch(const std::vector<std::string>& texts) const {
    std::vector<QualityPrediction> predictions;
    predictions.reserve(texts.size());
    
    for (const auto& text : texts) {
        predictions.push_back(predict(text));
    }
    
    return predictions;
}

double PerplexityModel::calculate_perplexity(const std::string& text) const {
    if (!model_loaded_ || !language_model_) {
        return 1000.0;
    }
    return language_model_->calculate_perplexity(text);
}

std::vector<double> PerplexityModel::calculate_sliding_window_perplexity(
    const std::string& text, size_t window_size, size_t stride) const {
    
    std::vector<double> perplexities;
    
    if (!model_loaded_ || text.length() < window_size) {
        return perplexities;
    }
    
    for (size_t i = 0; i <= text.length() - window_size; i += stride) {
        std::string window = text.substr(i, window_size);
        double perplexity = calculate_perplexity(window);
        perplexities.push_back(perplexity);
    }
    
    return perplexities;
}

double PerplexityModel::perplexity_to_quality_score(double perplexity) const {
    // Convert perplexity to quality score (0-1)
    // Lower perplexity = higher quality
    double normalized = std::max(0.0, 1.0 - (perplexity / perplexity_config_.max_perplexity));
    return std::clamp(normalized, 0.0, 1.0);
}

// FastTextQualityModel implementation
FastTextQualityModel::FastTextQualityModel(const std::string& model_path) {
    load_model(model_path);
}

FastTextQualityModel::~FastTextQualityModel() = default;

void FastTextQualityModel::load_model(const std::string& model_path) {
    classifier_ = std::make_unique<SimpleQualityClassifier>();
    model_loaded_ = classifier_->load_model(model_path);
}

void FastTextQualityModel::unload_model() {
    classifier_.reset();
    model_loaded_ = false;
}

QualityPrediction FastTextQualityModel::predict(const std::string& text) const {
    QualityPrediction prediction;
    prediction.model_name = get_model_name();
    
    if (!model_loaded_ || !classifier_) {
        prediction.quality_score = 0.0;
        prediction.confidence = 0.0;
        return prediction;
    }
    
    std::string processed_text = preprocess_text(text);
    prediction.quality_score = classifier_->classify(processed_text);
    prediction.confidence = 0.7; // Static confidence for demo
    
    return prediction;
}

std::vector<QualityPrediction> FastTextQualityModel::predict_batch(const std::vector<std::string>& texts) const {
    std::vector<QualityPrediction> predictions;
    predictions.reserve(texts.size());
    
    for (const auto& text : texts) {
        predictions.push_back(predict(text));
    }
    
    return predictions;
}

std::string FastTextQualityModel::preprocess_text(const std::string& text) const {
    // Simple preprocessing: normalize whitespace and convert to lowercase
    std::string processed = text;
    
    // Remove extra whitespace
    std::regex whitespace_regex("\\s+");
    processed = std::regex_replace(processed, whitespace_regex, " ");
    
    // Convert to lowercase
    std::transform(processed.begin(), processed.end(), processed.begin(), ::tolower);
    
    return processed;
}

// BERTQualityModel implementation
BERTQualityModel::BERTQualityModel(const std::string& model_path) {
    load_model(model_path);
}

BERTQualityModel::~BERTQualityModel() = default;

void BERTQualityModel::load_model(const std::string& model_path) {
    // For demo purposes, use the same classifier as FastText
    // In practice, you'd load a proper BERT model
    classifier_ = std::make_unique<SimpleQualityClassifier>();
    model_loaded_ = classifier_->load_model(model_path);
}

void BERTQualityModel::unload_model() {
    classifier_.reset();
    model_loaded_ = false;
}

QualityPrediction BERTQualityModel::predict(const std::string& text) const {
    QualityPrediction prediction;
    prediction.model_name = get_model_name();
    
    if (!model_loaded_ || !classifier_) {
        prediction.quality_score = 0.0;
        prediction.confidence = 0.0;
        return prediction;
    }
    
    // BERT models typically have higher accuracy
    prediction.quality_score = classifier_->classify(text);
    prediction.confidence = 0.9; // Higher confidence for BERT
    
    return prediction;
}

std::vector<QualityPrediction> BERTQualityModel::predict_batch(const std::vector<std::string>& texts) const {
    std::vector<QualityPrediction> predictions;
    predictions.reserve(texts.size());
    
    // In practice, BERT would process in batches for efficiency
    for (const auto& text : texts) {
        predictions.push_back(predict(text));
    }
    
    return predictions;
}

// ModelQualityFilter implementation
ModelQualityFilter::ModelQualityFilter(const ModelFilterConfig& config) 
    : model_config_(config) {
    // Initialize statistics histogram
    stats_.score_histogram.resize(10, 0); // 10 bins for score distribution
}

FilterDecision ModelQualityFilter::evaluate(const Document& doc) const {
    if (model_config_.multistage_config.enabled) {
        return multi_stage_evaluate(doc);
    }
    
    // Single-stage evaluation
    QualityPrediction ensemble_pred = ensemble_predict(doc.text);
    
    FilterResult result = FilterResult::UNKNOWN;
    double threshold = 0.5; // Default threshold
    
    if (!models_.empty()) {
        // Use the first model's threshold for simplicity
        if (models_[0]->get_model_type() == ModelType::FASTTEXT_CLASSIFIER) {
            threshold = model_config_.fasttext_config.quality_threshold;
        } else if (models_[0]->get_model_type() == ModelType::BERT_CLASSIFIER) {
            threshold = model_config_.bert_config.quality_threshold;
        }
        
        result = score_to_filter_result(ensemble_pred.quality_score, threshold);
    }
    
    update_stats(ensemble_pred, result);
    
    FilterDecision decision;
    decision.result = result;
    decision.confidence = ensemble_pred.confidence;
    decision.details = "Model-based quality score: " + std::to_string(ensemble_pred.quality_score);
    decision.metrics["quality_score"] = ensemble_pred.quality_score;
    decision.metrics["perplexity"] = ensemble_pred.perplexity;
    
    return decision;
}

void ModelQualityFilter::configure(const QualityConfig& config) {
    // Extract model-specific settings from general config if needed
    // This is a placeholder - in practice you'd map QualityConfig to ModelFilterConfig
}

void ModelQualityFilter::set_model_config(const ModelFilterConfig& config) {
    model_config_ = config;
}

void ModelQualityFilter::add_model(std::unique_ptr<QualityModel> model) {
    if (model) {
        std::string name = model->get_model_name();
        model_name_to_index_[name] = models_.size();
        models_.push_back(std::move(model));
    }
}

void ModelQualityFilter::remove_model(const std::string& model_name) {
    auto it = model_name_to_index_.find(model_name);
    if (it != model_name_to_index_.end()) {
        size_t index = it->second;
        models_.erase(models_.begin() + index);
        model_name_to_index_.erase(it);
        
        // Update indices
        for (auto& [name, idx] : model_name_to_index_) {
            if (idx > index) {
                idx--;
            }
        }
    }
}

void ModelQualityFilter::load_all_models() {
    for (auto& model : models_) {
        if (!model->is_loaded()) {
            // Load with appropriate config
            if (model->get_model_type() == ModelType::PERPLEXITY) {
                model->load_model(model_config_.perplexity_config.model_path);
            } else if (model->get_model_type() == ModelType::FASTTEXT_CLASSIFIER) {
                model->load_model(model_config_.fasttext_config.model_path);
            } else if (model->get_model_type() == ModelType::BERT_CLASSIFIER) {
                model->load_model(model_config_.bert_config.model_path);
            }
        }
    }
}

void ModelQualityFilter::unload_all_models() {
    for (auto& model : models_) {
        model->unload_model();
    }
}

std::vector<std::string> ModelQualityFilter::get_loaded_models() const {
    std::vector<std::string> loaded_models;
    for (const auto& model : models_) {
        if (model->is_loaded()) {
            loaded_models.push_back(model->get_model_name());
        }
    }
    return loaded_models;
}

std::vector<QualityPrediction> ModelQualityFilter::predict_quality_batch(const std::vector<Document>& docs) const {
    std::vector<QualityPrediction> predictions;
    predictions.reserve(docs.size());
    
    for (const auto& doc : docs) {
        predictions.push_back(ensemble_predict(doc.text));
    }
    
    return predictions;
}

std::vector<FilterDecision> ModelQualityFilter::evaluate_batch(const std::vector<Document>& docs) const {
    std::vector<FilterDecision> decisions;
    decisions.reserve(docs.size());
    
    for (const auto& doc : docs) {
        decisions.push_back(evaluate(doc));
    }
    
    return decisions;
}

void ModelQualityFilter::reset_stats() {
    stats_ = ModelFilterStats();
    stats_.score_histogram.resize(10, 0);
}

void ModelQualityFilter::print_detailed_stats() const {
    std::cout << "Model Quality Filter Statistics:\n";
    std::cout << "================================\n";
    std::cout << "Total processed: " << stats_.total_processed << "\n";
    std::cout << "Kept: " << stats_.kept << " (" << (100.0 * stats_.kept / stats_.total_processed) << "%)\n";
    std::cout << "Rejected: " << stats_.rejected << " (" << (100.0 * stats_.rejected / stats_.total_processed) << "%)\n";
    std::cout << "Average quality score: " << stats_.get_average_quality_score() << "\n";
    std::cout << "Average perplexity: " << stats_.average_perplexity << "\n";
    
    std::cout << "\nPer-model statistics:\n";
    for (const auto& [model_name, count] : stats_.model_predictions) {
        std::cout << "  " << model_name << ": " << count << " predictions\n";
        if (stats_.average_scores.count(model_name)) {
            std::cout << "    Average score: " << stats_.average_scores.at(model_name) << "\n";
        }
        if (stats_.processing_times.count(model_name)) {
            std::cout << "    Avg processing time: " << stats_.processing_times.at(model_name) << " ms\n";
        }
    }
}

QualityPrediction ModelQualityFilter::ensemble_predict(const std::string& text) const {
    if (models_.empty()) {
        return QualityPrediction();
    }
    
    // Check cache first
    if (model_config_.cache_predictions) {
        std::string cache_key = get_cache_key(text, "ensemble");
        auto it = prediction_cache_.find(cache_key);
        if (it != prediction_cache_.end()) {
            return it->second;
        }
    }
    
    std::vector<QualityPrediction> predictions;
    predictions.reserve(models_.size());
    
    // Get predictions from all models
    for (const auto& model : models_) {
        if (model->is_loaded()) {
            predictions.push_back(model->predict(text));
        }
    }
    
    if (predictions.empty()) {
        return QualityPrediction();
    }
    
    // Combine predictions
    QualityPrediction ensemble_pred;
    ensemble_pred.model_name = "Ensemble";
    ensemble_pred.quality_score = combine_scores(predictions);
    
    // Average confidence
    double total_confidence = 0.0;
    for (const auto& pred : predictions) {
        total_confidence += pred.confidence;
    }
    ensemble_pred.confidence = total_confidence / predictions.size();
    
    // Use perplexity from perplexity model if available
    for (const auto& pred : predictions) {
        if (pred.perplexity > 0) {
            ensemble_pred.perplexity = pred.perplexity;
            break;
        }
    }
    
    // Cache the result
    if (model_config_.cache_predictions) {
        std::string cache_key = get_cache_key(text, "ensemble");
        prediction_cache_[cache_key] = ensemble_pred;
    }
    
    return ensemble_pred;
}

double ModelQualityFilter::combine_scores(const std::vector<QualityPrediction>& predictions) const {
    if (predictions.empty()) return 0.0;
    
    double weighted_sum = 0.0;
    double total_weight = 0.0;
    
    for (const auto& pred : predictions) {
        double weight = 1.0; // Default weight
        
        // Apply model-specific weights
        if (pred.model_name.find("Perplexity") != std::string::npos) {
            weight = model_config_.perplexity_weight;
        } else if (pred.model_name.find("FastText") != std::string::npos) {
            weight = model_config_.fasttext_weight;
        } else if (pred.model_name.find("BERT") != std::string::npos) {
            weight = model_config_.bert_weight;
        }
        
        // Weight by confidence
        weight *= pred.confidence;
        
        weighted_sum += pred.quality_score * weight;
        total_weight += weight;
    }
    
    return total_weight > 0 ? weighted_sum / total_weight : 0.0;
}

std::unordered_map<std::string, double> ModelQualityFilter::extract_linguistic_features(const std::string& text) const {
    std::unordered_map<std::string, double> features;
    
    if (!model_config_.extract_linguistic_features) {
        return features;
    }
    
    // Basic linguistic features
    features["word_count"] = std::count(text.begin(), text.end(), ' ') + 1;
    features["sentence_count"] = std::count(text.begin(), text.end(), '.') + 
                                std::count(text.begin(), text.end(), '!') + 
                                std::count(text.begin(), text.end(), '?');
    
    if (features["sentence_count"] > 0) {
        features["avg_sentence_length"] = features["word_count"] / features["sentence_count"];
    }
    
    // Character-level features
    features["char_count"] = text.length();
    features["alpha_ratio"] = 0.0;
    features["digit_ratio"] = 0.0;
    features["punct_ratio"] = 0.0;
    
    for (char c : text) {
        if (std::isalpha(c)) features["alpha_ratio"]++;
        else if (std::isdigit(c)) features["digit_ratio"]++;
        else if (std::ispunct(c)) features["punct_ratio"]++;
    }
    
    if (text.length() > 0) {
        features["alpha_ratio"] /= text.length();
        features["digit_ratio"] /= text.length();
        features["punct_ratio"] /= text.length();
    }
    
    return features;
}

std::unordered_map<std::string, double> ModelQualityFilter::extract_statistical_features(const std::string& text) const {
    std::unordered_map<std::string, double> features;
    
    if (!model_config_.extract_statistical_features) {
        return features;
    }
    
    // Text entropy
    std::unordered_map<char, int> char_counts;
    for (char c : text) {
        char_counts[c]++;
    }
    
    double entropy = 0.0;
    for (const auto& [c, count] : char_counts) {
        double prob = static_cast<double>(count) / text.length();
        entropy -= prob * std::log2(prob);
    }
    features["entropy"] = entropy;
    
    // Vocabulary richness (type-token ratio approximation)
    std::istringstream iss(text);
    std::string word;
    std::unordered_set<std::string> unique_words;
    int total_words = 0;
    
    while (iss >> word) {
        unique_words.insert(word);
        total_words++;
    }
    
    features["vocabulary_richness"] = total_words > 0 ? 
        static_cast<double>(unique_words.size()) / total_words : 0.0;
    
    return features;
}

FilterDecision ModelQualityFilter::multi_stage_evaluate(const Document& doc) const {
    const auto& stages = model_config_.multistage_config.stage_order;
    const auto& thresholds = model_config_.multistage_config.stage_thresholds;
    
    for (size_t i = 0; i < stages.size() && i < thresholds.size(); i++) {
        ModelType stage_type = stages[i];
        double threshold = thresholds[i];
        
        // Find model of the current stage type
        QualityPrediction prediction;
        for (const auto& model : models_) {
            if (model->get_model_type() == stage_type && model->is_loaded()) {
                prediction = model->predict(doc.text);
                break;
            }
        }
        
        // Check if we should short-circuit
        if (model_config_.multistage_config.short_circuit && 
            should_short_circuit(prediction, i)) {
            
            FilterDecision decision;
            decision.result = FilterResult::REJECT;
            decision.confidence = prediction.confidence;
            decision.details = "Rejected at stage " + std::to_string(i) + 
                              " (score: " + std::to_string(prediction.quality_score) + ")";
            decision.metrics["quality_score"] = prediction.quality_score;
            
            update_stats(prediction, FilterResult::REJECT);
            return decision;
        }
    }
    
    // If we made it through all stages, use ensemble prediction
    QualityPrediction final_pred = ensemble_predict(doc.text);
    FilterResult result = score_to_filter_result(final_pred.quality_score, 0.5);
    
    update_stats(final_pred, result);
    
    FilterDecision decision;
    decision.result = result;
    decision.confidence = final_pred.confidence;
    decision.details = "Multi-stage evaluation complete (score: " + std::to_string(final_pred.quality_score) + ")";
    decision.metrics["quality_score"] = final_pred.quality_score;
    
    return decision;
}

bool ModelQualityFilter::should_short_circuit(const QualityPrediction& prediction, size_t stage_index) const {
    if (stage_index >= model_config_.multistage_config.stage_thresholds.size()) {
        return false;
    }
    
    double threshold = model_config_.multistage_config.stage_thresholds[stage_index];
    return prediction.quality_score < threshold;
}

std::string ModelQualityFilter::get_cache_key(const std::string& text, const std::string& model_name) const {
    // Simple hash-based cache key
    std::hash<std::string> hasher;
    size_t text_hash = hasher(text);
    size_t model_hash = hasher(model_name);
    return std::to_string(text_hash) + "_" + std::to_string(model_hash);
}

void ModelQualityFilter::update_stats(const QualityPrediction& prediction, FilterResult result) const {
    stats_.total_processed++;
    
    if (result == FilterResult::KEEP) {
        stats_.kept++;
    } else if (result == FilterResult::REJECT) {
        stats_.rejected++;
    }
    
    // Update model-specific stats
    stats_.model_predictions[prediction.model_name]++;
    stats_.average_scores[prediction.model_name] = 
        (stats_.average_scores[prediction.model_name] * (stats_.model_predictions[prediction.model_name] - 1) + 
         prediction.quality_score) / stats_.model_predictions[prediction.model_name];
    
    // Update perplexity stats
    if (prediction.perplexity > 0) {
        stats_.average_perplexity = 
            (stats_.average_perplexity * (stats_.total_processed - 1) + prediction.perplexity) / stats_.total_processed;
        stats_.min_perplexity = std::min(stats_.min_perplexity, prediction.perplexity);
        stats_.max_perplexity = std::max(stats_.max_perplexity, prediction.perplexity);
        
        if (prediction.perplexity > model_config_.perplexity_config.max_perplexity) {
            stats_.high_perplexity_count++;
        }
    }
    
    // Update score histogram
    int bin = std::min(9, static_cast<int>(prediction.quality_score * 10));
    if (bin >= 0 && bin < static_cast<int>(stats_.score_histogram.size())) {
        stats_.score_histogram[bin]++;
    }
}

FilterResult ModelQualityFilter::score_to_filter_result(double quality_score, double threshold) const {
    if (quality_score >= threshold) {
        return FilterResult::KEEP;
    } else {
        return FilterResult::REJECT;
    }
}

// Utility functions implementation
namespace model_utils {

std::unique_ptr<QualityModel> create_model(ModelType type, const std::string& model_path) {
    switch (type) {
        case ModelType::PERPLEXITY:
            return std::make_unique<PerplexityModel>(model_path);
        case ModelType::FASTTEXT_CLASSIFIER:
            return std::make_unique<FastTextQualityModel>(model_path);
        case ModelType::BERT_CLASSIFIER:
            return std::make_unique<BERTQualityModel>(model_path);
        default:
            return nullptr;
    }
}

bool validate_model_path(const std::string& model_path, ModelType type) {
    // Simple validation - check if file exists
    std::ifstream file(model_path);
    return file.good();
}

std::string preprocess_for_perplexity(const std::string& text) {
    // Basic preprocessing for perplexity calculation
    std::string processed = text;
    
    // Remove excessive whitespace
    std::regex whitespace_regex("\\s+");
    processed = std::regex_replace(processed, whitespace_regex, " ");
    
    // Remove leading/trailing whitespace
    processed.erase(0, processed.find_first_not_of(" \t\n\r"));
    processed.erase(processed.find_last_not_of(" \t\n\r") + 1);
    
    return processed;
}

std::string preprocess_for_classification(const std::string& text) {
    std::string processed = text;
    
    // Convert to lowercase
    std::transform(processed.begin(), processed.end(), processed.begin(), ::tolower);
    
    // Remove extra whitespace
    std::regex whitespace_regex("\\s+");
    processed = std::regex_replace(processed, whitespace_regex, " ");
    
    return processed;
}

std::vector<std::string> tokenize_for_bert(const std::string& text, size_t max_length) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    
    while (iss >> token && tokens.size() < max_length - 2) { // Reserve space for special tokens
        tokens.push_back(token);
    }
    
    return tokens;
}

double calculate_text_entropy(const std::string& text) {
    std::unordered_map<char, int> char_counts;
    for (char c : text) {
        char_counts[c]++;
    }
    
    double entropy = 0.0;
    for (const auto& [c, count] : char_counts) {
        double prob = static_cast<double>(count) / text.length();
        entropy -= prob * std::log2(prob);
    }
    
    return entropy;
}

double calculate_vocabulary_richness(const std::string& text) {
    std::istringstream iss(text);
    std::string word;
    std::unordered_set<std::string> unique_words;
    int total_words = 0;
    
    while (iss >> word) {
        unique_words.insert(word);
        total_words++;
    }
    
    return total_words > 0 ? static_cast<double>(unique_words.size()) / total_words : 0.0;
}

double calculate_syntactic_complexity(const std::string& text) {
    // Simple approximation based on sentence structure
    size_t comma_count = std::count(text.begin(), text.end(), ',');
    size_t semicolon_count = std::count(text.begin(), text.end(), ';');
    size_t colon_count = std::count(text.begin(), text.end(), ':');
    size_t sentence_count = std::count(text.begin(), text.end(), '.') + 
                           std::count(text.begin(), text.end(), '!') + 
                           std::count(text.begin(), text.end(), '?');
    
    if (sentence_count == 0) return 0.0;
    
    return static_cast<double>(comma_count + semicolon_count + colon_count) / sentence_count;
}

double calculate_readability_score(const std::string& text) {
    // Simplified Flesch reading ease approximation
    size_t word_count = std::count(text.begin(), text.end(), ' ') + 1;
    size_t sentence_count = std::count(text.begin(), text.end(), '.') + 
                           std::count(text.begin(), text.end(), '!') + 
                           std::count(text.begin(), text.end(), '?');
    
    if (sentence_count == 0 || word_count == 0) return 0.0;
    
    double avg_sentence_length = static_cast<double>(word_count) / sentence_count;
    
    // Simplified score based on sentence length
    if (avg_sentence_length <= 8) return 0.9;
    else if (avg_sentence_length <= 15) return 0.7;
    else if (avg_sentence_length <= 25) return 0.5;
    else return 0.3;
}

void evaluate_model_performance(const QualityModel& model, 
                               const std::vector<Document>& test_docs,
                               const std::vector<bool>& ground_truth) {
    if (test_docs.size() != ground_truth.size()) {
        std::cerr << "Test documents and ground truth size mismatch\n";
        return;
    }
    
    size_t correct = 0;
    size_t total = test_docs.size();
    
    for (size_t i = 0; i < total; i++) {
        QualityPrediction pred = model.predict(test_docs[i].text);
        bool predicted_quality = pred.quality_score > 0.5;
        
        if (predicted_quality == ground_truth[i]) {
            correct++;
        }
    }
    
    double accuracy = static_cast<double>(correct) / total;
    std::cout << "Model " << model.get_model_name() << " accuracy: " << accuracy << "\n";
}

void benchmark_model_speed(const QualityModel& model, 
                          const std::vector<Document>& test_docs) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& doc : test_docs) {
        model.predict(doc.text);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double avg_time = static_cast<double>(duration.count()) / test_docs.size();
    std::cout << "Model " << model.get_model_name() << " average prediction time: " << avg_time << " ms\n";
}

ModelFilterConfig create_fast_config() {
    ModelFilterConfig config;
    
    // Fast configuration prioritizes speed over accuracy
    config.multistage_config.enabled = true;
    config.multistage_config.stage_order = {ModelType::FASTTEXT_CLASSIFIER};
    config.multistage_config.stage_thresholds = {0.3};
    config.multistage_config.short_circuit = true;
    
    config.fasttext_config.quality_threshold = 0.6;
    config.parallel_processing = true;
    config.batch_size = 64;
    config.cache_predictions = true;
    
    config.fasttext_weight = 1.0;
    config.extract_linguistic_features = false;
    config.extract_statistical_features = false;
    
    return config;
}

ModelFilterConfig create_accurate_config() {
    ModelFilterConfig config;
    
    // Accurate configuration prioritizes accuracy over speed
    config.multistage_config.enabled = true;
    config.multistage_config.stage_order = {ModelType::FASTTEXT_CLASSIFIER, ModelType::BERT_CLASSIFIER, ModelType::PERPLEXITY};
    config.multistage_config.stage_thresholds = {0.2, 0.4, 0.6};
    config.multistage_config.short_circuit = false;
    
    config.fasttext_config.quality_threshold = 0.8;
    config.bert_config.quality_threshold = 0.85;
    config.perplexity_config.max_perplexity = 30.0;
    
    config.parallel_processing = true;
    config.batch_size = 16;
    config.cache_predictions = true;
    
    config.fasttext_weight = 1.0;
    config.bert_weight = 2.0;
    config.perplexity_weight = 1.5;
    
    config.extract_linguistic_features = true;
    config.extract_statistical_features = true;
    config.extract_semantic_features = false;
    
    return config;
}

ModelFilterConfig create_balanced_config() {
    ModelFilterConfig config;
    
    // Balanced configuration between speed and accuracy
    config.multistage_config.enabled = true;
    config.multistage_config.stage_order = {ModelType::FASTTEXT_CLASSIFIER, ModelType::PERPLEXITY};
    config.multistage_config.stage_thresholds = {0.3, 0.7};
    config.multistage_config.short_circuit = true;
    
    config.fasttext_config.quality_threshold = 0.7;
    config.perplexity_config.max_perplexity = 40.0;
    
    config.parallel_processing = true;
    config.batch_size = 32;
    config.cache_predictions = true;
    
    config.fasttext_weight = 1.2;
    config.perplexity_weight = 1.0;
    
    config.extract_linguistic_features = true;
    config.extract_statistical_features = false;
    
    return config;
}

ModelFilterConfig load_config_from_file(const std::string& filename) {
    // Placeholder implementation - in practice you'd parse JSON/YAML
    ModelFilterConfig config;
    
    std::ifstream file(filename);
    if (file.is_open()) {
        // Parse configuration file
        // This is a simplified implementation
        std::string line;
        while (std::getline(file, line)) {
            // Parse each line and set config values
            // Implementation depends on chosen config format
        }
    }
    
    return config;
}

void save_config_to_file(const ModelFilterConfig& config, const std::string& filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        // Write configuration to file
        // This is a simplified implementation
        file << "# Model Filter Configuration\n";
        file << "perplexity_threshold=" << config.perplexity_config.max_perplexity << "\n";
        file << "fasttext_threshold=" << config.fasttext_config.quality_threshold << "\n";
        file << "bert_threshold=" << config.bert_config.quality_threshold << "\n";
        file << "parallel_processing=" << config.parallel_processing << "\n";
        file << "batch_size=" << config.batch_size << "\n";
    }
}

} // namespace model_utils

} // namespace quality
} // namespace rapidsift 