COSMOCC := $(HOME)/bin/cosmo/bin/cosmocc
CC := gcc
BUILD_DIR := o
HELLO_SRC := src/hello_world.c
HELLO_GCC := $(BUILD_DIR)/hello_gcc
HELLO_COSMO := $(BUILD_DIR)/hello_cosmo.com

.PHONY: clean build test hello-gcc hello-cosmo

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(HELLO_GCC): $(HELLO_SRC) | $(BUILD_DIR)
	$(CC) -O2 -Wall -Wextra -o $@ $<

$(HELLO_COSMO): $(HELLO_SRC) | $(BUILD_DIR)
	$(COSMOCC) -O2 -Wall -Wextra -o $@ $<

hello-gcc: $(HELLO_GCC)

hello-cosmo: $(HELLO_COSMO)

build: hello-gcc

test: hello-gcc
	./$(HELLO_GCC)
