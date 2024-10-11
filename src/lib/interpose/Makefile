CC := gcc
LD := gcc

# SRC_DIR		:= ./
INC_DIR		:= ./include -I../../include -I../../common/entry -I../../drivers

BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG

CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC  -shared -march=native $(DEBUG)
CFLAGS 		+= -I/usr/include/ -I$(INC_DIR)

# Configurations
CONFIG 	= ../../.config
include ${CONFIG}

ifdef CONFIG_NR_CPUS
	CFLAGS	+= -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif

ifeq ($(CONFIG_BLUEFIELD2),y)
ifeq ($(CONFIG_DOCA),y)
	# SRC_DIR	+= ../../drivers/doca
	CFLAGS	+= -DCONFIG_DOCA

	LIBDOCA_CFLAGS := $(shell pkg-config --cflags doca)
	LIBDOCA_LDFLAGS := $(shell pkg-config --libs doca)
	CFLAGS 		+= $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
	LDFLAGS 	+= $(LIBDOCA_LDFLAGS)
	
	ifeq ($(CONFIG_DOCA_REGEX),y)
		CFLAGS	+= -DCONFIG_DOCA_REGEX
	endif

	ifeq ($(CONFIG_DOCA_COMPRESS),y)
		CFLAGS	+= -DCONFIG_DOCA_COMPRESS
	endif
endif
endif

LDFLAGS 	:= -L/usr/lib -ldl

LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
CFLAGS 		+= $(LIBDPDK_CFLAGS)
LDFLAGS 	+= $(LIBDPDK_LDFLAGS)

# SRCS	= $(shell find ../../common -name '*.c' -print) $(shell find ./ -name '*.c' -print)
SRCS	= $(shell find ../../drivers/doca -name '*.c' -print) $(shell find ./ -name '*.c' -print)
OBJS	= $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET	= $(BUILD_DIR)/libnutcracker-interpose.so

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
	$(LD) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

clean:
	@rm -rf $(BUILD_DIR)