# Define compiler
CC=gcc

# Compiler flags
CFLAGS=-fPIC -Wall -Wextra -O3 -I../../pistachIO/include -I../../pistachIO/build/lib/aio -I../../pistachIO/include/linux

# Linker flags
LDFLAGS=-shared -L../../pistachIO/build/lib -lnftnl -lmnl

# Target library name
TARGET=netfilter_dns_filter.so

# Source files
SRCS=$(shell find $(SRC_DIR) -name '*.c' -print)

# Object files
OBJS=$(SRCS:.c=.o)

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Pattern rule for object files
$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET) 