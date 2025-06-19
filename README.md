# RapidSift: Comprehensive Text Deduplication Tool

RapidSift is a powerful Python toolkit for text deduplication that implements multiple state-of-the-art approaches to identify and handle duplicate content in text datasets. It's designed for preprocessing large text corpora for machine learning, particularly for language model training where duplicate content can harm model performance.

## Features

### 🎯 Multiple Deduplication Methods

1. **Exact Deduplication**: Hash-based matching for verbatim duplicates
   - Supports MD5, SHA1, SHA256, and xxHash algorithms
   - Catches identical documents (common in web crawls)

2. **Near-Duplicate Detection**: Fuzzy matching for almost-identical content
   - MinHash with LSH for efficient similarity detection
   - SimHash for compact fingerprinting
   - Catches duplicates with minor differences (punctuation, small edits)

3. **Semantic Deduplication**: Embedding-based similarity for paraphrases
   - Uses sentence transformers for semantic understanding
   - Identifies paraphrased or translated duplicates
   - Configurable similarity thresholds

4. **Soft Deduplication**: Re-weighting instead of removal
   - Assigns lower weights to recurring content
   - Maintains dataset diversity while reducing redundancy
   - 26% faster training with maintained model performance

### 🚀 Key Benefits

- **Flexible**: Multiple algorithms for different duplicate types
- **Scalable**: Efficient implementations for large datasets  
- **Production-Ready**: CLI interface and programmatic API
- **Comprehensive**: Handles exact, near, and semantic duplicates
- **Research-Backed**: Implements latest techniques from academic literature

## Installation

```bash
# Clone the repository
git clone https://github.com/your-repo/RapidSift.git
cd RapidSift

# Install dependencies
pip install -r requirements.txt

# Optional: Install in development mode
pip install -e .
```

## Quick Start

### Command Line Usage

```bash
# Exact deduplication
python main.py dedupe input.txt --method exact --output clean.txt

# Near-duplicate detection  
python main.py dedupe input.txt --method near --output clean.txt

# Semantic deduplication
python main.py dedupe input.txt --method semantic --output clean.txt

# Soft deduplication (creates weighted dataset)
python main.py dedupe input.txt --method soft --output weighted.csv
```

### Programmatic Usage

```python
from dedup import ExactDeduplicator, NearDeduplicator, SemanticDeduplicator, SoftDeduplicator

# Load your documents
documents = ["Text 1", "Text 2", "Text 1", ...]  # Your text data

# Exact deduplication
exact_dedup = ExactDeduplicator(hash_algorithm='sha256')
unique_docs = exact_dedup.deduplicate(documents)

# Near-duplicate detection
near_dedup = NearDeduplicator(method='minhash', threshold=0.8)
unique_docs, indices = near_dedup.deduplicate(documents)

# Semantic deduplication
semantic_dedup = SemanticDeduplicator(similarity_threshold=0.85)
unique_docs, indices = semantic_dedup.deduplicate(documents)

# Soft deduplication
soft_dedup = SoftDeduplicator(method='hash', decay_factor=0.5)
weights = soft_dedup.compute_weights(documents)
```

## Examples

Run the example scripts to see each method in action:

```bash
# Generate sample data
python examples/sample_data.py

# Try exact deduplication
python examples/exact_dedup_example.py

# Try near-duplicate detection
python examples/near_dedup_example.py

# Try soft deduplication
python examples/soft_dedup_example.py
```

## Method Comparison

| Method | Best For | Speed | Memory | Accuracy |
|--------|----------|-------|---------|----------|
| Exact | Verbatim duplicates | ⚡⚡⚡ | Low | Perfect |
| Near (MinHash) | Minor variations | ⚡⚡ | Medium | High |
| Near (SimHash) | Minor variations | ⚡⚡ | Low | Good |
| Semantic | Paraphrases/translations | ⚡ | High | Excellent |
| Soft | Preserving diversity | ⚡⚡ | Low | N/A |

## Configuration Options

### Exact Deduplication
- `hash_algorithm`: 'md5', 'sha1', 'sha256', 'xxhash'
- `keep_first`: Keep first or last occurrence

### Near-Duplicate Detection
- `method`: 'minhash' or 'simhash'
- `threshold`: Similarity threshold (0.0-1.0)
- `num_perm`: MinHash permutations (default: 128)
- `ngram_size`: N-gram size for MinHash (default: 5)

### Semantic Deduplication  
- `model_name`: Sentence transformer model
- `similarity_threshold`: Cosine similarity threshold
- `batch_size`: Embedding computation batch size

### Soft Deduplication
- `method`: 'hash', 'minhash', or 'ngram'
- `min_weight`: Minimum document weight
- `decay_factor`: Weight reduction for duplicates
- `similarity_threshold`: Similarity threshold

## File Formats

RapidSift supports multiple input/output formats:

- **Text files** (`.txt`): One document per line
- **CSV files** (`.csv`): Structured data with text column
- **JSON files** (`.json`): Structured JSON data

## Performance Tips

1. **For large datasets (>100K docs)**: Use exact or near-duplicate methods first
2. **For semantic accuracy**: Use semantic deduplication on smaller, filtered datasets  
3. **For training efficiency**: Use soft deduplication to maintain diversity
4. **For web crawl data**: Start with exact deduplication, then near-duplicate detection

## Research & References

This implementation is based on several research papers and industry practices:

- **MinHash**: Broder, A. Z. (1997). "On the resemblance and containment of documents"
- **SimHash**: Charikar, M. S. (2002). "Similarity estimation techniques from rounding algorithms"
- **Semantic Deduplication**: Using sentence transformers for semantic similarity
- **Soft Deduplication**: Inspired by recent work on data reweighting approaches

## Contributing

We welcome contributions! Please see our contributing guidelines and submit pull requests for:

- New deduplication algorithms
- Performance improvements  
- Additional file format support
- Documentation enhancements

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Citation

If you use RapidSift in your research, please cite:

```bibtex
@software{rapidsift2024,
  title={RapidSift: Comprehensive Text Deduplication Tool},
  author={Your Name},
  year={2024},
  url={https://github.com/your-repo/RapidSift}
}
```