#include "i8080_listing.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_hex_word(const char *text, size_t len) {
  size_t i;
  if (len == 0) {
    return 0;
  }
  for (i = 0; i < len; ++i) {
    if (!isxdigit((unsigned char)text[i])) {
      return 0;
    }
  }
  return 1;
}

static void trim(char *text) {
  size_t len;
  size_t start = 0;

  while (isspace((unsigned char)text[start])) {
    start += 1;
  }
  if (start != 0) {
    memmove(text, text + start, strlen(text + start) + 1);
  }

  len = strlen(text);
  while (len > 0 && isspace((unsigned char)text[len - 1])) {
    text[len - 1] = '\0';
    len -= 1;
  }
}

static void uppercase_copy(char *dst, size_t dst_size, const char *src) {
  size_t i;
  for (i = 0; src[i] != '\0' && i + 1 < dst_size; ++i) {
    dst[i] = (char)toupper((unsigned char)src[i]);
  }
  dst[i] = '\0';
}

static int is_known_mnemonic(const char *token) {
  static const char *const mnemonics[] = {
      "ACI",  "ADC",  "ADD",  "ADI",  "ANA",  "ANI",  "CALL", "CC",
      "CMA",  "CMC",  "CM",   "CMP",  "CNC",  "CNZ",  "CP",   "CPI",
      "CZ",   "DAD",  "DB",   "DCR",  "DCX",  "DS",   "DW",
      "EQU",  "HLT",  "INR",  "INX",  "JC",   "JM",   "JMP",  "JNC",
      "JNZ",  "JP",   "JZ",   "LDA",  "LDAX", "LHLD", "LXI",  "MOV",
      "MVI",  "NOP",  "ORA",  "ORI",  "ORG",  "POP",  "PUSH",
      "RAL",  "RAR",  "RC",   "RET",  "RLC",  "RM",   "RNC",  "RNZ",
      "RP",   "RRC",  "RZ",   "SBB",  "SBI",  "SHLD", "SPHL", "STA",
      "STAX", "STC",  "SUB",  "SUI",  "XCHG", "XRA",  "XRI",  "XTHL",
  };
  size_t i;

  for (i = 0; i < sizeof(mnemonics) / sizeof(mnemonics[0]); ++i) {
    if (strcmp(token, mnemonics[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static int operand_arity(const char *mnemonic) {
  if (strcmp(mnemonic, "DB") == 0 || strcmp(mnemonic, "DW") == 0) {
    return -1;
  }
  if (strcmp(mnemonic, "ORG") == 0 || strcmp(mnemonic, "EQU") == 0 ||
      strcmp(mnemonic, "DS") == 0 || strcmp(mnemonic, "JMP") == 0 ||
      strcmp(mnemonic, "JNZ") == 0 || strcmp(mnemonic, "JZ") == 0 ||
      strcmp(mnemonic, "JNC") == 0 || strcmp(mnemonic, "JC") == 0 ||
      strcmp(mnemonic, "JP") == 0 || strcmp(mnemonic, "JM") == 0 ||
      strcmp(mnemonic, "CALL") == 0 || strcmp(mnemonic, "CNZ") == 0 ||
      strcmp(mnemonic, "CZ") == 0 || strcmp(mnemonic, "CNC") == 0 ||
      strcmp(mnemonic, "CC") == 0 || strcmp(mnemonic, "CP") == 0 ||
      strcmp(mnemonic, "CM") == 0 || strcmp(mnemonic, "LDA") == 0 ||
      strcmp(mnemonic, "STA") == 0 || strcmp(mnemonic, "LHLD") == 0 ||
      strcmp(mnemonic, "SHLD") == 0 || strcmp(mnemonic, "INX") == 0 ||
      strcmp(mnemonic, "DCX") == 0 || strcmp(mnemonic, "DAD") == 0 ||
      strcmp(mnemonic, "PUSH") == 0 || strcmp(mnemonic, "POP") == 0 ||
      strcmp(mnemonic, "LDAX") == 0 || strcmp(mnemonic, "STAX") == 0 ||
      strcmp(mnemonic, "INR") == 0 || strcmp(mnemonic, "DCR") == 0 ||
      strcmp(mnemonic, "ADD") == 0 || strcmp(mnemonic, "ADC") == 0 ||
      strcmp(mnemonic, "SUB") == 0 || strcmp(mnemonic, "SBB") == 0 ||
      strcmp(mnemonic, "ANA") == 0 || strcmp(mnemonic, "XRA") == 0 ||
      strcmp(mnemonic, "ORA") == 0 || strcmp(mnemonic, "CMP") == 0 ||
      strcmp(mnemonic, "ADI") == 0 || strcmp(mnemonic, "ACI") == 0 ||
      strcmp(mnemonic, "SUI") == 0 || strcmp(mnemonic, "SBI") == 0 ||
      strcmp(mnemonic, "ANI") == 0 || strcmp(mnemonic, "XRI") == 0 ||
      strcmp(mnemonic, "ORI") == 0 || strcmp(mnemonic, "CPI") == 0) {
    return 1;
  }
  if (strcmp(mnemonic, "MOV") == 0 || strcmp(mnemonic, "MVI") == 0 ||
      strcmp(mnemonic, "LXI") == 0) {
    return 2;
  }
  return 0;
}

static int parse_statement_fields(const char *source,
                                  i8080_listing_source_fields *fields) {
  char buffer[256];
  char upper_first[64];
  char first[64];
  char *cursor = NULL;
  char *space = NULL;
  size_t spaces_after_mnemonic = 0;
  char *rest = NULL;

  snprintf(buffer, sizeof(buffer), "%s", source);
  trim(buffer);

  memset(fields, 0, sizeof(*fields));
  if (buffer[0] == '\0') {
    fields->kind = I8080_LISTING_SOURCE_EMPTY;
    return 1;
  }
  if (strcmp(buffer, "*") == 0) {
    fields->kind = I8080_LISTING_SOURCE_STAR;
    return 1;
  }

  fields->kind = I8080_LISTING_SOURCE_STATEMENT;
  cursor = buffer;
  if (!isspace((unsigned char)cursor[0])) {
    space = strpbrk(cursor, " \t");
    if (space == NULL) {
      uppercase_copy(upper_first, sizeof(upper_first), cursor);
      if (is_known_mnemonic(upper_first)) {
        snprintf(fields->mnemonic, sizeof(fields->mnemonic), "%s",
                 upper_first);
      } else {
        uppercase_copy(fields->label, sizeof(fields->label), cursor);
      }
      return 1;
    }

    memcpy(first, cursor, (size_t)(space - cursor));
    first[space - cursor] = '\0';
    uppercase_copy(upper_first, sizeof(upper_first), first);
    if (!is_known_mnemonic(upper_first)) {
      uppercase_copy(fields->label, sizeof(fields->label), first);
      cursor = space;
      while (isspace((unsigned char)*cursor)) {
        cursor += 1;
      }
    }
  } else {
    while (isspace((unsigned char)*cursor)) {
      cursor += 1;
    }
  }

  if (*cursor == '\0') {
    return 1;
  }

  space = strpbrk(cursor, " \t");
  if (space == NULL) {
    uppercase_copy(fields->mnemonic, sizeof(fields->mnemonic), cursor);
    return 1;
  }

  memcpy(first, cursor, (size_t)(space - cursor));
  first[space - cursor] = '\0';
  uppercase_copy(fields->mnemonic, sizeof(fields->mnemonic), first);

  rest = space;
  while (isspace((unsigned char)rest[spaces_after_mnemonic])) {
    spaces_after_mnemonic += 1;
  }
  rest += spaces_after_mnemonic;

  if (*rest == '\0') {
    return 1;
  }

  if (operand_arity(fields->mnemonic) == 0 &&
      is_known_mnemonic(fields->mnemonic)) {
    trim(rest);
    snprintf(fields->comment, sizeof(fields->comment), "%s", rest);
    return 1;
  }

  {
    size_t i;
    size_t split = strlen(rest);
    for (i = 0; rest[i] != '\0'; ++i) {
      if (rest[i] == ' ' && rest[i + 1] == ' ') {
        split = i;
        break;
      }
    }

    if (split == strlen(rest)) {
      trim(rest);
      snprintf(fields->operand, sizeof(fields->operand), "%s", rest);
      return 1;
    }

    memcpy(fields->operand, rest, split);
    fields->operand[split] = '\0';
    trim(fields->operand);
    snprintf(fields->comment, sizeof(fields->comment), "%s", rest + split);
    trim(fields->comment);
  }

  return 1;
}

int i8080_parse_listing_record(const char *line, i8080_listing_record *record) {
  const char *cursor = line;
  char token[64];

  memset(record, 0, sizeof(*record));
  while (isspace((unsigned char)*cursor)) {
    cursor += 1;
  }
  if (!is_hex_word(cursor, 4)) {
    return 0;
  }
  memcpy(token, cursor, 4);
  token[4] = '\0';
  record->address = (uint16_t)strtoul(token, NULL, 16);
  record->has_address = 1;
  cursor += 4;

  for (;;) {
    size_t tok_len = 0;

    while (*cursor == ' ') {
      cursor += 1;
    }
    while (cursor[tok_len] != '\0' && !isspace((unsigned char)cursor[tok_len])) {
      tok_len += 1;
    }
    if (tok_len == 4 && isdigit((unsigned char)cursor[0]) &&
        isdigit((unsigned char)cursor[1]) && isdigit((unsigned char)cursor[2]) &&
        isdigit((unsigned char)cursor[3])) {
      memcpy(token, cursor, 4);
      token[4] = '\0';
      record->statement_line_number = (size_t)strtoul(token, NULL, 10);
      record->has_statement_line_number = 1;
      cursor += 4;
      break;
    }
    if (tok_len != 2 || !is_hex_word(cursor, 2)) {
      break;
    }
    if (record->byte_count >= I8080_LISTING_MAX_BYTES) {
      return 0;
    }
    memcpy(token, cursor, 2);
    token[2] = '\0';
    record->bytes[record->byte_count] = (uint8_t)strtoul(token, NULL, 16);
    record->byte_count += 1;
    cursor += 2;
  }

  while (isspace((unsigned char)*cursor)) {
    cursor += 1;
  }
  snprintf(record->source, sizeof(record->source), "%s", cursor);
  trim(record->source);
  return 1;
}

int i8080_listing_looks_like_listing(const char *text) {
  int checked = 0;

  while (*text != '\0' && checked < 32) {
    const char *line_end = strchr(text, '\n');
    char line[256];
    size_t len;
    i8080_listing_record record;

    if (line_end == NULL) {
      len = strlen(text);
    } else {
      len = (size_t)(line_end - text);
    }
    if (len >= sizeof(line)) {
      len = sizeof(line) - 1;
    }
    memcpy(line, text, len);
    line[len] = '\0';

    if (i8080_parse_listing_record(line, &record)) {
      return 1;
    }

    checked += 1;
    if (line_end == NULL) {
      break;
    }
    text = line_end + 1;
  }

  return 0;
}

int i8080_parse_listing_source(const char *source,
                               i8080_listing_source_fields *fields) {
  return parse_statement_fields(source, fields);
}

void i8080_format_listing_bytes(char *dst, size_t dst_size,
                                const uint8_t *bytes, size_t byte_count) {
  size_t i;
  size_t used = 0;

  if (dst_size == 0) {
    return;
  }
  dst[0] = '\0';
  for (i = 0; i < byte_count; ++i) {
    int written = snprintf(dst + used, dst_size - used, "%s%02X",
                           i == 0 ? "" : " ", bytes[i]);
    if (written < 0 || (size_t)written >= dst_size - used) {
      dst[dst_size - 1] = '\0';
      return;
    }
    used += (size_t)written;
  }
}
