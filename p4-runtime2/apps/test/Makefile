# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -O3 -I../../src/include -g $(shell python3-config --includes)

# Linker flags for creating shared library
LDFLAGS = -L../../src/build/lib $(shell python3-config --ldflags) -lpython3.10

# Source files=
SRCS=$(shell find $(SRC_DIR) -name '*.c' -print)

# Object files
OBJS = $(SRCS:.c=.o)

# Output library name
TARGET = test

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -f $(OBJS) $(LIBRARY)

# Phony targets
.PHONY: all clean
