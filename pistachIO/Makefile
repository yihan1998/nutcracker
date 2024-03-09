CC := gcc

SRC_DIR		:= ./
BUILD_DIR	:= ./build
RUNTIME_DIR	:= ../cacao
DEBUG		:= -DDEBUG
CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC $(DEBUG)
LIBPATH 	:= -I/usr/include/ -I./include
CFLAGS 		+= $(LIBPATH)

# Configurations
CONFIG 	= $(RUNTIME_DIR)/.config
include ${CONFIG}

ifeq ($(CONFIG_DPDK),y)
	CFLAGS	+= -DCONFIG_DPDK
endif

ifdef CONFIG_NR_CPUS
	CFLAGS	+= -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif

TARGET		:= $(BUILD_DIR)/pistachio

LDFLAGS 	:= -L/usr/lib -lpthread -lrt

# DPDK
LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
CFLAGS 		+= $(LIBDPDK_CFLAGS)
LDFLAGS 	+= $(LIBDPDK_LDFLAGS)

all: $(TARGET)

C_STANDARD	:= -std=gnu11

C_SRC		:= $(shell find ./ -name '*.c' -print)

OBJECTS 	:= $(C_SRC:%.c=$(BUILD_DIR)/%.o)

MKDIR_P := mkdir -p

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) -c $< -o $(@)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(C_STANDARD) $^ -o $@ $(LDFLAGS)

clean:
	@rm -rf $(BUILD_DIR)
