CC := gcc
LD := gcc

COMMON_DIR	:= ../../src/common
INC_DIR		:= ../../src/include

BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG

CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC -march=native $(DEBUG)
CFLAGS 		+= -I/usr/include/ -I$(INC_DIR)

LDFLAGS 	:= -L/usr/lib -lssl -lcrypto -lpthread -lrt -ldl

LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
CFLAGS 		+= $(LIBDPDK_CFLAGS)
LDFLAGS 	+= $(LIBDPDK_LDFLAGS)

SRCS	= $(COMMON_DIR)/printk.c netfilter.c
OBJS	= $(SRCS:%.c=$(BUILD_DIR)/%.o)
TARGET	= $(BUILD_DIR)/libnftnl.so

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