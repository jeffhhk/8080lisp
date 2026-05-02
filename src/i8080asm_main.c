#include "i8080_asm.h"

#include <stdio.h>

int main(int argc, char **argv) {
  i8080_asm_error error;
  i8080_image image;

  if (argc != 3) {
    fprintf(stderr, "usage: %s input.asm output.bin\n", argv[0]);
    return 1;
  }

  if (!i8080_assemble_file(argv[1], &image, &error)) {
    if (error.line_number != 0) {
      fprintf(stderr, "%s:%zu: %s\n", argv[1], error.line_number, error.message);
    } else {
      fprintf(stderr, "%s\n", error.message);
    }
    return 1;
  }

  if (!i8080_write_binary(argv[2], &image, &error)) {
    fprintf(stderr, "%s\n", error.message);
    return 1;
  }

  return 0;
}
