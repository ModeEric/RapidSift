# Examples CMakeLists.txt for RapidSift Quality Filter

# Model-based quality filtering example
add_executable(model_filtering_example model_filtering_example.cpp)
target_link_libraries(model_filtering_example rapidsift_quality)

# Install examples
install(TARGETS 
    model_filtering_example
    RUNTIME DESTINATION bin/examples
) 