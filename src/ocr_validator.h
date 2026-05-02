#ifndef OCR_VALIDATOR_H
#define OCR_VALIDATOR_H

#include <stddef.h>

typedef struct {
  size_t line_number;
  char message[160];
} ocr_validator_error;

int ocr_validate_texts(const char *raw_name, const char *raw_text,
                       const char *corrected_name, const char *corrected_text,
                       const char *ledger_name, const char *ledger_text,
                       ocr_validator_error *error);
int ocr_validate_files(const char *raw_path, const char *corrected_path,
                       const char *ledger_path, ocr_validator_error *error);

#endif
