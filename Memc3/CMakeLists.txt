CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(piekv C CXX)

option(USE_MLX5 "Compile with mlx5 driver" ON)
option(MEM_EFFICIENCY "Compile with print memory utilization rate on each memory flowing" OFF)
option(LANTENCY "Comlile with macro for latency test" OFF)
option(LANTENCY_TRANSITION "Comlile with macro for testing latency only during transition period" OFF)
option(MULTIPLE_SHIFT "Each trigger shifts multiple pages" OFF)
option(TEST_HIT_RATE "Test hit ratio" OFF)

SET(CMAKE_CXX_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -ggdb -std=gnu++11 -march=native -msse4.2")
SET(CMAKE_C_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -ggdb -std=gnu11 -march=native -msse4.2")
SET(CMAKE_CXX_STANDARD 14)#后来加的
# Set default build type as debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default build type: Debug." FORCE)
endif()

if(MULTIPLE_SHIFT)
	add_definitions(-DMULTIPLE_SHIFT)
endif()

if(TEST_HIT_RATE)
	add_definitions(-DLOCAL_HIT_TEST)
endif()

if(MEM_EFFICIENCY)
	add_definitions(-DEXP_MEM_EFFICIENCY)
endif(MEM_EFFICIENCY)

if(LANTENCY OR LANTENCY_TRANSITION)
	message("Doing EXP for testing latency")
	add_definitions(-DEXP_LATENCY)
    # set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -DEXP_TRANSITION_LATENCY)
    # set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -DEXP_TRANSITION_LATENCY)
endif()

if(LANTENCY_TRANSITION)
	message("Doing EXP for testing latency only during transition period")
	add_definitions(-DTRANSITION_ONLY)
endif(LANTENCY_TRANSITION)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_C_FLAGS_DEBUG "-O0")

add_library(flexiblekv STATIC
	src/mempool
    src/basic_hash
    src/operation
    src/cached
    src/memc3_util
    src/stats
    src/slabs
    src/assoc_chain
    src/assoc_cuckoo
    src/items
    src/hash
    src/thread
    src/memcached
	src/communication
)

SET(DPDK_LIBS
	rte_net	     rte_ethdev       rte_kni        rte_mbuf      rte_distributor	  rte_pmd_bond         rte_pmd_ring  	rte_power
	rte_acl      rte_eal          rte_kvargs   	 rte_mempool   rte_mempool_ring	  rte_pmd_e1000        rte_pmd_virtio   rte_ring
	rte_cfgfile  rte_hash         rte_lpm      	 rte_meter     rte_pmd_i40e   	  rte_pmd_vmxnet3_uio  rte_sched        rte_timer
	rte_cmdline  rte_ip_frag      rte_pipeline 	 rte_pmd_ixgbe rte_mempool_stack  rte_port             rte_table        rte_pmd_bnxt
	rte_pmd_ena  rte_pmd_nfp
	rte_pci		 rte_bus_vdev	  rte_pmd_vdev_netvsc		   rte_stack		  rte_cryptodev	  	   rte_eventdev	    rte_bus_pci
	# uct
)

# Add Mellanox libraries if options are specified
if(USE_MLX5)
	SET(DPDK_LIBS ${DPDK_LIBS} rte_pmd_mlx5 ibverbs mlx5)
endif()

SET(ALL_LIBS
	-Wl,--whole-archive
	-Wl,--start-group ${DPDK_LIBS} numa -Wl,--end-group
	pthread dl rt m
	-Wl,--no-whole-archive
	-gdwarf-4 -g3
	-lcurl
)

INCLUDE_DIRECTORIES(
	/home/bwb/dpdk-stable-19.11.5/x86_64-native-linuxapp-gcc/include
	/home/bwb/dpdk-stable-19.11.5/drivers/net/bnxt
	/home/bwb/dpdk-stable-19.11.5/dpdk/drivers/net/ixgbe/base
	/home/bwb/dpdk-stable-19.11.5/drivers/net/ixgbe
	/home/bwb/dpdk-stable-19.11.5/drivers/net/i40e/base
	/home/bwb/dpdk-stable-19.11.5/drivers/net/i40e
	/home/bwb/dpdk-stable-19.11.5/drivers/net/e1000
	/home/bwb/dpdk-stable-19.11.5/drivers/net/e1000/base
	/home/bwb/dpdk-stable-19.11.5/drivers/net/mlx5
	${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(
	/usr/lib/gcc/x86_64-linux-gnu/7/../../../x86_64-linux-gnu
	/home/bwb/dpdk-stable-19.11.5/x86_64-native-linuxapp-gcc/lib
	/usr/lib/x86_64-linux-gnu
)

# add_definitions(-D_DEVELOPER_MOOD_)
# add_definitions(-DBUCKET_CLEANER)
add_definitions(-DCOLLECT_STATS -DTABLE_COLLECT_STATS -DSTORE_COLLECT_STATS)
# add_definitions(-DIMPORT_LOG)
# add_definitions(-D_DUMP_PKT)
# add_definitions(-D_DUMP_PKT_SEND)
add_definitions(-DTABLE_CONCURRENT -DCONCURRENT)
add_definitions(-DHARDCODE_DIVISION)
add_definitions(-DASSUME_HASHED)


ADD_EXECUTABLE(main src/main.cc)
TARGET_LINK_LIBRARIES(main flexiblekv)
TARGET_LINK_LIBRARIES(main ${ALL_LIBS})
# TARGET_LINK_LIBRARIES(mem_flowing libxxhash.so)

# add_subdirectory(perf_count)
# set(LIBRARY_PATH ${LIBRARY_PATH} perf_count/)

# add_definitions(-DUSE_PERF_COUNT)

# ADD_EXECUTABLE(microbench microbench.cpp)
# TARGET_LINK_LIBRARIES(microbench flexiblekv)
# TARGET_LINK_LIBRARIES(microbench ${ALL_LIBS})
# TARGET_LINK_LIBRARIES(microbench libxxhash.so)
# TARGET_LINK_LIBRARIES(microbench perf_count)
