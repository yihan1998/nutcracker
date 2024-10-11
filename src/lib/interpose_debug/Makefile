CC := gcc
LD := gcc

# SRC_DIR		:= ./
INC_DIR		:= ./include -I../../include -I../../common/entry -I../../drivers

BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG

CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -march=native $(DEBUG)
CFLAGS 		+= -I/usr/include/ -I$(INC_DIR)

# Configurations
CONFIG 	= ../../.config
include ${CONFIG}

ifdef CONFIG_NR_CPUS
	CFLAGS	+= -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif

CFLAGS	+= -DCONFIG_DOCA

LDFLAGS 	:= -L/usr/lib -ldl -lz

LIBDOCA_CFLAGS := $(shell pkg-config --cflags doca)
LIBDOCA_LDFLAGS := $(shell pkg-config --libs doca)
CFLAGS 		+= $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
LDFLAGS 	+= $(LIBDOCA_LDFLAGS)

# CFLAGS	+= -DCONFIG_DOCA_REGEX
CFLAGS	+= -DCONFIG_DOCA_COMPRESS

LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
CFLAGS 		+= $(LIBDPDK_CFLAGS)
LDFLAGS 	+= $(LIBDPDK_LDFLAGS) -L/usr/lib/aarch64-linux-gnu -lz

SRCS	= $(shell find ../../drivers/doca -name '*.c' -print) $(shell find ../../common -name '*.c' -print) $(shell find ./ -name '*.c' -print)
OBJS	= $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET	= $(BUILD_DIR)/interpose

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

clean:
	@rm -rf $(BUILD_DIR)