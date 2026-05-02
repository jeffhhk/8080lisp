#include "lisp_host_io.h"

#include <stdio.h>

enum { OUTPUT_CAPACITY = 4096 };

static const unsigned char *input_text;
static size_t input_pos;
static char output_text[OUTPUT_CAPACITY];
static size_t output_len;
static int capture_output;

void lisp_host_reset(void) {
  input_text = NULL;
  input_pos = 0;
  output_len = 0;
  output_text[0] = '\0';
  capture_output = 0;
}

void lisp_host_set_input(const char *text) {
  input_text = (const unsigned char *)text;
  input_pos = 0;
}

void lisp_host_clear_output(void) {
  output_len = 0;
  output_text[0] = '\0';
  capture_output = 1;
}

const char *lisp_host_output(void) {
  output_text[output_len] = '\0';
  return output_text;
}

void lisp_host_outc(int ch) {
  if (!capture_output) {
    putchar(ch);
    fflush(stdout);
    return;
  }

  if (output_len + 1 < OUTPUT_CAPACITY) {
    output_text[output_len++] = (char)ch;
    output_text[output_len] = '\0';
  }
}

int lisp_host_inch(void) {
  if (input_text != NULL) {
    const unsigned char next = input_text[input_pos];
    if (next == '\0') {
      return EOF;
    }
    input_pos += 1;
    return next;
  }

  return getchar();
}
