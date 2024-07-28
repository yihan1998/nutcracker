CC := gcc
LD := gcc

SRC_DIR		:= ./
BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG
CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC -march=native $(DEBUG) #$(shell python3-config --includes)

INC 		:= -I/usr/include/ -I../include
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

LDFLAGS 	:= -L/usr/lib -lssl -lcrypto -lpthread -lrt -ldl #$(shell python3-config --ldflags)

SCHED_SUBDIRS	:= flax/
IO_SUBDIRS		:= pistachIO/
RT_SUBDIRS		:= sesame/

SCHED_SRCS		:= $(shell find $(SUBDIRS) -name '*.c' -print)
OBJS 		= $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET 		= $(BUILD_DIR)/nutcracker

all: $(TARGET)

C_STANDARD	:= -std=gnu11

MKDIR_P := mkdir -p

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) -c $< -o $(@)

$(TARGET): $(OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS)
	@echo 'Finished building: $@'
	@echo ' '