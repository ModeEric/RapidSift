# Copyright and License Compliance Filtering

## Overview

The License Compliance Filter addresses the growing need to respect copyright and licensing restrictions when building training datasets. This filter helps ensure legal and ethical data collection by:

- Filtering out copyrighted content without proper licensing
- Maintaining allowlists of open-licensed domains 
- Detecting copyright notices and license indicators
- Handling opt-out requests and removal notices
- Validating compliance with various open licenses

## Key Features

### License Detection
- **Creative Commons**: Detects CC0, CC BY, CC BY-SA, CC BY-NC, CC BY-ND
- **Standard Licenses**: MIT, Apache 2.0, GPL v2/v3, BSD 2/3-Clause
- **Public Domain**: Government publications, archived materials
- **Proprietary Content**: Copyright notices, restricted content

### Domain Management
- **Allowlists**: Curated lists of trusted open-content domains
- **Blocklists**: Known copyright-heavy or problematic domains
- **Creative Commons Sites**: Domains with CC-licensed content
- **Public Domain Sources**: Government, academic, and archival sites

### Compliance Validation
- **Confidence Scoring**: Assess license detection reliability
- **Copyright Detection**: Find © notices and "all rights reserved" text
- **Paywall Detection**: Identify subscription-required content
- **Meta Tag Analysis**: Check HTML license metadata

### Legal Protection
- **Opt-out Processing**: Handle publisher removal requests
- **DMCA Compliance**: Support for takedown notices
- **Robots.txt Respect**: Honor crawling restrictions
- **Attribution Tracking**: Maintain license requirement records

## Usage Examples

### Basic Usage
```cpp
#include "rapidsift/license_filter.hpp"

// Create filter with default safe configuration
auto config = license_utils::create_strict_config();
LicenseFilter filter(config);

// Evaluate a document
Document doc("doc1", "CC BY licensed content...", "https://creativecommons.org/blog");
auto decision = filter.evaluate(doc);

if (decision.result == FilterResult::KEEP) {
    std::cout << "Document passed compliance check\n";
}
```

### Configuration Options
```cpp
// Strict configuration (research/academic use)
auto strict_config = license_utils::create_strict_config();
// Only allows: Public Domain, CC0, CC BY

// Permissive configuration (general web scraping)
auto permissive_config = license_utils::create_permissive_config();
// Allows: Most open licenses, more lenient with unknown

// Custom configuration
LicenseFilterConfig custom_config;
custom_config.allowed_domains = {"wikipedia.org", "github.com", "arxiv.org"};
custom_config.strict_mode = true;
custom_config.confidence_threshold = 0.8;
```

### Detailed Assessment
```cpp
// Get detailed copyright analysis
auto assessment = filter.assess_copyright(doc);

std::cout << "Detected License: " << license_utils::license_type_to_string(assessment.detected_license) << "\n";
std::cout << "Has Copyright Notice: " << assessment.has_copyright_notice << "\n";
std::cout << "Compliance Confidence: " << assessment.compliance_confidence << "\n";

for (const auto& notice : assessment.copyright_notices) {
    std::cout << "Copyright Notice: " << notice << "\n";
}
```

### Opt-out Management
```cpp
// Add opt-out requests
filter.add_opt_out_request("news-publisher.com", "Publisher requested removal");
filter.load_opt_out_list("opt_out_domains.txt");

// Check if domain is opted out
bool opted_out = filter.is_opted_out("example-domain.com");
```

### Domain Management
```cpp
// Load domain lists from files
filter.load_domain_lists("config/");  // Loads allowed_domains.txt, blocked_domains.txt, etc.

// Add domains programmatically
filter.add_allowed_domain("trusted-source.edu");
filter.add_blocked_domain("copyright-heavy-site.com");
```

## Configuration Types

### Strict Configuration
- **Use Case**: Research datasets, academic publications
- **License Policy**: Only public domain and CC0/CC BY
- **Rejection Policy**: Reject any uncertain licenses
- **Confidence Threshold**: 0.8 (high confidence required)

### Permissive Configuration
- **Use Case**: General web scraping, broad training data
- **License Policy**: Most open licenses allowed
- **Rejection Policy**: Only reject clear copyright violations
- **Confidence Threshold**: 0.5 (moderate confidence)

### Research Configuration
- **Use Case**: Scientific/educational datasets
- **License Policy**: Academic-friendly licenses
- **Domain Focus**: .edu, .gov, arxiv.org, pubmed, etc.
- **Special Handling**: Fair use considerations

## Domain Categories

### Allowed Domains (Default)
```
wikipedia.org          # CC BY-SA licensed
wikimedia.org         # Various CC licenses  
archive.org           # Public domain archives
gutenberg.org         # Public domain books
github.com            # Open source code
stackoverflow.com     # CC BY-SA content
arxiv.org            # Research preprints
nasa.gov             # Public domain science
```

### Creative Commons Domains
```
creativecommons.org   # CC organization
flickr.com           # CC photo sharing
openclipart.org      # CC graphics
freesound.org        # CC audio
```

### Blocked Domains (Examples)
```
major-news-sites.com  # Subscription/paywall content
premium-journals.com  # Academic paywalls
corporate-blogs.com   # Proprietary content
```

## License Types Supported

| License | Code | Allowed by Default | Commercial Use |
|---------|------|-------------------|----------------|
| Public Domain | PUBLIC_DOMAIN | ✅ | ✅ |
| CC0 | CREATIVE_COMMONS_0 | ✅ | ✅ |
| CC BY | CREATIVE_COMMONS_BY | ✅ | ✅ |
| CC BY-SA | CREATIVE_COMMONS_BY_SA | ⚠️ | ✅ |
| CC BY-NC | CREATIVE_COMMONS_BY_NC | ❌ | ❌ |
| CC BY-ND | CREATIVE_COMMONS_BY_ND | ❌ | ⚠️ |
| MIT | MIT | ✅ | ✅ |
| Apache 2.0 | APACHE_2 | ✅ | ✅ |
| GPL v2/v3 | GPL_V2, GPL_V3 | ⚠️ | ⚠️ |
| BSD | BSD_2_CLAUSE, BSD_3_CLAUSE | ✅ | ✅ |
| Proprietary | PROPRIETARY | ❌ | ❌ |
| Copyrighted | COPYRIGHTED | ❌ | ❌ |

## Compliance Best Practices

### 1. Dataset Documentation
- Document which filters were applied
- List allowed and blocked domains
- Record license detection confidence thresholds
- Maintain audit trails of removed content

### 2. Regular Updates
- Update domain lists as licensing changes
- Process new opt-out requests promptly  
- Review and update license detection patterns
- Monitor for new copyright challenges

### 3. Legal Considerations
- Consult legal experts for commercial use
- Understand fair use implications
- Respect robots.txt and meta tags
- Implement proper attribution systems

### 4. Transparency
- Publish filtering criteria openly
- Provide mechanisms for content owner contact
- Document removal and opt-out processes
- Regular compliance audits

## Advanced Features

### Robots.txt Compliance
```cpp
// Check robots.txt before processing
bool allowed = filter.check_robots_txt("example.com");
```

### Paywall Detection
```cpp
// Detect subscription-required content
bool is_paywalled = filter.is_paywalled(doc);
```

### Batch Processing
```cpp
// Process multiple documents efficiently
auto decisions = filter.evaluate_batch(documents);
```

### Statistics and Reporting
```cpp
// Get detailed compliance statistics
filter.print_compliance_report();

const auto& stats = filter.get_stats();
std::cout << "Compliance rate: " << stats.get_compliance_rate() << std::endl;
```

## Integration with Other Filters

The License Filter works alongside other RapidSift filters:

```cpp
// Combined filtering pipeline
ContentProcessor content_filter(content_config);
LicenseFilter license_filter(license_config);

// Apply both filters
auto content_result = content_filter.evaluate(doc);
if (content_result.result == FilterResult::KEEP) {
    auto license_result = license_filter.evaluate(doc);
    // Use both results for final decision
}
```

## Output and Logging

### Compliance Reports
- Total documents processed
- License distribution breakdown
- Rejection reasons and counts
- Domain-wise statistics
- Confidence score distributions

### Rejection Logs
- Detailed rejection reasons
- Source license information
- Confidence scores
- Suggested actions for content owners

## Legal Disclaimer

This filter is designed to help identify licensing and copyright issues, but it should not be considered legal advice. Users should:

- Consult qualified legal professionals for commercial applications
- Understand the specific legal requirements in their jurisdiction
- Implement proper attribution and compliance procedures
- Regularly review and update their policies

The filter's license detection is based on text analysis and may not catch all licensing terms or nuances. Human review is recommended for high-stakes applications. 