cmake_minimum_required(VERSION 3.10)
project(RapidSift VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options for performance
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address")
endif()

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Option to enable OpenMP for parallel processing
option(USE_OPENMP "Enable OpenMP for parallel processing" ON)
if(USE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        add_definitions(-DUSE_OPENMP)
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Note: External dependencies disabled for compatibility
# The code uses built-in alternatives for maximum compatibility:
# - Standard library hash functions instead of xxhash
# - Simple command-line parsing instead of cxxopts
# - Basic JSON-like output instead of nlohmann_json

# Core library
add_library(rapidsift_core
    src/exact_dedup.cpp
    src/near_dedup.cpp
    src/utils.cpp
    src/language_filter.cpp
    src/text_extractor.cpp
    src/text_extractor_utils.cpp
    src/decontamination_filter.cpp
)

target_link_libraries(rapidsift_core 
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(rapidsift_core OpenMP::OpenMP_CXX)
endif()

# Main executable
add_executable(rapidsift
    src/main.cpp
)

target_link_libraries(rapidsift 
    rapidsift_core
    # Note: cxxopts and nlohmann_json dependencies removed for compatibility
)

# Examples
add_subdirectory(examples)

# Testing
enable_testing()

# Individual test executables
add_executable(test_exact_dedup
    tests/test_exact_dedup.cpp
)
target_link_libraries(test_exact_dedup rapidsift_core)

add_executable(test_near_dedup
    tests/test_near_dedup.cpp
)
target_link_libraries(test_near_dedup rapidsift_core)

add_executable(test_utils
    tests/test_utils.cpp
)
target_link_libraries(test_utils rapidsift_core)

add_executable(test_language_filter
    tests/test_language_filter.cpp
)
target_link_libraries(test_language_filter rapidsift_core)

add_executable(test_text_extractor
    tests/test_text_extractor.cpp
)
target_link_libraries(test_text_extractor rapidsift_core)

# Comprehensive test runner
add_executable(run_all_tests
    tests/run_all_tests.cpp
)
target_link_libraries(run_all_tests rapidsift_core)

# Add tests to CTest
add_test(NAME ExactDeduplication COMMAND test_exact_dedup)
add_test(NAME NearDeduplication COMMAND test_near_dedup)
add_test(NAME Utilities COMMAND test_utils)
add_test(NAME LanguageFilter COMMAND test_language_filter)
add_test(NAME TextExtractor COMMAND test_text_extractor)
add_test(NAME Integration COMMAND run_all_tests)

# Set test properties
set_tests_properties(ExactDeduplication PROPERTIES TIMEOUT 30)
set_tests_properties(NearDeduplication PROPERTIES TIMEOUT 60)
set_tests_properties(Utilities PROPERTIES TIMEOUT 30)
set_tests_properties(LanguageFilter PROPERTIES TIMEOUT 60)
set_tests_properties(TextExtractor PROPERTIES TIMEOUT 60)
set_tests_properties(Integration PROPERTIES TIMEOUT 120)

# Performance tests (longer timeout)
add_executable(performance_test
    tests/performance_test.cpp
)
target_link_libraries(performance_test rapidsift_core)
add_test(NAME Performance COMMAND performance_test)
set_tests_properties(Performance PROPERTIES TIMEOUT 300)

# Test data generator utility
add_executable(create_test_data
    tests/create_test_data.cpp
)
# No dependencies needed for test data generator

# Custom target for running all tests with nice output
add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure
    DEPENDS test_exact_dedup test_near_dedup test_utils test_language_filter test_text_extractor run_all_tests performance_test
    COMMENT "Running all RapidSift tests"
)

# Custom target for generating test data
add_custom_target(generate_test_data
    COMMAND create_test_data
    DEPENDS create_test_data
    COMMENT "Generating comprehensive test data files"
)

# Installation
install(TARGETS rapidsift DESTINATION bin)
install(TARGETS rapidsift_core DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Install test executables (optional)
install(TARGETS run_all_tests DESTINATION bin OPTIONAL)

# Config file generation skipped for simplicity

# Summary
message(STATUS "")
message(STATUS "RapidSift Build Configuration Summary:")
message(STATUS "======================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenMP support: ${USE_OPENMP}")
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: Yes")
else()
    message(STATUS "OpenMP found: No")
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  rapidsift          - Main CLI application")
message(STATUS "  rapidsift_core     - Core library")
message(STATUS "  test_all           - Run all tests")
message(STATUS "  run_all_tests      - Comprehensive test runner")
message(STATUS "  generate_test_data - Create test data files")
message(STATUS "  create_test_data   - Test data generator utility")
message(STATUS "")
message(STATUS "To build and test:")
message(STATUS "  make -j$(nproc)")
message(STATUS "  make test_all")
message(STATUS "") 