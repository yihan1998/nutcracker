CC := gcc
LD := gcc

SRC_DIR		:= ./
BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG
CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC -march=native $(DEBUG) #$(shell python3-config --includes)

INC 		:= -I/usr/include/ -I./entry -I./include
CFLAGS 		+= $(INC)

# Configurations
CONFIG 	= .config
include ${CONFIG}

ifeq ($(CONFIG_DPDK),y)
	CFLAGS	+= -DCONFIG_DPDK
endif

ifdef CONFIG_NR_CPUS
	CFLAGS	+= -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif

LDFLAGS 	:= -L/usr/lib -lpthread -lrt -ldl -ltcc #$(shell python3-config --ldflags)

# DPDK
# DPDK_DIR	= /root/dpdk-19.11
# DPDK_BUILD	= $(DPDK_DIR)/build
# CFLAGS 		+= -I$(DPDK_BUILD)/include 
# LDFLAGS 	+= -L$(DPDK_BUILD)/lib -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive \
# 				-lrte_eal -lrte_mbuf -lrte_ethdev -lrte_mempool \
#     			-lrte_ring -lrte_net -lrte_bus_pci -lrte_pci -lrte_bus_vdev \
#    	 			-ldl -lpthread -lm
LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
CFLAGS 		+= $(LIBDPDK_CFLAGS)
LDFLAGS 	+= $(LIBDPDK_LDFLAGS)

# Project files
LIBAIO_SRCS	= printk/printk.c kernel/sched/wait.c lib/find_bit.c fs/file.c lib/aio/aio.c
LIBAIO_OBJS	= $(LIBAIO_SRCS:%.c=$(BUILD_DIR)/lib/%.o)
LIBAIO 	= $(BUILD_DIR)/lib/libaio.so

LIBNFTNL_SRCS	= printk/printk.c lib/netfilter/netfilter.c
LIBNFTNL_OBJS	= $(LIBNFTNL_SRCS:%.c=$(BUILD_DIR)/lib/%.o)
LIBNFTNL	= $(BUILD_DIR)/lib/libnftnl.so

SUBDIRS 	:= entry/ kernel/ ipc/ net/ loader/ printk/

EXE_SRCS	:= $(shell find $(SUBDIRS) -name '*.c' -print) 
EXE_SRCS 	+= fs/file.c fs/fork.c fs/fs.c fs/read_write.c lib/find_bit.c main.c
EXE_OBJS 	= $(EXE_SRCS:%.c=$(BUILD_DIR)/%.o)
EXE_NAME 	= $(BUILD_DIR)/pistachio

all: $(EXE_NAME) $(LIBAIO) $(LIBNFTNL)

C_STANDARD	:= -std=gnu11

MKDIR_P := mkdir -p

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) -c $< -o $(@)

$(BUILD_DIR)/lib/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) -c $< -o $(@)

$(EXE_NAME): $(EXE_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS)
	@echo 'Finished building: $@'
	@echo ' '

$(LIBAIO): $(LIBAIO_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

$(LIBNFTNL): $(LIBNFTNL_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

clean:
	@rm -rf $(BUILD_DIR)
