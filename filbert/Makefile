CC := gcc

SRC_DIR		:= ./
BUILD_DIR	:= ./build
DEBUG		:= -DDEBUG
CFLAGS 		:= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC $(DEBUG)
LIBPATH 	:= -I/usr/include/ -I./include
CFLAGS 		+= $(LIBPATH)

TARGET		:= $(BUILD_DIR)/filbert

LDFLAGS 	:= -L/usr/lib -lpthread -lrt

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
