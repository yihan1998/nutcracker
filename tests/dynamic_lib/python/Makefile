# Define compiler
CC=gcc

# Compiler flags
CFLAGS=-fPIC -Wall -Wextra -O3 $(shell python3-config --includes)

# Linker flags
LDFLAGS=-shared $(shell python3-config --ldflags)

# Target library name
TARGET_LIB=libdynamic_python.so

# Source files
SRCS=$(shell find $(SRC_DIR) -name '*.c' -print)

# Object files
OBJS=$(SRCS:.c=.o)

# Default target
all: $(TARGET_LIB)

$(TARGET_LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Pattern rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET_LIB)