CC := gcc
LD := gcc

COMMON_DIR	:= ../src/common
INC_DIR		:= ../src/include

BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG

CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC -march=native $(DEBUG) $(shell python3-config --includes)
CFLAGS 		+= -I/usr/include/ -I$(INC_DIR)

LDFLAGS 	:= -L/usr/lib -lssl -lcrypto -lpthread -lrt -ldl $(shell python3-config --ldflags) -lpython3.10

LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
CFLAGS 		+= $(LIBDPDK_CFLAGS)
LDFLAGS 	+= $(LIBDPDK_LDFLAGS)

LIBNFTNL_SRCS	= $(COMMON_DIR)/printk.c netfilter/netfilter.c
LIBNFTNL_OBJS	= $(LIBNFTNL_SRCS:%.c=$(BUILD_DIR)/lib/%.o)
LIBNFTNL	= $(BUILD_DIR)/lib/libnftnl.so

LIBINTERPOSE_SRCS	= interpose/nutcracker.c
LIBINTERPOSE_OBJS	= $(LIBINTERPOSE_SRCS:%.c=$(BUILD_DIR)/lib/%.o)
LIBINTERPOSE		= $(BUILD_DIR)/lib/libnutcracker-interpose.so

all: $(LIBNFTNL) $(LIBINTERPOSE)

C_STANDARD	:= -std=gnu11

MKDIR_P := mkdir -p

$(BUILD_DIR)/lib/%.o: %.c
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) -c $< -o $(@)

$(LIBNFTNL): $(LIBNFTNL_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

$(LIBINTERPOSE): $(LIBINTERPOSE_OBJS)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@$(MKDIR_P) $(dir $@)
	$(LD) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

clean:
	@rm -rf $(BUILD_DIR)