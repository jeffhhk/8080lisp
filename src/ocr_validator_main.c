#include "ocr_validator.h"

#include <stdio.h>

int main(int argc, char **argv) {
  static const char *default_raw = "orig/lisp_8080_rawocr_2026-04-13.asm";
  static const char *default_corrected = "src/lisp_8080_corrected.asm";
  static const char *default_ledger = "docs/OCR_CORRECTIONS.yaml";
  const char *raw_path = default_raw;
  const char *corrected_path = default_corrected;
  const char *ledger_path = default_ledger;
  ocr_validator_error error;

  if (argc != 1 && argc != 4) {
    fprintf(stderr,
            "usage: %s [raw.asm corrected.asm corrections.yaml]\n",
            argv[0]);
    return 1;
  }
  if (argc == 4) {
    raw_path = argv[1];
    corrected_path = argv[2];
    ledger_path = argv[3];
  }

  if (!ocr_validate_files(raw_path, corrected_path, ledger_path, &error)) {
    if (error.line_number != 0) {
      fprintf(stderr, "%s:%zu: %s\n", ledger_path, error.line_number,
              error.message);
    } else {
      fprintf(stderr, "%s\n", error.message);
    }
    return 1;
  }

  puts("OCR provenance validated");
  return 0;
}
