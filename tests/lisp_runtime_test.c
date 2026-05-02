#include "lisp_host_io.h"
#include "lisp_runtime.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void expect_u8(const char *label, uint8_t actual, uint8_t expected) {
  if (actual == expected) {
    return;
  }

  fprintf(stderr, "%s: expected 0x%02x but saw 0x%02x\n", label, expected,
          actual);
  failures += 1;
}

static void expect_u16(const char *label, uint16_t actual, uint16_t expected) {
  if (actual == expected) {
    return;
  }

  fprintf(stderr, "%s: expected 0x%04x but saw 0x%04x\n", label, expected,
          actual);
  failures += 1;
}

static void expect_text(const char *label, const char *actual,
                        const char *expected) {
  if (strcmp(actual, expected) == 0) {
    return;
  }

  fprintf(stderr, "%s: expected \"%s\" but saw \"%s\"\n", label, expected,
          actual);
  failures += 1;
}

static uint16_t read_u16(const uint8_t *memory, unsigned offset) {
  return (uint16_t)(memory[offset] | ((uint16_t)memory[offset + 1] << 8));
}

static void test_boot_initializes_original_memory_layout(void) {
  const uint8_t *memory;

  lisp_host_reset();
  lisp_boot();
  memory = lisp_memory_base();

  expect_u16("CVTFQ", read_u16(memory, 0x04ae), 0x0700);
  expect_u8("free queue next low", memory[0x0700], 0xff);
  expect_u8("free queue next high", memory[0x0701], 0xff);
  expect_u8("free queue length low", memory[0x0702], 0x00);
  expect_u8("free queue length high", memory[0x0703], 0x40);
  expect_u16("EVQAL", read_u16(memory, 0x04b4), 0x8000);
}

static void test_monitor_io_shims_round_trip_bytes(void) {
  lisp_host_reset();
  lisp_host_set_input("Az");
  lisp_host_clear_output();

  expect_u8("INCH first", (uint8_t)lisp_inch(), 'A');
  expect_u8("INCH second", (uint8_t)lisp_inch(), 'z');
  expect_u8("INCH eof", (uint8_t)lisp_inch(), 0x00);

  lisp_outc('H');
  lisp_outc('i');
  lisp_crlf();

  expect_text("captured output", lisp_host_output(), "Hi\n");
}

int main(void) {
  test_boot_initializes_original_memory_layout();
  test_monitor_io_shims_round_trip_bytes();

  if (failures != 0) {
    fprintf(stderr, "2 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("2 tests 0 failures 0 skipped");
  return 0;
}
