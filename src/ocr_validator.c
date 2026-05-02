#include "ocr_validator.h"

#include "i8080_listing.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  OCR_FIELD_BYTES = 0,
  OCR_FIELD_LABEL = 1,
  OCR_FIELD_MNEMONIC = 2,
  OCR_FIELD_OPERAND = 3,
  OCR_FIELD_COMMENT = 4,
  OCR_FIELD_WHOLE_LINE = 5,
} ocr_field;

typedef struct {
  size_t physical_line_number;
  size_t line_number;
  int has_address;
  uint16_t address;
  ocr_field field;
  char original[256];
  char corrected[256];
  int matched;
} discrepancy;

typedef struct {
  char id[64];
  int has_id;
  size_t line_number;
  int has_line_number;
  int has_address;
  uint16_t address;
  ocr_field field;
  int has_field;
  char original[256];
  int has_original;
  char corrected[256];
  int has_corrected;
  char evidence[256];
  int has_evidence;
  char basis[256];
  int has_basis;
  char confidence[64];
  int has_confidence;
  size_t ledger_line_number;
} provenance_entry;

typedef struct {
  discrepancy *items;
  size_t count;
  size_t capacity;
} discrepancy_list;

typedef struct {
  provenance_entry *items;
  size_t count;
  size_t capacity;
} provenance_list;

static void set_error(ocr_validator_error *error, size_t line_number,
                      const char *message) {
  if (error == NULL) {
    return;
  }
  error->line_number = line_number;
  snprintf(error->message, sizeof(error->message), "%s", message);
}

static char *dup_text(const char *text) {
  size_t len = strlen(text);
  char *copy = malloc(len + 1);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, text, len + 1);
  return copy;
}

static char *next_line(char **cursor) {
  char *start;
  char *end;

  if (cursor == NULL || *cursor == NULL || **cursor == '\0') {
    return NULL;
  }

  start = *cursor;
  end = strchr(start, '\n');
  if (end == NULL) {
    *cursor = start + strlen(start);
    return start;
  }

  *end = '\0';
  *cursor = end + 1;
  return start;
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

static const char *field_name(ocr_field field) {
  switch (field) {
  case OCR_FIELD_BYTES:
    return "bytes";
  case OCR_FIELD_LABEL:
    return "label";
  case OCR_FIELD_MNEMONIC:
    return "mnemonic";
  case OCR_FIELD_OPERAND:
    return "operand";
  case OCR_FIELD_COMMENT:
    return "comment";
  case OCR_FIELD_WHOLE_LINE:
    return "whole_line";
  }
  return "unknown";
}

static int parse_field_name(const char *text, ocr_field *field) {
  char upper[32];

  uppercase_copy(upper, sizeof(upper), text);
  if (strcmp(upper, "BYTES") == 0) {
    *field = OCR_FIELD_BYTES;
    return 1;
  }
  if (strcmp(upper, "LABEL") == 0) {
    *field = OCR_FIELD_LABEL;
    return 1;
  }
  if (strcmp(upper, "MNEMONIC") == 0) {
    *field = OCR_FIELD_MNEMONIC;
    return 1;
  }
  if (strcmp(upper, "OPERAND") == 0) {
    *field = OCR_FIELD_OPERAND;
    return 1;
  }
  if (strcmp(upper, "COMMENT") == 0) {
    *field = OCR_FIELD_COMMENT;
    return 1;
  }
  if (strcmp(upper, "WHOLE_LINE") == 0) {
    *field = OCR_FIELD_WHOLE_LINE;
    return 1;
  }
  return 0;
}

static int parse_basis_name(const char *text) {
  static const char *const basis_names[] = {
      "instruction-encoding",
      "cross-reference",
      "control-flow",
      "duplicate-pattern",
      "runtime-behavior",
      "scan-review",
  };
  size_t i;

  for (i = 0; i < sizeof(basis_names) / sizeof(basis_names[0]); ++i) {
    if (strcmp(text, basis_names[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static int parse_number(const char *text, uint16_t *value) {
  char buffer[64];
  char *end = NULL;
  size_t len = strlen(text);
  int base = 10;
  size_t i;

  if (len == 0 || len >= sizeof(buffer)) {
    return 0;
  }

  snprintf(buffer, sizeof(buffer), "%s", text);
  trim(buffer);
  len = strlen(buffer);
  if (len == 0) {
    return 0;
  }
  if (len > 2 && buffer[0] == '0' && (buffer[1] == 'x' || buffer[1] == 'X')) {
    base = 16;
  } else if (buffer[len - 1] == 'h' || buffer[len - 1] == 'H') {
    base = 16;
    buffer[len - 1] = '\0';
  } else {
    for (i = 0; buffer[i] != '\0'; ++i) {
      if (isalpha((unsigned char)buffer[i])) {
        base = 16;
        break;
      }
    }
  }

  *value = (uint16_t)strtoul(buffer, &end, base);
  return end != NULL && *end == '\0';
}

static int append_discrepancy(discrepancy_list *list, size_t physical_line_number,
                              size_t line_number, int has_address,
                              uint16_t address, ocr_field field,
                              const char *original, const char *corrected,
                              ocr_validator_error *error) {
  discrepancy *item;

  if (list->count == list->capacity) {
    size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    discrepancy *grown =
        realloc(list->items, new_capacity * sizeof(*list->items));
    if (grown == NULL) {
      set_error(error, 0, "out of memory");
      return 0;
    }
    list->items = grown;
    list->capacity = new_capacity;
  }

  item = &list->items[list->count++];
  memset(item, 0, sizeof(*item));
  item->physical_line_number = physical_line_number;
  item->line_number = line_number;
  item->has_address = has_address;
  item->address = address;
  item->field = field;
  snprintf(item->original, sizeof(item->original), "%s", original);
  snprintf(item->corrected, sizeof(item->corrected), "%s", corrected);
  return 1;
}

static int append_provenance(provenance_list *list, const provenance_entry *entry,
                             ocr_validator_error *error) {
  provenance_entry *item;
  size_t i;

  for (i = 0; i < list->count; ++i) {
    if (strcmp(list->items[i].id, entry->id) == 0) {
      set_error(error, entry->ledger_line_number, "duplicate provenance id");
      return 0;
    }
  }

  if (list->count == list->capacity) {
    size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    provenance_entry *grown =
        realloc(list->items, new_capacity * sizeof(*list->items));
    if (grown == NULL) {
      set_error(error, 0, "out of memory");
      return 0;
    }
    list->items = grown;
    list->capacity = new_capacity;
  }

  item = &list->items[list->count++];
  *item = *entry;
  return 1;
}

static int parse_yaml_value(char *value) {
  size_t len;

  trim(value);
  len = strlen(value);
  if (len >= 2 &&
      ((value[0] == '"' && value[len - 1] == '"') ||
       (value[0] == '\'' && value[len - 1] == '\''))) {
    memmove(value, value + 1, len - 2);
    value[len - 2] = '\0';
  }
  return 1;
}

static int parse_key_value(char *line, char *key, size_t key_size, char *value,
                           size_t value_size) {
  char *colon = strchr(line, ':');
  size_t key_len;

  if (colon == NULL) {
    return 0;
  }
  *colon = '\0';
  trim(line);
  key_len = strlen(line);
  if (key_len >= key_size) {
    key_len = key_size - 1;
  }
  memcpy(key, line, key_len);
  key[key_len] = '\0';
  snprintf(value, value_size, "%s", colon + 1);
  parse_yaml_value(value);
  return 1;
}

static int finalize_provenance_entry(provenance_list *entries,
                                     const provenance_entry *entry, int active,
                                     ocr_validator_error *error) {
  if (!active) {
    return 1;
  }
  if (!entry->has_id || !entry->has_line_number || !entry->has_address ||
      !entry->has_field || !entry->has_original || !entry->has_corrected ||
      !entry->has_evidence || !entry->has_basis || !entry->has_confidence) {
    set_error(error, entry->ledger_line_number,
              "provenance entry is missing required fields");
    return 0;
  }
  return append_provenance(entries, entry, error);
}

static int parse_provenance_entries(const char *ledger_text,
                                    provenance_list *entries,
                                    ocr_validator_error *error) {
  char *content = dup_text(ledger_text);
  char *cursor = NULL;
  char *line = NULL;
  size_t ledger_line_number = 0;
  provenance_entry current;
  int active = 0;

  if (content == NULL) {
    set_error(error, 0, "out of memory");
    return 0;
  }

  memset(&current, 0, sizeof(current));
  cursor = content;
  while ((line = next_line(&cursor)) != NULL) {
    char buffer[512];
    char key[64];
    char value[256];
    char *payload = NULL;
    uint16_t parsed_number = 0;

    ledger_line_number += 1;
    snprintf(buffer, sizeof(buffer), "%s", line);
    trim(buffer);
    if (buffer[0] == '\0' || buffer[0] == '#') {
      continue;
    }

    payload = buffer;
    if (payload[0] == '-') {
      if (!finalize_provenance_entry(entries, &current, active, error)) {
        free(content);
        return 0;
      }
      memset(&current, 0, sizeof(current));
      active = 1;
      payload += 1;
      while (isspace((unsigned char)*payload)) {
        payload += 1;
      }
      current.ledger_line_number = ledger_line_number;
      if (*payload == '\0') {
        continue;
      }
    } else if (!active) {
      set_error(error, ledger_line_number,
                "provenance fields must belong to a list entry");
      free(content);
      return 0;
    }

    if (!parse_key_value(payload, key, sizeof(key), value, sizeof(value))) {
      set_error(error, ledger_line_number, "invalid provenance entry syntax");
      free(content);
      return 0;
    }

    if (strcmp(key, "id") == 0) {
      snprintf(current.id, sizeof(current.id), "%s", value);
      current.has_id = 1;
    } else if (strcmp(key, "line_number") == 0) {
      if (!parse_number(value, &parsed_number)) {
        set_error(error, ledger_line_number, "invalid provenance line_number");
        free(content);
        return 0;
      }
      current.line_number = parsed_number;
      current.has_line_number = 1;
    } else if (strcmp(key, "address") == 0) {
      if (!parse_number(value, &parsed_number)) {
        set_error(error, ledger_line_number, "invalid provenance address");
        free(content);
        return 0;
      }
      current.address = parsed_number;
      current.has_address = 1;
    } else if (strcmp(key, "field") == 0) {
      if (!parse_field_name(value, &current.field)) {
        set_error(error, ledger_line_number, "invalid provenance field");
        free(content);
        return 0;
      }
      current.has_field = 1;
    } else if (strcmp(key, "original") == 0) {
      snprintf(current.original, sizeof(current.original), "%s", value);
      current.has_original = 1;
    } else if (strcmp(key, "corrected") == 0) {
      snprintf(current.corrected, sizeof(current.corrected), "%s", value);
      current.has_corrected = 1;
    } else if (strcmp(key, "evidence") == 0) {
      snprintf(current.evidence, sizeof(current.evidence), "%s", value);
      current.has_evidence = 1;
    } else if (strcmp(key, "basis") == 0) {
      if (!parse_basis_name(value)) {
        set_error(error, ledger_line_number, "invalid provenance basis");
        free(content);
        return 0;
      }
      snprintf(current.basis, sizeof(current.basis), "%s", value);
      current.has_basis = 1;
    } else if (strcmp(key, "confidence") == 0) {
      snprintf(current.confidence, sizeof(current.confidence), "%s", value);
      current.has_confidence = 1;
    } else {
      set_error(error, ledger_line_number, "unsupported provenance field");
      free(content);
      return 0;
    }
  }

  if (!finalize_provenance_entry(entries, &current, active, error)) {
    free(content);
    return 0;
  }

  free(content);
  return 1;
}

static void canonicalize_text(char *dst, size_t dst_size, const char *src,
                              int uppercase) {
  char buffer[256];

  snprintf(buffer, sizeof(buffer), "%s", src);
  trim(buffer);
  if (uppercase) {
    uppercase_copy(dst, dst_size, buffer);
  } else {
    snprintf(dst, dst_size, "%s", buffer);
  }
}

static int add_field_discrepancy(discrepancy_list *list, size_t physical_line_number,
                                 size_t line_number, int has_address,
                                 uint16_t address, ocr_field field,
                                 const char *original, const char *corrected,
                                 ocr_validator_error *error) {
  char left[256];
  char right[256];
  int uppercase = field == OCR_FIELD_LABEL || field == OCR_FIELD_MNEMONIC;

  canonicalize_text(left, sizeof(left), original, uppercase);
  canonicalize_text(right, sizeof(right), corrected, uppercase);
  if (strcmp(left, right) == 0) {
    return 1;
  }
  return append_discrepancy(list, physical_line_number, line_number, has_address,
                            address, field, left, right, error);
}

static void copy_trimmed_line(char *dst, size_t dst_size, const char *src) {
  snprintf(dst, dst_size, "%s", src == NULL ? "" : src);
  trim(dst);
}

static int compare_record_sources(discrepancy_list *discrepancies,
                                  size_t physical_line_number,
                                  const i8080_listing_record *raw_record,
                                  const i8080_listing_record *corrected_record,
                                  ocr_validator_error *error) {
  i8080_listing_source_fields raw_fields;
  i8080_listing_source_fields corrected_fields;

  if (!i8080_parse_listing_source(raw_record->source, &raw_fields) ||
      !i8080_parse_listing_source(corrected_record->source, &corrected_fields)) {
    return append_discrepancy(discrepancies, physical_line_number,
                              raw_record->statement_line_number,
                              raw_record->has_address, raw_record->address,
                              OCR_FIELD_WHOLE_LINE, raw_record->source,
                              corrected_record->source, error);
  }

  if (raw_fields.kind != corrected_fields.kind) {
    return append_discrepancy(discrepancies, physical_line_number,
                              raw_record->statement_line_number,
                              raw_record->has_address, raw_record->address,
                              OCR_FIELD_WHOLE_LINE, raw_record->source,
                              corrected_record->source, error);
  }

  if (raw_fields.kind != I8080_LISTING_SOURCE_STATEMENT) {
    if (strcmp(raw_record->source, corrected_record->source) == 0) {
      return 1;
    }
    return append_discrepancy(discrepancies, physical_line_number,
                              raw_record->statement_line_number,
                              raw_record->has_address, raw_record->address,
                              OCR_FIELD_WHOLE_LINE, raw_record->source,
                              corrected_record->source, error);
  }

  if (!add_field_discrepancy(discrepancies, physical_line_number,
                             raw_record->statement_line_number,
                             raw_record->has_address, raw_record->address,
                             OCR_FIELD_LABEL, raw_fields.label,
                             corrected_fields.label, error) ||
      !add_field_discrepancy(discrepancies, physical_line_number,
                             raw_record->statement_line_number,
                             raw_record->has_address, raw_record->address,
                             OCR_FIELD_MNEMONIC, raw_fields.mnemonic,
                             corrected_fields.mnemonic, error) ||
      !add_field_discrepancy(discrepancies, physical_line_number,
                             raw_record->statement_line_number,
                             raw_record->has_address, raw_record->address,
                             OCR_FIELD_OPERAND, raw_fields.operand,
                             corrected_fields.operand, error) ||
      !add_field_discrepancy(discrepancies, physical_line_number,
                             raw_record->statement_line_number,
                             raw_record->has_address, raw_record->address,
                             OCR_FIELD_COMMENT, raw_fields.comment,
                             corrected_fields.comment, error)) {
    return 0;
  }

  return 1;
}

static int collect_discrepancies(const char *raw_text, const char *corrected_text,
                                 discrepancy_list *discrepancies,
                                 ocr_validator_error *error) {
  char *raw_copy = dup_text(raw_text);
  char *corrected_copy = dup_text(corrected_text);
  char *raw_cursor = NULL;
  char *corrected_cursor = NULL;
  size_t physical_line_number = 0;

  if (raw_copy == NULL || corrected_copy == NULL) {
    free(raw_copy);
    free(corrected_copy);
    set_error(error, 0, "out of memory");
    return 0;
  }

  raw_cursor = raw_copy;
  corrected_cursor = corrected_copy;
  for (;;) {
    char *raw_line = next_line(&raw_cursor);
    char *corrected_line = next_line(&corrected_cursor);
    i8080_listing_record raw_record;
    i8080_listing_record corrected_record;
    char raw_trimmed[256];
    char corrected_trimmed[256];
    char raw_bytes[64];
    char corrected_bytes[64];

    if (raw_line == NULL && corrected_line == NULL) {
      break;
    }
    physical_line_number += 1;
    copy_trimmed_line(raw_trimmed, sizeof(raw_trimmed), raw_line);
    copy_trimmed_line(corrected_trimmed, sizeof(corrected_trimmed),
                      corrected_line);

    int raw_ok =
        i8080_parse_listing_record(raw_line == NULL ? "" : raw_line, &raw_record);
    int corrected_ok = i8080_parse_listing_record(
        corrected_line == NULL ? "" : corrected_line, &corrected_record);

    if (!raw_ok || !corrected_ok) {
      size_t line_number = physical_line_number;
      int has_address = 0;
      uint16_t address = 0;

      if (raw_ok) {
        line_number = raw_record.has_statement_line_number
                          ? raw_record.statement_line_number
                          : physical_line_number;
        has_address = raw_record.has_address;
        address = raw_record.address;
      } else if (corrected_ok) {
        line_number = corrected_record.has_statement_line_number
                          ? corrected_record.statement_line_number
                          : physical_line_number;
        has_address = corrected_record.has_address;
        address = corrected_record.address;
      }

      if (strcmp(raw_trimmed, corrected_trimmed) != 0 &&
          !append_discrepancy(discrepancies, physical_line_number, line_number,
                              has_address, address,
                              OCR_FIELD_WHOLE_LINE, raw_trimmed,
                              corrected_trimmed, error)) {
        free(raw_copy);
        free(corrected_copy);
        return 0;
      }
      continue;
    }

    i8080_format_listing_bytes(raw_bytes, sizeof(raw_bytes), raw_record.bytes,
                               raw_record.byte_count);
    i8080_format_listing_bytes(corrected_bytes, sizeof(corrected_bytes),
                               corrected_record.bytes,
                               corrected_record.byte_count);
    if (!add_field_discrepancy(discrepancies, physical_line_number,
                               raw_record.has_statement_line_number
                                   ? raw_record.statement_line_number
                                   : physical_line_number,
                               raw_record.has_address, raw_record.address,
                               OCR_FIELD_BYTES, raw_bytes, corrected_bytes,
                               error) ||
        !compare_record_sources(discrepancies, physical_line_number, &raw_record,
                                &corrected_record, error)) {
      free(raw_copy);
      free(corrected_copy);
      return 0;
    }
  }

  free(raw_copy);
  free(corrected_copy);
  return 1;
}

static int match_provenance_entries(discrepancy_list *discrepancies,
                                    const provenance_list *entries,
                                    ocr_validator_error *error) {
  size_t i;

  for (i = 0; i < entries->count; ++i) {
    size_t j;
    discrepancy *match = NULL;

    for (j = 0; j < discrepancies->count; ++j) {
      discrepancy *candidate = &discrepancies->items[j];
      if (candidate->matched) {
        continue;
      }
      if (candidate->line_number != entries->items[i].line_number ||
          candidate->field != entries->items[i].field ||
          candidate->has_address != entries->items[i].has_address) {
        continue;
      }
      if (candidate->has_address &&
          candidate->address != entries->items[i].address) {
        continue;
      }
      if (strcmp(candidate->original, entries->items[i].original) != 0 ||
          strcmp(candidate->corrected, entries->items[i].corrected) != 0) {
        continue;
      }
      if (match != NULL) {
        set_error(error, entries->items[i].ledger_line_number,
                  "ambiguous provenance entry");
        return 0;
      }
      match = candidate;
    }

    if (match == NULL) {
      char message[160];
      snprintf(message, sizeof(message),
               "stale provenance entry %s does not match a discrepancy",
               entries->items[i].id);
      set_error(error, entries->items[i].ledger_line_number, message);
      return 0;
    }
    match->matched = 1;
  }

  for (i = 0; i < discrepancies->count; ++i) {
    char message[160];

    if (discrepancies->items[i].matched) {
      continue;
    }
    snprintf(message, sizeof(message),
             "unaccounted discrepancy at line %zu field %s",
             discrepancies->items[i].line_number,
             field_name(discrepancies->items[i].field));
    set_error(error, discrepancies->items[i].physical_line_number, message);
    return 0;
  }

  return 1;
}

static int read_file(const char *path, char **text, ocr_validator_error *error) {
  FILE *fp = fopen(path, "rb");
  long size;
  size_t read_count;

  if (fp == NULL) {
    set_error(error, 0, "could not open input file");
    return 0;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    set_error(error, 0, "could not seek input file");
    return 0;
  }
  size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    set_error(error, 0, "could not size input file");
    return 0;
  }
  if (fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    set_error(error, 0, "could not rewind input file");
    return 0;
  }

  *text = malloc((size_t)size + 1);
  if (*text == NULL) {
    fclose(fp);
    set_error(error, 0, "out of memory");
    return 0;
  }
  read_count = fread(*text, 1, (size_t)size, fp);
  fclose(fp);
  (*text)[read_count] = '\0';
  return 1;
}

int ocr_validate_texts(const char *raw_name, const char *raw_text,
                       const char *corrected_name, const char *corrected_text,
                       const char *ledger_name, const char *ledger_text,
                       ocr_validator_error *error) {
  discrepancy_list discrepancies = {0};
  provenance_list entries = {0};
  int ok = 0;

  (void)raw_name;
  (void)corrected_name;
  (void)ledger_name;

  if (!collect_discrepancies(raw_text, corrected_text, &discrepancies, error) ||
      !parse_provenance_entries(ledger_text, &entries, error) ||
      !match_provenance_entries(&discrepancies, &entries, error)) {
    goto cleanup;
  }

  ok = 1;

cleanup:
  free(discrepancies.items);
  free(entries.items);
  return ok;
}

int ocr_validate_files(const char *raw_path, const char *corrected_path,
                       const char *ledger_path, ocr_validator_error *error) {
  char *raw_text = NULL;
  char *corrected_text = NULL;
  char *ledger_text = NULL;
  int ok = 0;

  if (!read_file(raw_path, &raw_text, error) ||
      !read_file(corrected_path, &corrected_text, error) ||
      !read_file(ledger_path, &ledger_text, error)) {
    goto cleanup;
  }

  ok = ocr_validate_texts(raw_path, raw_text, corrected_path, corrected_text,
                          ledger_path, ledger_text, error);

cleanup:
  free(raw_text);
  free(corrected_text);
  free(ledger_text);
  return ok;
}
