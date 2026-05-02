COSMOCC := $(HOME)/bin/cosmo/bin/cosmocc
CC := gcc
CFLAGS := -O2 -Wall -Wextra -std=c11 -Isrc -Itests
BUILD_DIR := o
RUNTIME_SRCS := src/lisp_runtime.S
ASM_SRCS := src/i8080_asm.c
EMU_SRCS := src/i8080_emu.c
HOST_IO_SRCS := tests/lisp_host_io.c
MAIN_SRC := tests/lisp_main.c
TEST_SRC := tests/lisp_runtime_test.c
ASM_MAIN_SRC := src/i8080asm_main.c
ASM_TEST_SRC := tests/i8080_asm_test.c
EMU_MAIN_SRC := src/i8080emu_main.c
EMU_TEST_SRC := tests/i8080_emu_test.c
LISP_GCC := $(BUILD_DIR)/lisp_gcc
LISP_TEST := $(BUILD_DIR)/lisp_test
I8080_ASM := $(BUILD_DIR)/i8080asm
I8080_ASM_TEST := $(BUILD_DIR)/i8080_asm_test
I8080_EMU := $(BUILD_DIR)/i8080emu
I8080_EMU_TEST := $(BUILD_DIR)/i8080_emu_test
LISP_COSMO := $(BUILD_DIR)/lisp_cosmo.com

.PHONY: clean build test lisp-gcc lisp-test lisp-cosmo i8080asm i8080emu

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LISP_GCC): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC)

$(LISP_TEST): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(TEST_SRC)

$(I8080_ASM): $(ASM_SRCS) $(ASM_MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(ASM_MAIN_SRC)

$(I8080_ASM_TEST): $(ASM_SRCS) $(ASM_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(ASM_TEST_SRC)

$(I8080_EMU): $(ASM_SRCS) $(EMU_SRCS) $(EMU_MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(EMU_MAIN_SRC)

$(I8080_EMU_TEST): $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(EMU_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(EMU_TEST_SRC)

$(LISP_COSMO): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC) | $(BUILD_DIR)
	$(COSMOCC) -O2 -Wall -Wextra -Isrc -Itests -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC)

lisp-gcc: $(LISP_GCC)

lisp-test: $(LISP_TEST)

i8080asm: $(I8080_ASM)

i8080emu: $(I8080_EMU)

lisp-cosmo: $(LISP_COSMO)

build: lisp-gcc i8080asm i8080emu

test: lisp-test $(I8080_ASM_TEST) $(I8080_EMU_TEST)
	./$(LISP_TEST)
	./$(I8080_ASM_TEST)
	./$(I8080_EMU_TEST)
