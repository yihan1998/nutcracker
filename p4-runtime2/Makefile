CXX_CLANG = clang++
CC 		= gcc

DEBUG	= -DDEBUG
CXXFLAGS = -Wall -Werror -O3 -g  -I./ -I./include $(DEBUG)
CXXFLAGS_CLANG = $(CXXFLAGS)

CFLAGS = -Wall -Werror -O3 -g -std=gnu11 -I./ -I./include -I./common/entry $(DEBUG)
CFLAGS_GCC = $(CFLAGS)

LLVM_CXXFLAGS := $(shell llvm-config --cxxflags) -I./common/entry
LLVM_LDFLAGS := $(shell llvm-config --ldflags --system-libs --libs core orcjit native)

CXXFLAGS_CLANG += $(LLVM_CXXFLAGS)
LDFLAGS += $(LLVM_LDFLAGS)

SUBDIRS	:= flax/ pistachIO/ cacao/ common/ init/

# Configurations
CONFIG 	= .config
include ${CONFIG}

ifdef CONFIG_NR_CPUS
	CFLAGS	+= -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif

ifeq ($(CONFIG_BLUEFIELD2),y)
	CXXFLAGS_CLANG	+= -DCONFIG_BLUEFIELD2
	CFLAGS_GCC		+= -DCONFIG_BLUEFIELD2

	LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
	LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
	CXXFLAGS_CLANG += $(LIBDPDK_CFLAGS)
	CFLAGS_GCC += $(LIBDPDK_CFLAGS)
	LDFLAGS += $(LIBDPDK_LDFLAGS)

	ifeq ($(CONFIG_DOCA),y)
		SUBDIRS	+= drivers/doca
		CXXFLAGS_CLANG	+= -DCONFIG_DOCA
		CFLAGS_GCC	+= -DCONFIG_DOCA

		LIBDOCA_CFLAGS := $(shell pkg-config --cflags doca)
		LIBDOCA_LDFLAGS := $(shell pkg-config --libs doca)
		CXXFLAGS_CLANG += $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
		CFLAGS_GCC += $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
		LDFLAGS += $(LIBDOCA_LDFLAGS)
		ifeq ($(CONFIG_DOCA_REGEX),y)
			CFLAGS	+= -DCONFIG_DOCA_REGEX
		endif
		ifeq ($(CONFIG_DOCA_COMPRESS),y)
			CFLAGS	+= -DCONFIG_DOCA_COMPRESS
		endif
		ifeq ($(CONFIG_DOCA_FLOW),y)
			CFLAGS	+= -DCONFIG_DOCA_FLOW
		endif
	endif
else ifeq ($(CONFIG_BLUEFIELD3),y)
	CXXFLAGS_CLANG	+= -DCONFIG_BLUEFIELD3
	CFLAGS_GCC		+= -DCONFIG_BLUEFIELD3

	LIBDPDK_CFLAGS := $(shell pkg-config --cflags libdpdk)
	LIBDPDK_LDFLAGS := $(shell pkg-config --libs libdpdk)
	CXXFLAGS_CLANG += $(LIBDPDK_CFLAGS)
	CFLAGS_GCC += $(LIBDPDK_CFLAGS)
	LDFLAGS += $(LIBDPDK_LDFLAGS)

	ifeq ($(CONFIG_DOCA),y)
		LIBDOCA_CFLAGS := $(shell pkg-config --cflags doca-common)
		LIBDOCA_CFLAGS += $(shell pkg-config --cflags doca-flow)
		LIBDOCA_LDFLAGS := $(shell pkg-config --libs doca-common)
		LIBDOCA_LDFLAGS += $(shell pkg-config --libs doca-flow)
		CXXFLAGS_CLANG += $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
		CFLAGS_GCC += $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
		LDFLAGS += $(LIBDOCA_LDFLAGS)

		ifeq ($(CONFIG_DOCA_COMPRESS),y)
			LIBDOCA_CFLAGS := $(shell pkg-config --cflags doca-compress)
			LIBDOCA_LDFLAGS := $(shell pkg-config --libs doca-compress)
			CXXFLAGS_CLANG += $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
			CFLAGS_GCC += $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
			LDFLAGS += $(LIBDOCA_LDFLAGS)
			CFLAGS	+= -DCONFIG_DOCA_COMPRESS
		endif
	endif
endif

# Define source and object files
SRC_CLANG 		= $(shell find ./ -name '*.cpp' -print)
SRC_GCC 		= $(shell find $(SUBDIRS) -name '*.c' -print)

SRC_NFTNL		= ./common/printk.c $(shell find ./lib/netfilter -name '*.c' -print)
OBJ_NFTNL		= $(SRC_NFTNL:%.c=$(BUILD_DIR)/%.o)
NFTNL			= libnftnl.so

SRC_INTERPOSE	= $(shell find ./common -name '*.c' -print) $(shell find ./drivers/doca -name '*.c' -print) $(shell find ./lib/interpose -name '*.c' -print)
OBJ_INTERPOSE	= $(SRC_INTERPOSE:%.c=$(BUILD_DIR)/lib/%.o)
INTERPOSE		= /lib/libnutcracker-interpose.so

# Define build directory
BUILD_DIR = build

# Define object files in the build/ directory
OBJ_CLANG = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC_CLANG))
OBJ_GCC = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_GCC))

LIB_CFLAGS	:= $(CFLAGS) -fPIC

# Define target
TARGET = nutcracker

# Default target
all: $(BUILD_DIR) $(OBJ_GCC) $(OBJ_CLANG) $(BUILD_DIR)/$(TARGET) $(BUILD_DIR)/$(INTERPOSE) #$(BUILD_DIR)/$(NFTNL)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Ensure build directory and subdirectories exist before compiling
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_CLANG) $(CXXFLAGS_CLANG) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_GCC) -c $< -o $@

$(BUILD_DIR)/lib/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_GCC) $(LIB_CFLAGS) -c $< -o $@

# Link object files and create the executable in the build/ directory
$(BUILD_DIR)/$(TARGET): $(OBJ_CLANG) $(OBJ_GCC)
	$(CXX_CLANG) $(OBJ_CLANG) $(OBJ_GCC) -o $(BUILD_DIR)/$(TARGET) $(LDFLAGS)

$(BUILD_DIR)/$(NFTNL): $(OBJ_NFTNL)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@mkdir -p $(dir $@)
	$(CC) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

# $(BUILD_DIR)/$(NFTNL): $(OBJ_NFTNL)
# 	@echo ' '
# 	@echo 'Building $(@)'
# 	@echo 'Invoking Linker'
# 	@mkdir -p $(dir $@)
# 	$(CC) -o $(@) $^ $(LDFLAGS) -shared
# 	@echo 'Finished building: $@'
# 	@echo ' '

$(BUILD_DIR)/$(INTERPOSE): $(OBJ_INTERPOSE)
	@echo ' '
	@echo 'Building $(@)'
	@echo 'Invoking Linker'
	@mkdir -p $(dir $@)
	$(CC) -o $(@) $^ $(LDFLAGS) -shared
	@echo 'Finished building: $@'
	@echo ' '

# Clean up object files and the target in the build/ directory
clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/$(TARGET)
	rm -rf $(BUILD_DIR)