# Model-Based Quality Filtering

This document describes the model-based quality filtering system in RapidSift, which implements advanced techniques for assessing text quality using machine learning models.

## Overview

Model-based quality filtering goes beyond simple heuristic rules to use learned models for evaluating text quality. This approach can capture complex quality signals that fixed rules miss, such as fluency, topicality, coherence, and factuality.

## Supported Approaches

### 1. Perplexity Filtering
Uses pre-trained language models to calculate perplexity scores for text.

### 2. FastText-Style Classification
Lightweight bag-of-ngrams classifiers for quick quality assessment.

### 3. BERT-Based Classification
Transformer-based classifiers for high-accuracy quality predictions.

### 4. Multi-Stage Filtering
Combines multiple models in a cascade for optimal efficiency and accuracy.

## Usage Example

```cpp
#include "rapidsift/model_quality_filter.hpp"

// Create filter
auto config = model_utils::create_balanced_config();
ModelQualityFilter filter(config);

// Add and load models
filter.add_model(model_utils::create_model(
    ModelType::FASTTEXT_CLASSIFIER, "path/to/model.bin"));
filter.load_all_models();

// Filter document
Document doc("doc1", "High-quality research text...");
auto decision = filter.evaluate(doc);
```

## Best Practices

1. Start with FastText models for speed
2. Validate thresholds on your data
3. Monitor performance metrics
4. Use domain-specific training data
5. Retrain models regularly
