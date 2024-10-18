# Define variables
CC = clang
CFLAGS = -emit-llvm -c -mcpu=cortex-a72 -DALLOW_EXPERIMENTAL_API
INCLUDES = -I./ \
            -I/opt/mellanox/doca/include \
            -I/opt/mellanox/flexio/include \
            -I/opt/mellanox/dpdk/include/dpdk \
            -I/opt/mellanox/dpdk/include/dpdk/../aarch64-linux-gnu/dpdk \
            -I/opt/mellanox/dpdk/include/dpdk \
            -I/usr/include/json-c \
            -I/usr/include/libnl3 \
            -I/usr/include/glib-2.0 \
            -I/usr/lib/aarch64-linux-gnu/glib-2.0/include

# Source files
SRC_INGRESS = ingress.c

# Target files
TARGET_INGRESS = ingress.bc

# Default target
all: $(TARGET_INGRESS)

# Rule for building ingress
$(TARGET_INGRESS): $(SRC_INGRESS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

# Clean up build artifacts
clean:
	rm -f $(TARGET_INGRESS)