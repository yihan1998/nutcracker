# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -fPIC

# Linker flags for creating shared library
LDFLAGS = -shared

# Source files=
SRCS=$(shell find $(SRC_DIR) -name '*.c' -print)

# Object files
OBJS = $(SRCS:.c=.o)

# Output library name
LIBRARY = libfile_verify.so

# Default target
all: $(LIBRARY)

# Rule to build the shared library
$(LIBRARY): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Rule to build object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -f $(OBJS) $(LIBRARY)

# Phony targets
.PHONY: all clean
