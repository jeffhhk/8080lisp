#include "i8080_asm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void expect_int(const char *label, int actual, int expected) {
  if (actual == expected) {
    return;
  }
  fprintf(stderr, "%s: expected %d but saw %d\n", label, expected, actual);
  failures += 1;
}

static void expect_u8(const char *label, uint8_t actual, uint8_t expected) {
  if (actual == expected) {
    return;
  }
  fprintf(stderr, "%s: expected 0x%02x but saw 0x%02x\n", label, expected,
          actual);
  failures += 1;
}

static void test_clean_source_assembly_encodes_labels_and_data(void) {
  static const char *program =
      "ORG 0100H\n"
      "START: MVI A,'A'\n"
      "CALL 0F009H\n"
      "LXI H,MESSAGE\n"
      "JMP START\n"
      "MESSAGE: DB 'O','K'\n"
      "DW START\n";
  i8080_image image;
  i8080_asm_error error;

  expect_int("assemble clean source",
             i8080_assemble_text("clean", program, &image, &error), 1);
  expect_int("origin", image.origin, 0x0100);
  expect_int("limit", image.limit, 0x010f);
  expect_u8("mvi opcode", image.bytes[0x0100], 0x3e);
  expect_u8("mvi imm", image.bytes[0x0101], 'A');
  expect_u8("call opcode", image.bytes[0x0102], 0xcd);
  expect_u8("call low", image.bytes[0x0103], 0x09);
  expect_u8("call high", image.bytes[0x0104], 0xf0);
  expect_u8("lxi opcode", image.bytes[0x0105], 0x21);
  expect_u8("message low", image.bytes[0x0106], 0x0b);
  expect_u8("message high", image.bytes[0x0107], 0x01);
  expect_u8("jump opcode", image.bytes[0x0108], 0xc3);
  expect_u8("jump low", image.bytes[0x0109], 0x00);
  expect_u8("jump high", image.bytes[0x010a], 0x01);
  expect_u8("message O", image.bytes[0x010b], 'O');
  expect_u8("message K", image.bytes[0x010c], 'K');
  expect_u8("word low", image.bytes[0x010d], 0x00);
  expect_u8("word high", image.bytes[0x010e], 0x01);
}

static void test_listing_ingest_reconstructs_original_binary(void) {
  i8080_image image;
  i8080_asm_error error;

  expect_int("assemble listing",
             i8080_assemble_file("orig/lisp_8080_rawocr_2026-04-13.asm", &image,
                                 &error),
             1);
  expect_int("listing origin", image.origin, 0x0000);
  expect_int("listing limit", image.limit, 0x06df);
  expect_u8("entry byte 0", image.bytes[0x0000], 0xc3);
  expect_u8("entry byte 1", image.bytes[0x0001], 0xb6);
  expect_u8("entry byte 2", image.bytes[0x0002], 0x04);
  expect_u8("curch zero", image.bytes[0x049c], 0x00);
  expect_u8("ds reserved zero", image.bytes[0x04a8], 0x00);
  expect_u8("start opcode", image.bytes[0x04b6], 0x21);
  expect_u8("asoc fn low", image.bytes[0x06d3], 0x16);
  expect_u8("asoc fn high", image.bytes[0x06d4], 0x01);
}

int main(void) {
  test_clean_source_assembly_encodes_labels_and_data();
  test_listing_ingest_reconstructs_original_binary();

  if (failures != 0) {
    fprintf(stderr, "2 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("2 tests 0 failures 0 skipped");
  return 0;
}
