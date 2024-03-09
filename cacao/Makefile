CC = gcc
AS = gcc
LD = gcc

DEBUG_LEVEL	:= -DDEBUG
CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC $(DEBUG_LEVEL)

export CFLAGS

# Configurations
CONFIG 	= .config
include ${CONFIG}

ifeq ($(CONFIG_DPDK),y)
	CFLAGS	+= -DCONFIG_DPDK
endif

ifdef CONFIG_NR_CPUS
	CFLAGS	+= -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif

ifeq ($(CONFIG_HAVE_XSTATE),y)
	CFLAGS	+= -DCONFIG_HAVE_XSTATE
endif

SRC_DIR		:= .
BUILD_DIR	:= ./build

TARGET		:= $(BUILD_DIR)/libcacao.so

LDFLAGS 	:= -shared -ldl -lrt -L/usr/lib/x86_64-native-linux-gnu

# DPDK
LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
CFLAGS 		+= $(LIBDPDK_CFLAGS)
LDFLAGS 	+= $(LIBDPDK_LDFLAGS)

SUBDIRS := entry/ kernel/ net/ fs/ init/ lib/ #ipc/ net/ mm/ pthl/ 

C_SRC	:= $(shell find $(SRC_DIR) -name '*.c' -print)

OBJECTS := $(C_SRC:%.c=$(BUILD_DIR)/%.o)

MKDIR_P := mkdir -p

all:	$(SUBDIRS) $(TARGET)

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: all $(SUBDIRS)

$(BUILD_DIR)/libcacao.so: $(OBJECTS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $(OBJECTS) $(LDFLAGS)
	@echo 'Finished building: $@'
	@echo ' '

clean:
	@echo 'Cleaning $(BUILD_DIR)'
	@rm -rf $(BUILD_DIR)
