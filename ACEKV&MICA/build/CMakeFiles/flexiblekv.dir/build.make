# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/bwb/GPCode/proto-mica

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/bwb/GPCode/proto-mica/build

# Include any dependencies generated for this target.
include CMakeFiles/flexiblekv.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/flexiblekv.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/flexiblekv.dir/flags.make

CMakeFiles/flexiblekv.dir/src/mempool.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/mempool.cc.o: ../src/mempool.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/flexiblekv.dir/src/mempool.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/mempool.cc.o -c /home/bwb/GPCode/proto-mica/src/mempool.cc

CMakeFiles/flexiblekv.dir/src/mempool.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/mempool.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/proto-mica/src/mempool.cc > CMakeFiles/flexiblekv.dir/src/mempool.cc.i

CMakeFiles/flexiblekv.dir/src/mempool.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/mempool.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/proto-mica/src/mempool.cc -o CMakeFiles/flexiblekv.dir/src/mempool.cc.s

CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o: ../src/basic_hash.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o -c /home/bwb/GPCode/proto-mica/src/basic_hash.cc

CMakeFiles/flexiblekv.dir/src/basic_hash.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/basic_hash.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/proto-mica/src/basic_hash.cc > CMakeFiles/flexiblekv.dir/src/basic_hash.cc.i

CMakeFiles/flexiblekv.dir/src/basic_hash.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/basic_hash.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/proto-mica/src/basic_hash.cc -o CMakeFiles/flexiblekv.dir/src/basic_hash.cc.s

CMakeFiles/flexiblekv.dir/src/cuckoo.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/cuckoo.cc.o: ../src/cuckoo.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/flexiblekv.dir/src/cuckoo.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/cuckoo.cc.o -c /home/bwb/GPCode/proto-mica/src/cuckoo.cc

CMakeFiles/flexiblekv.dir/src/cuckoo.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/cuckoo.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/proto-mica/src/cuckoo.cc > CMakeFiles/flexiblekv.dir/src/cuckoo.cc.i

CMakeFiles/flexiblekv.dir/src/cuckoo.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/cuckoo.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/proto-mica/src/cuckoo.cc -o CMakeFiles/flexiblekv.dir/src/cuckoo.cc.s

CMakeFiles/flexiblekv.dir/src/hashtable.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/hashtable.cc.o: ../src/hashtable.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/flexiblekv.dir/src/hashtable.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/hashtable.cc.o -c /home/bwb/GPCode/proto-mica/src/hashtable.cc

CMakeFiles/flexiblekv.dir/src/hashtable.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/hashtable.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/proto-mica/src/hashtable.cc > CMakeFiles/flexiblekv.dir/src/hashtable.cc.i

CMakeFiles/flexiblekv.dir/src/hashtable.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/hashtable.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/proto-mica/src/hashtable.cc -o CMakeFiles/flexiblekv.dir/src/hashtable.cc.s

CMakeFiles/flexiblekv.dir/src/log.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/log.cc.o: ../src/log.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/flexiblekv.dir/src/log.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/log.cc.o -c /home/bwb/GPCode/proto-mica/src/log.cc

CMakeFiles/flexiblekv.dir/src/log.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/log.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/proto-mica/src/log.cc > CMakeFiles/flexiblekv.dir/src/log.cc.i

CMakeFiles/flexiblekv.dir/src/log.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/log.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/proto-mica/src/log.cc -o CMakeFiles/flexiblekv.dir/src/log.cc.s

CMakeFiles/flexiblekv.dir/src/operation.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/operation.cc.o: ../src/operation.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/flexiblekv.dir/src/operation.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/operation.cc.o -c /home/bwb/GPCode/proto-mica/src/operation.cc

CMakeFiles/flexiblekv.dir/src/operation.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/operation.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/proto-mica/src/operation.cc > CMakeFiles/flexiblekv.dir/src/operation.cc.i

CMakeFiles/flexiblekv.dir/src/operation.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/operation.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/proto-mica/src/operation.cc -o CMakeFiles/flexiblekv.dir/src/operation.cc.s

CMakeFiles/flexiblekv.dir/src/communication.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/communication.cc.o: ../src/communication.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/flexiblekv.dir/src/communication.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/communication.cc.o -c /home/bwb/GPCode/proto-mica/src/communication.cc

CMakeFiles/flexiblekv.dir/src/communication.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/communication.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/proto-mica/src/communication.cc > CMakeFiles/flexiblekv.dir/src/communication.cc.i

CMakeFiles/flexiblekv.dir/src/communication.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/communication.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/proto-mica/src/communication.cc -o CMakeFiles/flexiblekv.dir/src/communication.cc.s

# Object files for target flexiblekv
flexiblekv_OBJECTS = \
"CMakeFiles/flexiblekv.dir/src/mempool.cc.o" \
"CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o" \
"CMakeFiles/flexiblekv.dir/src/cuckoo.cc.o" \
"CMakeFiles/flexiblekv.dir/src/hashtable.cc.o" \
"CMakeFiles/flexiblekv.dir/src/log.cc.o" \
"CMakeFiles/flexiblekv.dir/src/operation.cc.o" \
"CMakeFiles/flexiblekv.dir/src/communication.cc.o"

# External object files for target flexiblekv
flexiblekv_EXTERNAL_OBJECTS =

libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/mempool.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/cuckoo.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/hashtable.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/log.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/operation.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/communication.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/build.make
libflexiblekv.a: CMakeFiles/flexiblekv.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/bwb/GPCode/proto-mica/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Linking CXX static library libflexiblekv.a"
	$(CMAKE_COMMAND) -P CMakeFiles/flexiblekv.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/flexiblekv.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/flexiblekv.dir/build: libflexiblekv.a

.PHONY : CMakeFiles/flexiblekv.dir/build

CMakeFiles/flexiblekv.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/flexiblekv.dir/cmake_clean.cmake
.PHONY : CMakeFiles/flexiblekv.dir/clean

CMakeFiles/flexiblekv.dir/depend:
	cd /home/bwb/GPCode/proto-mica/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bwb/GPCode/proto-mica /home/bwb/GPCode/proto-mica /home/bwb/GPCode/proto-mica/build /home/bwb/GPCode/proto-mica/build /home/bwb/GPCode/proto-mica/build/CMakeFiles/flexiblekv.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/flexiblekv.dir/depend

