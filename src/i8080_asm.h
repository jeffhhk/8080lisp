#ifndef I8080_ASM_H
#define I8080_ASM_H

#include <stddef.h>
#include <stdint.h>

enum { I8080_IMAGE_SIZE = 65536 };

typedef struct {
  uint8_t bytes[I8080_IMAGE_SIZE];
  uint8_t used[I8080_IMAGE_SIZE];
  uint16_t origin;
  uint16_t limit;
} i8080_image;

typedef struct {
  size_t line_number;
  char message[160];
} i8080_asm_error;

void i8080_image_init(i8080_image *image);
size_t i8080_image_size(const i8080_image *image);

int i8080_assemble_file(const char *path, i8080_image *image,
                        i8080_asm_error *error);
int i8080_assemble_text(const char *name, const char *text, i8080_image *image,
                        i8080_asm_error *error);
int i8080_write_binary(const char *path, const i8080_image *image,
                       i8080_asm_error *error);

#endif
