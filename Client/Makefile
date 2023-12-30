export RTE_SDK=/home/bwb/dpdk-stable-19.11.5
export RTE_TARGET=x86_64-native-linuxapp-gcc

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif


# export LD_LIBRARY_PATH=/home/bwb/dpdk-client/tools/
# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = client

# all source are stored in SRCS-y
# SRCS-y := csdn-demo.c
SRCS-y := client.c

CFLAGS += -g -O0 

LDLIBS += -L./tools
LDLIBS += -lxxhash


#-D_DUMP_PKT -D_DUMP_PKT_SEND #-D_STATIC_WORKLOAD_
#CFLAGS += $(WERROR_FLAGS)

include $(RTE_SDK)/mk/rte.extapp.mk


