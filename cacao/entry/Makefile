CC	= gcc

SRC_DIR		:= ..
BUILD_DIR	:= ../build/$(notdir $(shell pwd))

LIBPATH	:= -I/usr/include/ -I$(SRC_DIR)/include/ -I./

C_SRC	:= $(shell find ./ -name '*.c' -print)

OBJECTS := $(C_SRC:%.c=$(BUILD_DIR)/%.o)

all: $(OBJECTS)

MKDIR_P := mkdir -p

C_STANDARD	:= -std=gnu11

$(BUILD_DIR)/%.o: %.c
	@echo Compiling $(<F)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(C_STANDARD) $(CFLAGS) $(LIBPATH) -c $< -o $(@)