CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -Isrc -Itests
BUILD_DIR = o
LISTING_SRCS := src/i8080_listing.c
SOURCE_DISPLAY_SRCS := src/i8080_coverage_source_display.c
RUNTIME_SRCS := src/lisp_runtime.S
ASM_SRCS := src/i8080_asm.c $(LISTING_SRCS) $(SOURCE_DISPLAY_SRCS)
EMU_SRCS := src/i8080_emu.c
OCR_VALIDATOR_SRCS := src/ocr_validator.c $(LISTING_SRCS)
HOST_IO_SRCS := tests/lisp_host_io.c
MAIN_SRC := tests/lisp_main.c
TEST_SRC := tests/lisp_runtime_test.c
ASM_MAIN_SRC := src/i8080asm_main.c
ASM_TEST_SRC := tests/i8080_asm_test.c
EMU_MAIN_SRC := src/i8080emu_main.c
EMU_TEST_SRC := tests/i8080_emu_test.c
INTERPRETER_TEST_SRC := tests/lisp_interpreter_test.c
OCR_VALIDATOR_MAIN_SRC := src/ocr_validator_main.c
OCR_VALIDATOR_TEST_SRC := tests/ocr_validator_test.c
LISP_GCC := $(BUILD_DIR)/lisp_gcc
LISP_TEST := $(BUILD_DIR)/lisp_test
I8080_ASM := $(BUILD_DIR)/i8080asm
I8080_ASM_TEST := $(BUILD_DIR)/i8080_asm_test
I8080_EMU := $(BUILD_DIR)/i8080emu
I8080_EMU_TEST := $(BUILD_DIR)/i8080_emu_test
LISP_INTERPRETER_TEST := $(BUILD_DIR)/lisp_interpreter_test
OCR_VALIDATOR := $(BUILD_DIR)/ocr_validator
OCR_VALIDATOR_TEST := $(BUILD_DIR)/ocr_validator_test

.PHONY: clean build build-cosmo test lisp-gcc lisp-test i8080asm i8080emu ocr-validator

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

$(LISP_INTERPRETER_TEST): $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(INTERPRETER_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(INTERPRETER_TEST_SRC)

$(OCR_VALIDATOR): $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_MAIN_SRC)

$(OCR_VALIDATOR_TEST): $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_TEST_SRC)

lisp-gcc: $(LISP_GCC)

lisp-test: $(LISP_TEST)

i8080asm: $(I8080_ASM)

i8080emu: $(I8080_EMU)

ocr-validator: $(OCR_VALIDATOR)

build: lisp-gcc i8080asm i8080emu ocr-validator

build-cosmo:
	$(MAKE) clean
	env PATH="$$PATH:$(HOME)/bin/cosmo/bin" $(MAKE) build CC=x86_64-unknown-cosmo-cc
	mv o/i8080emu o/i8080emu.zip
	zip o/i8080emu \
		Makefile docs/BUILD.md docs/DEVELOPMENT_LOG.yaml docs/IMPLEMENTATION_BACKLOG.yaml docs/IMPLEMENTATION_PLAN.md \
		docs/OCR_CORRECTIONS.yaml docs/Reference.md docs/a_lisp_interpreter_for_the_8080__van_Buer__Dr_Dobbs__1978.pdf \
		orig/lisp_8080_rawocr_2026-04-13.asm src/i8080_asm.c src/i8080_asm.h src/i8080_coverage_source_display.c \
		src/i8080_coverage_source_display.h src/i8080_emu.c src/i8080_emu.h src/i8080_listing.c src/i8080_listing.h \
		src/i8080asm_main.c src/i8080emu_main.c src/lisp_8080_corrected.asm src/lisp_runtime.S src/lisp_runtime.h \
		src/ocr_validator.c src/ocr_validator.h src/ocr_validator_main.c tests/i8080_asm_test.c tests/i8080_emu_test.c \
		tests/lisp_host_io.c tests/lisp_host_io.h tests/lisp_interpreter_test.c tests/lisp_main.c tests/lisp_runtime_test.c \
		tests/ocr_validator_test.c
	mv o/i8080emu.zip  o/i8080emu

test: lisp-test $(I8080_EMU) $(I8080_ASM_TEST) $(I8080_EMU_TEST) $(LISP_INTERPRETER_TEST) $(OCR_VALIDATOR_TEST)
	./$(LISP_TEST)
	./$(I8080_ASM_TEST)
	./$(I8080_EMU_TEST)
	./$(LISP_INTERPRETER_TEST)
	./$(OCR_VALIDATOR_TEST)
