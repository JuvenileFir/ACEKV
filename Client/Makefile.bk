ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = client

# all source are stored in SRCS-y
# SRCS-y := csdn-demo.c
SRCS-y := client.c

CFLAGS += -g -O0 #-D_DUMP_PKT -D_DUMP_PKT_SEND #-D_STATIC_WORKLOAD_
#CFLAGS += $(WERROR_FLAGS)

include $(RTE_SDK)/mk/rte.extapp.mk
