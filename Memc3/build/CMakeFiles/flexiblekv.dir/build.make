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
CMAKE_SOURCE_DIR = /home/bwb/GPCode/Memc3

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/bwb/GPCode/Memc3/build

# Include any dependencies generated for this target.
include CMakeFiles/flexiblekv.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/flexiblekv.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/flexiblekv.dir/flags.make

CMakeFiles/flexiblekv.dir/src/mempool.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/mempool.cc.o: ../src/mempool.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/flexiblekv.dir/src/mempool.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/mempool.cc.o -c /home/bwb/GPCode/Memc3/src/mempool.cc

CMakeFiles/flexiblekv.dir/src/mempool.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/mempool.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/mempool.cc > CMakeFiles/flexiblekv.dir/src/mempool.cc.i

CMakeFiles/flexiblekv.dir/src/mempool.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/mempool.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/mempool.cc -o CMakeFiles/flexiblekv.dir/src/mempool.cc.s

CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o: ../src/basic_hash.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o -c /home/bwb/GPCode/Memc3/src/basic_hash.cc

CMakeFiles/flexiblekv.dir/src/basic_hash.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/basic_hash.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/basic_hash.cc > CMakeFiles/flexiblekv.dir/src/basic_hash.cc.i

CMakeFiles/flexiblekv.dir/src/basic_hash.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/basic_hash.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/basic_hash.cc -o CMakeFiles/flexiblekv.dir/src/basic_hash.cc.s

CMakeFiles/flexiblekv.dir/src/operation.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/operation.cc.o: ../src/operation.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/flexiblekv.dir/src/operation.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/operation.cc.o -c /home/bwb/GPCode/Memc3/src/operation.cc

CMakeFiles/flexiblekv.dir/src/operation.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/operation.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/operation.cc > CMakeFiles/flexiblekv.dir/src/operation.cc.i

CMakeFiles/flexiblekv.dir/src/operation.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/operation.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/operation.cc -o CMakeFiles/flexiblekv.dir/src/operation.cc.s

CMakeFiles/flexiblekv.dir/src/cached.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/cached.cc.o: ../src/cached.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/flexiblekv.dir/src/cached.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/cached.cc.o -c /home/bwb/GPCode/Memc3/src/cached.cc

CMakeFiles/flexiblekv.dir/src/cached.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/cached.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/cached.cc > CMakeFiles/flexiblekv.dir/src/cached.cc.i

CMakeFiles/flexiblekv.dir/src/cached.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/cached.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/cached.cc -o CMakeFiles/flexiblekv.dir/src/cached.cc.s

CMakeFiles/flexiblekv.dir/src/memc3_util.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/memc3_util.cc.o: ../src/memc3_util.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/flexiblekv.dir/src/memc3_util.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/memc3_util.cc.o -c /home/bwb/GPCode/Memc3/src/memc3_util.cc

CMakeFiles/flexiblekv.dir/src/memc3_util.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/memc3_util.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/memc3_util.cc > CMakeFiles/flexiblekv.dir/src/memc3_util.cc.i

CMakeFiles/flexiblekv.dir/src/memc3_util.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/memc3_util.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/memc3_util.cc -o CMakeFiles/flexiblekv.dir/src/memc3_util.cc.s

CMakeFiles/flexiblekv.dir/src/stats.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/stats.cc.o: ../src/stats.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/flexiblekv.dir/src/stats.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/stats.cc.o -c /home/bwb/GPCode/Memc3/src/stats.cc

CMakeFiles/flexiblekv.dir/src/stats.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/stats.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/stats.cc > CMakeFiles/flexiblekv.dir/src/stats.cc.i

CMakeFiles/flexiblekv.dir/src/stats.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/stats.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/stats.cc -o CMakeFiles/flexiblekv.dir/src/stats.cc.s

CMakeFiles/flexiblekv.dir/src/slabs.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/slabs.cc.o: ../src/slabs.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/flexiblekv.dir/src/slabs.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/slabs.cc.o -c /home/bwb/GPCode/Memc3/src/slabs.cc

CMakeFiles/flexiblekv.dir/src/slabs.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/slabs.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/slabs.cc > CMakeFiles/flexiblekv.dir/src/slabs.cc.i

CMakeFiles/flexiblekv.dir/src/slabs.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/slabs.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/slabs.cc -o CMakeFiles/flexiblekv.dir/src/slabs.cc.s

CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.o: ../src/assoc_chain.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.o -c /home/bwb/GPCode/Memc3/src/assoc_chain.cc

CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/assoc_chain.cc > CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.i

CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/assoc_chain.cc -o CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.s

CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.o: ../src/assoc_cuckoo.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.o -c /home/bwb/GPCode/Memc3/src/assoc_cuckoo.cc

CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/assoc_cuckoo.cc > CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.i

CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/assoc_cuckoo.cc -o CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.s

CMakeFiles/flexiblekv.dir/src/items.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/items.cc.o: ../src/items.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object CMakeFiles/flexiblekv.dir/src/items.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/items.cc.o -c /home/bwb/GPCode/Memc3/src/items.cc

CMakeFiles/flexiblekv.dir/src/items.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/items.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/items.cc > CMakeFiles/flexiblekv.dir/src/items.cc.i

CMakeFiles/flexiblekv.dir/src/items.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/items.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/items.cc -o CMakeFiles/flexiblekv.dir/src/items.cc.s

CMakeFiles/flexiblekv.dir/src/hash.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/hash.cc.o: ../src/hash.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building CXX object CMakeFiles/flexiblekv.dir/src/hash.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/hash.cc.o -c /home/bwb/GPCode/Memc3/src/hash.cc

CMakeFiles/flexiblekv.dir/src/hash.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/hash.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/hash.cc > CMakeFiles/flexiblekv.dir/src/hash.cc.i

CMakeFiles/flexiblekv.dir/src/hash.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/hash.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/hash.cc -o CMakeFiles/flexiblekv.dir/src/hash.cc.s

CMakeFiles/flexiblekv.dir/src/thread.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/thread.cc.o: ../src/thread.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Building CXX object CMakeFiles/flexiblekv.dir/src/thread.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/thread.cc.o -c /home/bwb/GPCode/Memc3/src/thread.cc

CMakeFiles/flexiblekv.dir/src/thread.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/thread.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/thread.cc > CMakeFiles/flexiblekv.dir/src/thread.cc.i

CMakeFiles/flexiblekv.dir/src/thread.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/thread.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/thread.cc -o CMakeFiles/flexiblekv.dir/src/thread.cc.s

CMakeFiles/flexiblekv.dir/src/memcached.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/memcached.cc.o: ../src/memcached.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_13) "Building CXX object CMakeFiles/flexiblekv.dir/src/memcached.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/memcached.cc.o -c /home/bwb/GPCode/Memc3/src/memcached.cc

CMakeFiles/flexiblekv.dir/src/memcached.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/memcached.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/memcached.cc > CMakeFiles/flexiblekv.dir/src/memcached.cc.i

CMakeFiles/flexiblekv.dir/src/memcached.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/memcached.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/memcached.cc -o CMakeFiles/flexiblekv.dir/src/memcached.cc.s

CMakeFiles/flexiblekv.dir/src/communication.cc.o: CMakeFiles/flexiblekv.dir/flags.make
CMakeFiles/flexiblekv.dir/src/communication.cc.o: ../src/communication.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_14) "Building CXX object CMakeFiles/flexiblekv.dir/src/communication.cc.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flexiblekv.dir/src/communication.cc.o -c /home/bwb/GPCode/Memc3/src/communication.cc

CMakeFiles/flexiblekv.dir/src/communication.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flexiblekv.dir/src/communication.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/bwb/GPCode/Memc3/src/communication.cc > CMakeFiles/flexiblekv.dir/src/communication.cc.i

CMakeFiles/flexiblekv.dir/src/communication.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flexiblekv.dir/src/communication.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/bwb/GPCode/Memc3/src/communication.cc -o CMakeFiles/flexiblekv.dir/src/communication.cc.s

# Object files for target flexiblekv
flexiblekv_OBJECTS = \
"CMakeFiles/flexiblekv.dir/src/mempool.cc.o" \
"CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o" \
"CMakeFiles/flexiblekv.dir/src/operation.cc.o" \
"CMakeFiles/flexiblekv.dir/src/cached.cc.o" \
"CMakeFiles/flexiblekv.dir/src/memc3_util.cc.o" \
"CMakeFiles/flexiblekv.dir/src/stats.cc.o" \
"CMakeFiles/flexiblekv.dir/src/slabs.cc.o" \
"CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.o" \
"CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.o" \
"CMakeFiles/flexiblekv.dir/src/items.cc.o" \
"CMakeFiles/flexiblekv.dir/src/hash.cc.o" \
"CMakeFiles/flexiblekv.dir/src/thread.cc.o" \
"CMakeFiles/flexiblekv.dir/src/memcached.cc.o" \
"CMakeFiles/flexiblekv.dir/src/communication.cc.o"

# External object files for target flexiblekv
flexiblekv_EXTERNAL_OBJECTS =

libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/mempool.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/basic_hash.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/operation.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/cached.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/memc3_util.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/stats.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/slabs.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/assoc_chain.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/assoc_cuckoo.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/items.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/hash.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/thread.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/memcached.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/src/communication.cc.o
libflexiblekv.a: CMakeFiles/flexiblekv.dir/build.make
libflexiblekv.a: CMakeFiles/flexiblekv.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/bwb/GPCode/Memc3/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_15) "Linking CXX static library libflexiblekv.a"
	$(CMAKE_COMMAND) -P CMakeFiles/flexiblekv.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/flexiblekv.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/flexiblekv.dir/build: libflexiblekv.a

.PHONY : CMakeFiles/flexiblekv.dir/build

CMakeFiles/flexiblekv.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/flexiblekv.dir/cmake_clean.cmake
.PHONY : CMakeFiles/flexiblekv.dir/clean

CMakeFiles/flexiblekv.dir/depend:
	cd /home/bwb/GPCode/Memc3/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/bwb/GPCode/Memc3 /home/bwb/GPCode/Memc3 /home/bwb/GPCode/Memc3/build /home/bwb/GPCode/Memc3/build /home/bwb/GPCode/Memc3/build/CMakeFiles/flexiblekv.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/flexiblekv.dir/depend

