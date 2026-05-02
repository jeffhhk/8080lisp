#include "ocr_validator.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void expect_int(const char *label, int actual, int expected) {
  if (actual == expected) {
    return;
  }
  fprintf(stderr, "%s: expected %d but saw %d\n", label, expected, actual);
  failures += 1;
}

static void expect_contains(const char *label, const char *actual,
                            const char *expected_substring) {
  if (strstr(actual, expected_substring) != NULL) {
    return;
  }
  fprintf(stderr, "%s: expected \"%s\" to contain \"%s\"\n", label, actual,
          expected_substring);
  failures += 1;
}

static void test_validator_accepts_accounted_field_discrepancy(void) {
  static const char *raw_text =
      "0000 C3 03 00       0001 START  JNP  LOOP     OCR COMMENT\n"
      "0003 76             0002 LOOP   HLT\n";
  static const char *corrected_text =
      "0000 C3 03 00       0001 START  JMP  LOOP     TRUE COMMENT\n"
      "0003 76             0002 LOOP   HLT\n";
  static const char *ledger_text =
      "- id: OCR-0001\n"
      "  line_number: 1\n"
      "  address: 0x0000\n"
      "  field: mnemonic\n"
      "  original: JNP\n"
      "  corrected: JMP\n"
      "  evidence: raw listing mnemonic is inconsistent with opcode bytes\n"
      "  basis: instruction-encoding\n"
      "  confidence: high\n"
      "- id: OCR-0002\n"
      "  line_number: 1\n"
      "  address: 0x0000\n"
      "  field: comment\n"
      "  original: OCR COMMENT\n"
      "  corrected: TRUE COMMENT\n"
      "  evidence: corrected transcription review\n"
      "  basis: scan-review\n"
      "  confidence: medium\n";
  ocr_validator_error error;

  expect_int("accounted discrepancy validates",
             ocr_validate_texts("raw", raw_text, "corrected", corrected_text,
                                "ledger", ledger_text, &error),
             1);
}

static void test_validator_rejects_unaccounted_discrepancy(void) {
  static const char *raw_text =
      "0000 C3 03 00       0001 START  JNP  LOOP\n";
  static const char *corrected_text =
      "0000 C3 03 00       0001 START  JMP  LOOP\n";
  static const char *ledger_text = "";
  ocr_validator_error error;

  expect_int("unaccounted discrepancy fails",
             ocr_validate_texts("raw", raw_text, "corrected", corrected_text,
                                "ledger", ledger_text, &error),
             0);
  expect_contains("unaccounted message", error.message,
                  "unaccounted discrepancy");
}

static void test_validator_rejects_stale_provenance_entry(void) {
  static const char *text =
      "0000 C3 03 00       0001 START  JMP  LOOP\n";
  static const char *ledger_text =
      "- id: OCR-0003\n"
      "  line_number: 1\n"
      "  address: 0x0000\n"
      "  field: mnemonic\n"
      "  original: JNP\n"
      "  corrected: JMP\n"
      "  evidence: stale test fixture\n"
      "  basis: instruction-encoding\n"
      "  confidence: low\n";
  ocr_validator_error error;

  expect_int("stale provenance fails",
             ocr_validate_texts("raw", text, "corrected", text, "ledger",
                                ledger_text, &error),
             0);
  expect_contains("stale provenance message", error.message,
                  "stale provenance entry");
}

static void test_validator_rejects_invalid_basis_value(void) {
  static const char *raw_text =
      "0000 C3 03 00       0001 START  JNP  LOOP\n";
  static const char *corrected_text =
      "0000 C3 03 00       0001 START  JMP  LOOP\n";
  static const char *ledger_text =
      "- id: OCR-0004\n"
      "  line_number: 1\n"
      "  address: 0x0000\n"
      "  field: mnemonic\n"
      "  original: JNP\n"
      "  corrected: JMP\n"
      "  evidence: test fixture\n"
      "  basis: opcode-proof\n"
      "  confidence: high\n";
  ocr_validator_error error;

  expect_int("invalid basis fails",
             ocr_validate_texts("raw", raw_text, "corrected", corrected_text,
                                "ledger", ledger_text, &error),
             0);
  expect_contains("invalid basis message", error.message,
                  "invalid provenance basis");
}

static void test_validator_ignores_address_only_change(void) {
  static const char *raw_text =
      "0000 C3 03 00       0001 START  JMP  LOOP\n";
  static const char *corrected_text =
      "0003 C3 03 00       0001 START  JMP  LOOP\n";
  static const char *ledger_text = "";
  ocr_validator_error error;

  expect_int("address-only change ignored",
             ocr_validate_texts("raw", raw_text, "corrected", corrected_text,
                                "ledger", ledger_text, &error),
             1);
}

static void test_validator_accepts_block_byte_run(void) {
  static const char *raw_text =
      "0000 C3 B6 04       0011 CADDR  CALL CDR\n"
      "0003 CD 10 00       0012 CADR   CALL CDR\n"
      "0006 CD 10 00       0013 CAR    PUSH PSW\n"
      "0009 F5             0014 CAR2   MOV  A,M\n";
  static const char *corrected_text =
      "0003 CD 10 00       0011 CADDR  CALL CDR\n"
      "0006 CD 10 00       0012 CADR   CALL CDR\n"
      "0009 F5             0013 CAR    PUSH PSW\n"
      "000A 7E             0014 CAR2   MOV  A,M\n";
  static const char *ledger_text =
      "- id: OCR-0006\n"
      "  line_number: 11\n"
      "  line_number_end: 14\n"
      "  address: 0x0000\n"
      "  address_end: 0x0009\n"
      "  field: bytes\n"
      "  original: \"C3 B6 04 | CD 10 00 | F5\"\n"
      "  corrected: \"CD 10 00 | F5 | 7E\"\n"
      "  evidence: shifted OCR block changed the machine code associated with these source lines\n"
      "  basis: duplicate-pattern\n"
      "  confidence: high\n";
  ocr_validator_error error;

  expect_int("block byte run validates",
             ocr_validate_texts("raw", raw_text, "corrected", corrected_text,
                                "ledger", ledger_text, &error),
             1);
}

static void test_validator_accepts_bytes_only_to_blank_discrepancy(void) {
  static const char *raw_text =
      "0000 76\n";
  static const char *corrected_text =
      "\n";
  static const char *ledger_text =
      "- id: OCR-0005\n"
      "  line_number: 1\n"
      "  address: 0x0000\n"
      "  field: whole_line\n"
      "  original: 0000 76\n"
      "  corrected: ''\n"
      "  evidence: corrected listing intentionally drops a bytes-only OCR fragment\n"
      "  basis: scan-review\n"
      "  confidence: high\n";
  ocr_validator_error error;

  expect_int("bytes-only to blank validates",
             ocr_validate_texts("raw", raw_text, "corrected", corrected_text,
                                "ledger", ledger_text, &error),
             1);
}

static void test_validator_accepts_repository_placeholder_files(void) {
  ocr_validator_error error;

  expect_int("repository files validate",
             ocr_validate_files("orig/lisp_8080_rawocr_2026-04-13.asm",
                                "src/lisp_8080_corrected.asm",
                                "docs/OCR_CORRECTIONS.yaml", &error),
             1);
}

int main(void) {
  test_validator_accepts_accounted_field_discrepancy();
  test_validator_rejects_unaccounted_discrepancy();
  test_validator_rejects_stale_provenance_entry();
  test_validator_rejects_invalid_basis_value();
  test_validator_ignores_address_only_change();
  test_validator_accepts_block_byte_run();
  test_validator_accepts_bytes_only_to_blank_discrepancy();
  test_validator_accepts_repository_placeholder_files();

  if (failures != 0) {
    fprintf(stderr, "8 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("8 tests 0 failures 0 skipped");
  return 0;
}
