# Makefile for lock_bench

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -O3
LDFLAGS = -lbenchmark -lpthread

# Source and output files
SRC = lock_bench.cpp
OUT = lock_bench

# Default target
all: $(OUT)

# Rule to compile the output
$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

# Clean up build files
clean:
	rm -f $(OUT)

# Phony targets
.PHONY: all clean
