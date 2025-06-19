# CMake generated Testfile for 
# Source directory: /Users/ericmodesitt/RapidSift/filters/dedup
# Build directory: /Users/ericmodesitt/RapidSift/filters/dedup/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ExactDeduplication "/Users/ericmodesitt/RapidSift/filters/dedup/build/test_exact_dedup")
set_tests_properties(ExactDeduplication PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;106;add_test;/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;0;")
add_test(NearDeduplication "/Users/ericmodesitt/RapidSift/filters/dedup/build/test_near_dedup")
set_tests_properties(NearDeduplication PROPERTIES  TIMEOUT "60" _BACKTRACE_TRIPLES "/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;107;add_test;/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;0;")
add_test(Utilities "/Users/ericmodesitt/RapidSift/filters/dedup/build/test_utils")
set_tests_properties(Utilities PROPERTIES  TIMEOUT "30" _BACKTRACE_TRIPLES "/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;108;add_test;/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;0;")
add_test(LanguageFilter "/Users/ericmodesitt/RapidSift/filters/dedup/build/test_language_filter")
set_tests_properties(LanguageFilter PROPERTIES  TIMEOUT "60" _BACKTRACE_TRIPLES "/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;109;add_test;/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;0;")
add_test(TextExtractor "/Users/ericmodesitt/RapidSift/filters/dedup/build/test_text_extractor")
set_tests_properties(TextExtractor PROPERTIES  TIMEOUT "60" _BACKTRACE_TRIPLES "/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;110;add_test;/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;0;")
add_test(Integration "/Users/ericmodesitt/RapidSift/filters/dedup/build/run_all_tests")
set_tests_properties(Integration PROPERTIES  TIMEOUT "120" _BACKTRACE_TRIPLES "/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;111;add_test;/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;0;")
add_test(Performance "/Users/ericmodesitt/RapidSift/filters/dedup/build/performance_test")
set_tests_properties(Performance PROPERTIES  TIMEOUT "300" _BACKTRACE_TRIPLES "/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;126;add_test;/Users/ericmodesitt/RapidSift/filters/dedup/CMakeLists.txt;0;")
subdirs("examples")
