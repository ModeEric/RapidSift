cmake_minimum_required(VERSION 3.12)
project(RapidSiftQuality VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compile options
add_compile_options(-Wall -Wextra -O3)

# Find required packages
find_package(OpenMP)
find_package(PkgConfig REQUIRED)

# Try to find xxhash
pkg_check_modules(XXHASH REQUIRED libxxhash)

# Find or download cxxopts
find_path(CXXOPTS_INCLUDE_DIR cxxopts.hpp PATHS /usr/include /usr/local/include)
if(NOT CXXOPTS_INCLUDE_DIR)
    include(FetchContent)
    FetchContent_Declare(
        cxxopts
        GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
        GIT_TAG v3.1.1
    )
    FetchContent_MakeAvailable(cxxopts)
endif()

# Find or download nlohmann/json
find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp PATHS /usr/include /usr/local/include)
if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Find or download re2 for regex operations
find_path(RE2_INCLUDE_DIR re2/re2.h PATHS /usr/include /usr/local/include)
find_library(RE2_LIBRARY re2 PATHS /usr/lib /usr/local/lib)

if(NOT RE2_INCLUDE_DIR OR NOT RE2_LIBRARY)
    include(FetchContent)
    FetchContent_Declare(
        re2
        GIT_REPOSITORY https://github.com/google/re2.git
        GIT_TAG 2023-07-01
    )
    FetchContent_MakeAvailable(re2)
    set(RE2_LIBRARY re2::re2)
else()
    add_library(re2_external INTERFACE)
    target_include_directories(re2_external INTERFACE ${RE2_INCLUDE_DIR})
    target_link_libraries(re2_external INTERFACE ${RE2_LIBRARY})
    set(RE2_LIBRARY re2_external)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CXXOPTS_INCLUDE_DIR)
    include_directories(${CXXOPTS_INCLUDE_DIR})
endif()
if(NLOHMANN_JSON_INCLUDE_DIR)
    include_directories(${NLOHMANN_JSON_INCLUDE_DIR})
endif()

# Source files
set(SOURCES
    src/length_filter.cpp
    src/gibberish_filter.cpp
    src/repetition_filter.cpp
    src/model_quality_filter.cpp
    src/utils.cpp
)

# Create library
add_library(rapidsift_quality ${SOURCES})

# Link libraries
target_link_libraries(rapidsift_quality 
    ${XXHASH_LIBRARIES} 
    ${RE2_LIBRARY}
)

# Link OpenMP if available
if(OpenMP_CXX_FOUND)
    target_link_libraries(rapidsift_quality OpenMP::OpenMP_CXX)
endif()

if(NOT CXXOPTS_INCLUDE_DIR)
    target_link_libraries(rapidsift_quality cxxopts)
endif()

if(NOT NLOHMANN_JSON_INCLUDE_DIR)
    target_link_libraries(rapidsift_quality nlohmann_json::nlohmann_json)
endif()

# Create main executable
add_executable(quality_filter src/main.cpp)
target_link_libraries(quality_filter rapidsift_quality)

# Add examples
add_subdirectory(examples)

# Add tests
enable_testing()
# add_subdirectory(tests)

# Install targets
install(TARGETS rapidsift_quality quality_filter
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include) 