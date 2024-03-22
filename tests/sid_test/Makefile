# Define the compiler
CC = gcc
EMCC = /local/yihan/emsdk/upstream/emscripten/emcc

# Compiler flags
CFLAGS = -O3 -g -I../../sid
LDFLAGS = -L../../sid -lsid

# The default target
all: sid_test sid_test_prog.wasm

sid_test: sid_test.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

sid_test_prog.wasm: sid_test_prog.c
	$(EMCC) -I../../sid $< -o $@

# Rule for cleaning up
clean:
	rm -f sid_test sid_test_prog.wasm