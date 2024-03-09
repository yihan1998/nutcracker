CC := gcc
CFLAGS := -Wall -g
SRCDIR := src
BINDIR := bin
SOURCES := $(shell find $(SRCDIR) -type f -name '*.c')
TARGETS := $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%,$(SOURCES))

.PHONY: all clean

all: $(TARGETS)

$(BINDIR)/%: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BINDIR)/*
