# Examples for RapidSift Content Filter
add_executable(content_example simple_example.cpp)
target_link_libraries(content_example rapidsift_content)

# Install examples
install(TARGETS content_example RUNTIME DESTINATION bin/examples) 