#ifndef I8080_LISTING_H
#define I8080_LISTING_H

#include <stddef.h>
#include <stdint.h>

enum {
  I8080_LISTING_MAX_BYTES = 8,
};

typedef struct {
  int has_address;
  uint16_t address;
  int has_statement_line_number;
  size_t statement_line_number;
  uint8_t bytes[I8080_LISTING_MAX_BYTES];
  size_t byte_count;
  char source[256];
} i8080_listing_record;

typedef enum {
  I8080_LISTING_SOURCE_EMPTY = 0,
  I8080_LISTING_SOURCE_STAR = 1,
  I8080_LISTING_SOURCE_STATEMENT = 2,
} i8080_listing_source_kind;

typedef struct {
  i8080_listing_source_kind kind;
  char label[64];
  char mnemonic[64];
  char operand[128];
  char comment[128];
} i8080_listing_source_fields;

int i8080_parse_listing_record(const char *line, i8080_listing_record *record);
int i8080_listing_looks_like_listing(const char *text);
int i8080_parse_listing_source(const char *source,
                               i8080_listing_source_fields *fields);
void i8080_format_listing_bytes(char *dst, size_t dst_size,
                                const uint8_t *bytes, size_t byte_count);

#endif
