#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <exception>
#include <type_traits>

namespace test_framework {

// Utility functions for safe string conversion
template<typename T>
std::string to_string_safe(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else {
        return std::string(value);
    }
}

// Specialization for std::string
inline std::string to_string_safe(const std::string& value) {
    return value;
}

// Specialization for const char*
inline std::string to_string_safe(const char* value) {
    return std::string(value);
}



class TestResult {
public:
    bool passed = false;
    std::string test_name;
    std::string error_message;
    double duration_ms = 0.0;
};

class TestSuite {
public:
    TestSuite(const std::string& name) : suite_name_(name) {}
    
    void add_test(const std::string& name, std::function<void()> test_func) {
        tests_.push_back({name, test_func});
    }
    
    void run_all() {
        std::cout << "🧪 Running Test Suite: " << suite_name_ << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        int passed = 0;
        int failed = 0;
        
        for (const auto& test : tests_) {
            auto result = run_test(test.name, test.func);
            
            if (result.passed) {
                std::cout << "✅ " << result.test_name 
                         << " (" << result.duration_ms << "ms)" << std::endl;
                ++passed;
            } else {
                std::cout << "❌ " << result.test_name 
                         << " (" << result.duration_ms << "ms)" << std::endl;
                std::cout << "   Error: " << result.error_message << std::endl;
                ++failed;
            }
        }
        
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "📊 Results: " << passed << " passed, " << failed << " failed" << std::endl;
        
        if (failed > 0) {
            std::cout << "❌ Test suite FAILED" << std::endl;
        } else {
            std::cout << "✅ Test suite PASSED" << std::endl;
        }
        std::cout << std::endl;
    }

private:
    struct Test {
        std::string name;
        std::function<void()> func;
    };
    
    std::string suite_name_;
    std::vector<Test> tests_;
    
    TestResult run_test(const std::string& name, std::function<void()> test_func) {
        TestResult result;
        result.test_name = name;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            test_func();
            result.passed = true;
        } catch (const std::exception& e) {
            result.passed = false;
            result.error_message = e.what();
        } catch (...) {
            result.passed = false;
            result.error_message = "Unknown exception";
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        result.duration_ms = duration.count() / 1000.0;
        
        return result;
    }
};

// Test assertion macros
#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error("ASSERT_TRUE failed: " #condition); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error("ASSERT_FALSE failed: " #condition); \
    }

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        throw std::runtime_error("ASSERT_EQ failed: expected \"" + test_framework::to_string_safe(expected) + \
                                 "\" but got \"" + test_framework::to_string_safe(actual) + "\""); \
    }

#define ASSERT_NE(expected, actual) \
    if ((expected) == (actual)) { \
        throw std::runtime_error("ASSERT_NE failed: values should not be equal: " + test_framework::to_string_safe(expected)); \
    }

#define ASSERT_LT(a, b) \
    if (!((a) < (b))) { \
        throw std::runtime_error("ASSERT_LT failed: " + std::to_string(a) + " is not less than " + std::to_string(b)); \
    }

#define ASSERT_GT(a, b) \
    if (!((a) > (b))) { \
        throw std::runtime_error("ASSERT_GT failed: " + std::to_string(a) + " is not greater than " + std::to_string(b)); \
    }

#define ASSERT_GE(a, b) \
    if (!((a) >= (b))) { \
        throw std::runtime_error("ASSERT_GE failed: " + std::to_string(a) + " is not greater than or equal to " + std::to_string(b)); \
    }

#define ASSERT_LE(a, b) \
    if (!((a) <= (b))) { \
        throw std::runtime_error("ASSERT_LE failed: " + std::to_string(a) + " is not less than or equal to " + std::to_string(b)); \
    }

#define ASSERT_NEAR(expected, actual, tolerance) \
    if (std::abs((expected) - (actual)) > (tolerance)) { \
        throw std::runtime_error("ASSERT_NEAR failed: " + std::to_string(actual) + \
                                 " is not within " + std::to_string(tolerance) + \
                                 " of " + std::to_string(expected)); \
    }

#define ASSERT_STREQ(expected, actual) \
    if ((expected) != (actual)) { \
        throw std::runtime_error("ASSERT_STREQ failed: expected \"" + std::string(expected) + \
                                 "\" but got \"" + std::string(actual) + "\""); \
    }

#define ASSERT_THROWS(expression, exception_type) \
    try { \
        expression; \
        throw std::runtime_error("ASSERT_THROWS failed: expected exception was not thrown"); \
    } catch (const exception_type&) { \
        /* Expected exception caught */ \
    }

} // namespace test_framework 