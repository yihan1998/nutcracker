CC := gcc
LD := gcc

SRC_DIR		:= ./
BUILD_DIR	:= ./build
CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC -march=native #$(shell python3-config --includes)

INC 		:= -I/usr/include/ -I./drivers -I./entry -I./include
CFLAGS 		+= $(INC)

LDFLAGS 	:= -L/usr/lib -lpthread -lrt -ldl -L/home/ubuntu/tinycc -ltcc #$(shell python3-config --ldflags)

# DOCA
LIBDOCA_CFLAGS := $(shell pkg-config --cflags doca)
LIBDOCA_LDFLAGS := $(shell pkg-config --libs doca)
CFLAGS 		+= $(LIBDOCA_CFLAGS) -DDOCA_ALLOW_EXPERIMENTAL_API
LDFLAGS 	+= $(LIBDOCA_LDFLAGS)

EXE_SRCS	:= $(shell find ./ -name '*.c' -print) 
EXE_OBJS 	= $(EXE_SRCS:%.c=$(BUILD_DIR)/%.o)
EXE_NAME 	= $(BUILD_DIR)/test_doca_regex

all: $(EXE_NAME) $(LIBAIO) $(LIBNFTNL)

C_STANDARD	:= -std=gnu11

MKDIR_P := mkdir -p

$(BUILD_DIR)/%.o: %.c
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

clean:
	@rm -rf $(BUILD_DIR)
