# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 4.0

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/bin/cmake

# The command to remove a file.
RM = /usr/local/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/ericmodesitt/RapidSift/filters/dedup

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/ericmodesitt/RapidSift/filters/dedup/build

# Include any dependencies generated for this target.
include CMakeFiles/create_test_data.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/create_test_data.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/create_test_data.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/create_test_data.dir/flags.make

CMakeFiles/create_test_data.dir/codegen:
.PHONY : CMakeFiles/create_test_data.dir/codegen

CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o: CMakeFiles/create_test_data.dir/flags.make
CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o: /Users/ericmodesitt/RapidSift/filters/dedup/tests/create_test_data.cpp
CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o: CMakeFiles/create_test_data.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/ericmodesitt/RapidSift/filters/dedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o -MF CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o.d -o CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o -c /Users/ericmodesitt/RapidSift/filters/dedup/tests/create_test_data.cpp

CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/ericmodesitt/RapidSift/filters/dedup/tests/create_test_data.cpp > CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.i

CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/ericmodesitt/RapidSift/filters/dedup/tests/create_test_data.cpp -o CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.s

# Object files for target create_test_data
create_test_data_OBJECTS = \
"CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o"

# External object files for target create_test_data
create_test_data_EXTERNAL_OBJECTS =

create_test_data: CMakeFiles/create_test_data.dir/tests/create_test_data.cpp.o
create_test_data: CMakeFiles/create_test_data.dir/build.make
create_test_data: CMakeFiles/create_test_data.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/ericmodesitt/RapidSift/filters/dedup/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable create_test_data"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/create_test_data.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/create_test_data.dir/build: create_test_data
.PHONY : CMakeFiles/create_test_data.dir/build

CMakeFiles/create_test_data.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/create_test_data.dir/cmake_clean.cmake
.PHONY : CMakeFiles/create_test_data.dir/clean

CMakeFiles/create_test_data.dir/depend:
	cd /Users/ericmodesitt/RapidSift/filters/dedup/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/ericmodesitt/RapidSift/filters/dedup /Users/ericmodesitt/RapidSift/filters/dedup /Users/ericmodesitt/RapidSift/filters/dedup/build /Users/ericmodesitt/RapidSift/filters/dedup/build /Users/ericmodesitt/RapidSift/filters/dedup/build/CMakeFiles/create_test_data.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/create_test_data.dir/depend

