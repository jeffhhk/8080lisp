COSMOCC := $(HOME)/bin/cosmo/bin/cosmocc
CC := gcc
CFLAGS := -O2 -Wall -Wextra -std=c11 -Isrc -Itests
BUILD_DIR := o
RUNTIME_SRCS := src/lisp_runtime.S
HOST_IO_SRCS := tests/lisp_host_io.c
MAIN_SRC := tests/lisp_main.c
TEST_SRC := tests/lisp_runtime_test.c
LISP_GCC := $(BUILD_DIR)/lisp_gcc
LISP_TEST := $(BUILD_DIR)/lisp_test
LISP_COSMO := $(BUILD_DIR)/lisp_cosmo.com

.PHONY: clean build test lisp-gcc lisp-test lisp-cosmo

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LISP_GCC): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC)

$(LISP_TEST): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(TEST_SRC)

$(LISP_COSMO): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC) | $(BUILD_DIR)
	$(COSMOCC) -O2 -Wall -Wextra -Isrc -Itests -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC)

lisp-gcc: $(LISP_GCC)

lisp-test: $(LISP_TEST)

lisp-cosmo: $(LISP_COSMO)

build: lisp-gcc

test: lisp-test
	./$(LISP_TEST)
