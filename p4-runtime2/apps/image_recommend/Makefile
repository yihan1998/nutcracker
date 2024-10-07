# Compiler
CC = gcc

# Python configuration
PYTHON_CFLAGS = $(shell python3-config --cflags)
PYTHON_LDFLAGS = $(shell python3-config --ldflags)

# Compiler flags
CFLAGS = -I../../include $(PYTHON_CFLAGS) -DLATENCY_BREAKDOWN
LDFLAGS = -L../../build/lib $(PYTHON_LDFLAGS) -lpython3.10

# Library name
# LIBRARY = image_recommend_wrapper.so
TARGET = image_recommend

# Source files
SRCS = image_recommend_wrapper.c

# Object files
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to build the object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
