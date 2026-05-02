CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11 -Isrc -Itests -I$(BUILD_DIR)
BUILD_DIR = o
BINEXT ?=
COSMO_PATH ?= $(HOME)/bin/cosmo/bin
COSMO_CC ?= x86_64-unknown-cosmo-cc
COSMO_BINEXT ?= .com
EMSDK_DIR ?= $(CURDIR)/toolchains/emsdk
EMSDK_VERSION ?= latest
EMCC_ENV = EMSDK_DIR="$(EMSDK_DIR)" EMSDK_VERSION="$(EMSDK_VERSION)"
LISTING_SRCS := src/i8080_listing.c
SOURCE_DISPLAY_SRCS := src/i8080_coverage_source_display.c
RUNTIME_SRCS := src/lisp_runtime.S
RUNTIME_EMSCRIPTEN_SRCS := src/lisp_runtime_emscripten.c
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
WEB_APP_SRCS := src/i8080_web_app.c
WEB_MAIN_SRC := src/i8080web_main.c
WEB_TEST_SRC := tests/i8080_web_test.c
WEB_SHELL := web/i8080web_shell.html
WEB_EMBED_HEADER := $(BUILD_DIR)/lisp_8080_corrected_program.h
TEXT_TO_HEADER := tools/text_to_c_header.py
COSMO_EMBED_FILES := \
	Makefile docs/BUILD.md docs/DEVELOPMENT_LOG.yaml docs/IMPLEMENTATION_BACKLOG.yaml docs/IMPLEMENTATION_PLAN.md \
	docs/OCR_CORRECTIONS.yaml docs/Reference.md docs/a_lisp_interpreter_for_the_8080__van_Buer__Dr_Dobbs__1978.pdf \
	orig/lisp_8080_rawocr_2026-04-13.asm src/i8080_asm.c src/i8080_asm.h src/i8080_coverage_source_display.c \
	src/i8080_coverage_source_display.h src/i8080_emu.c src/i8080_emu.h src/i8080_listing.c src/i8080_listing.h \
	src/i8080_web_app.c src/i8080_web_app.h src/i8080asm_main.c src/i8080emu_main.c src/i8080web_main.c \
	src/lisp_8080_corrected.asm src/lisp_runtime.S src/lisp_runtime.h src/lisp_runtime_emscripten.c \
	src/ocr_validator.c src/ocr_validator.h src/ocr_validator_main.c tests/i8080_asm_test.c tests/i8080_emu_test.c \
	tests/i8080_web_test.c tests/lisp_host_io.c tests/lisp_host_io.h tests/lisp_interpreter_test.c tests/lisp_main.c \
	tests/lisp_runtime_test.c tests/ocr_validator_test.c tools/text_to_c_header.py web/i8080web_shell.html \
	ensure_emsdk_installed.sh ensure_emsdk_uninstalled.sh \
	ensure_emsdk_installed.sh ensure_emsdk_uninstalled.sh src/i8080_web_app.c src/i8080_web_app.h \
	src/i8080web_main.c src/lisp_runtime_emscripten.c tests/i8080_web_test.c tools/text_to_c_header.py \
	web/i8080web_shell.html
LISP_GCC := $(BUILD_DIR)/lisp_gcc
LISP_TEST := $(BUILD_DIR)/lisp_test
LISP_EMSCRIPTEN_RUNTIME_TEST := $(BUILD_DIR)/lisp_runtime_emscripten_test
I8080_ASM := $(BUILD_DIR)/i8080asm
I8080_ASM_TEST := $(BUILD_DIR)/i8080_asm_test
I8080_EMU := $(BUILD_DIR)/i8080emu
I8080_EMU_TEST := $(BUILD_DIR)/i8080_emu_test
I8080_WEB_TEST := $(BUILD_DIR)/i8080_web_test
LISP_INTERPRETER_TEST := $(BUILD_DIR)/lisp_interpreter_test
OCR_VALIDATOR := $(BUILD_DIR)/ocr_validator
OCR_VALIDATOR_TEST := $(BUILD_DIR)/ocr_validator_test
I8080_COSMO := $(BUILD_DIR)/8080LISP$(BINEXT)
I8080_WEB := $(BUILD_DIR)/i8080web.html

.PHONY: clean build build-cosmo build-cosmo-inner package-cosmo test lisp-gcc lisp-test i8080asm i8080emu i8080web ocr-validator

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LISP_GCC): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(MAIN_SRC)

$(LISP_TEST): $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_SRCS) $(HOST_IO_SRCS) $(TEST_SRC)

$(LISP_EMSCRIPTEN_RUNTIME_TEST): $(RUNTIME_EMSCRIPTEN_SRCS) $(HOST_IO_SRCS) $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(RUNTIME_EMSCRIPTEN_SRCS) $(HOST_IO_SRCS) $(TEST_SRC)

$(I8080_ASM): $(ASM_SRCS) $(ASM_MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(ASM_MAIN_SRC)

$(I8080_ASM_TEST): $(ASM_SRCS) $(ASM_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(ASM_TEST_SRC)

$(I8080_EMU): $(ASM_SRCS) $(EMU_SRCS) $(EMU_MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(EMU_MAIN_SRC)

$(I8080_EMU_TEST): $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(EMU_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(EMU_TEST_SRC)

$(WEB_EMBED_HEADER): src/lisp_8080_corrected.asm $(TEXT_TO_HEADER) | $(BUILD_DIR)
	python3 $(TEXT_TO_HEADER) --input $< --output $@ --variable i8080web_embedded_program

$(I8080_WEB_TEST): $(ASM_SRCS) $(EMU_SRCS) $(WEB_APP_SRCS) $(RUNTIME_EMSCRIPTEN_SRCS) $(HOST_IO_SRCS) $(WEB_TEST_SRC) $(WEB_EMBED_HEADER) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(WEB_APP_SRCS) $(RUNTIME_EMSCRIPTEN_SRCS) $(HOST_IO_SRCS) $(WEB_TEST_SRC)

$(LISP_INTERPRETER_TEST): $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(INTERPRETER_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(HOST_IO_SRCS) $(INTERPRETER_TEST_SRC)

$(OCR_VALIDATOR): $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_MAIN_SRC)

$(OCR_VALIDATOR_TEST): $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(OCR_VALIDATOR_SRCS) $(OCR_VALIDATOR_TEST_SRC)

$(I8080_WEB): $(ASM_SRCS) $(EMU_SRCS) $(WEB_APP_SRCS) $(WEB_MAIN_SRC) $(RUNTIME_EMSCRIPTEN_SRCS) $(WEB_EMBED_HEADER) $(WEB_SHELL) ensure_emsdk_installed.sh | $(BUILD_DIR)
	$(EMCC_ENV) ./ensure_emsdk_installed.sh
	$(EMCC_ENV) bash -lc '. "$(EMSDK_DIR)/emsdk_env.sh" >/dev/null && emcc -O2 -Wall -Wextra -std=c11 -Isrc -Itests -I$(BUILD_DIR) -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web -sEXIT_RUNTIME=0 -sSINGLE_FILE=1 -sEXPORTED_FUNCTIONS=_main,_i8080web_eval,_i8080web_restart -sEXPORTED_RUNTIME_METHODS=ccall,cwrap --shell-file $(WEB_SHELL) -o $@ $(ASM_SRCS) $(EMU_SRCS) $(WEB_APP_SRCS) $(WEB_MAIN_SRC) $(RUNTIME_EMSCRIPTEN_SRCS)'

lisp-gcc: $(LISP_GCC)

lisp-test: $(LISP_TEST)

i8080asm: $(I8080_ASM)

i8080emu: $(I8080_EMU)

i8080web: $(I8080_WEB)

ocr-validator: $(OCR_VALIDATOR)

build: lisp-gcc i8080asm i8080emu ocr-validator

build-cosmo:
	$(MAKE) clean
	env PATH="$$PATH:$(COSMO_PATH)" $(MAKE) build-cosmo-inner CC=$(COSMO_CC) BINEXT=$(COSMO_BINEXT)

build-cosmo-inner: build package-cosmo

package-cosmo: $(I8080_EMU)
	mv $(I8080_EMU) $(I8080_EMU).zip
	zip $(I8080_EMU).zip $(COSMO_EMBED_FILES)
	mv $(I8080_EMU).zip $(I8080_EMU)

package-web: $(I8080_WEB)

test: lisp-test $(LISP_EMSCRIPTEN_RUNTIME_TEST) $(I8080_EMU) $(I8080_ASM_TEST) $(I8080_EMU_TEST) $(I8080_WEB_TEST) $(LISP_INTERPRETER_TEST) $(OCR_VALIDATOR_TEST)
	./$(LISP_TEST)
	./$(LISP_EMSCRIPTEN_RUNTIME_TEST)
	./$(I8080_ASM_TEST)
	./$(I8080_EMU_TEST)
	./$(I8080_WEB_TEST)
	./$(LISP_INTERPRETER_TEST)
	./$(OCR_VALIDATOR_TEST)
