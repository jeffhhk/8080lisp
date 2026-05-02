#include "i8080_coverage_source_display.h"

#include <stdlib.h>
#include <string.h>

static void set_error(i8080_asm_error *error, const char *message) {
  if (error == NULL) {
    return;
  }
  error->line_number = 0;
  snprintf(error->message, sizeof(error->message), "%s", message);
}

static int read_file(const char *path, char **text, i8080_asm_error *error) {
  FILE *stream = fopen(path, "rb");
  long size;
  size_t read_count;

  if (stream == NULL) {
    set_error(error, "could not open input file");
    return 0;
  }
  if (fseek(stream, 0, SEEK_END) != 0) {
    fclose(stream);
    set_error(error, "could not seek input file");
    return 0;
  }
  size = ftell(stream);
  if (size < 0) {
    fclose(stream);
    set_error(error, "could not size input file");
    return 0;
  }
  if (fseek(stream, 0, SEEK_SET) != 0) {
    fclose(stream);
    set_error(error, "could not rewind input file");
    return 0;
  }

  *text = malloc((size_t)size + 1);
  if (*text == NULL) {
    fclose(stream);
    set_error(error, "out of memory");
    return 0;
  }

  read_count = fread(*text, 1, (size_t)size, stream);
  fclose(stream);
  (*text)[read_count] = '\0';
  return 1;
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

static size_t count_lines(const char *text) {
  size_t line_count = 0;

  if (text[0] == '\0') {
    return 0;
  }

  line_count = 1;
  while (*text != '\0') {
    if (*text == '\n') {
      line_count += 1;
    }
    text += 1;
  }
  return line_count;
}

static void trim_cr(char *line) {
  size_t len = strlen(line);

  if (len != 0 && line[len - 1] == '\r') {
    line[len - 1] = '\0';
  }
}

int i8080_write_coverage_source_display(FILE *stream, const char *path,
                                        const i8080_image *image,
                                        const i8080_coverage *coverage,
                                        const i8080_coverage *line_filter,
                                        i8080_asm_error *error) {
  char *text = NULL;
  char *cursor;
  char *line;
  size_t *line_hits = NULL;
  unsigned char *filtered_lines = NULL;
  size_t line_count;
  size_t line_number = 0;
  size_t previous_covered_line = 0;
  size_t address;

  if (!read_file(path, &text, error)) {
    return 0;
  }

  line_count = count_lines(text);
  line_hits = calloc(line_count + 1, sizeof(*line_hits));
  if (line_hits == NULL) {
    free(text);
    set_error(error, "out of memory");
    return 0;
  }
  filtered_lines = calloc(line_count + 1, sizeof(*filtered_lines));
  if (filtered_lines == NULL) {
    free(line_hits);
    free(text);
    set_error(error, "out of memory");
    return 0;
  }

  for (address = 0; address < I8080_IMAGE_SIZE; ++address) {
    size_t hits = coverage->ip_hits[address];
    size_t source_line = image->source_lines[address];

    if (hits == 0 || source_line == 0 || source_line > line_count) {
      continue;
    }
    line_hits[source_line] += hits;
  }
  if (line_filter != NULL) {
    for (address = 0; address < I8080_IMAGE_SIZE; ++address) {
      size_t source_line = image->source_lines[address];

      if (line_filter->ip_hits[address] == 0 || source_line == 0 ||
          source_line > line_count) {
        continue;
      }
      filtered_lines[source_line] = 1;
    }
  }

  fputs("source coverage:\n", stream);
  cursor = text;
  while ((line = next_line(&cursor)) != NULL) {
    line_number += 1;
    trim_cr(line);
    if (line_hits[line_number] == 0 || filtered_lines[line_number] != 0) {
      continue;
    }
    if (previous_covered_line != 0 && line_number > previous_covered_line + 1) {
      fputs("...\n", stream);
    }
    fprintf(stream, "%7zu | %s\n", line_hits[line_number], line);
    previous_covered_line = line_number;
  }

  free(filtered_lines);
  free(line_hits);
  free(text);
  return 1;
}
