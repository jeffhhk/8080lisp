#include "i8080_asm.h"
#include "i8080_emu.h"

#include <stdio.h>

static int host_inch(void *ctx) {
  (void)ctx;
  return getchar();
}

static void host_outc(void *ctx, uint8_t ch) {
  (void)ctx;
  putchar(ch);
  fflush(stdout);
}

static void host_abend(void *ctx, uint8_t code) {
  (void)ctx;
  fprintf(stderr, "\nABEND 0x%02x\n", code);
}

int main(int argc, char **argv) {
  i8080_asm_error error;
  i8080_image image;
  i8080_cpu cpu;
  i8080_hooks hooks = {
      .inch = host_inch,
      .outc = host_outc,
      .abend = host_abend,
      .ctx = NULL,
  };

  if (argc != 2) {
    fprintf(stderr, "usage: %s program.asm\n", argv[0]);
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

  i8080_init(&cpu, &hooks);
  i8080_load_image(&cpu, &image);
  i8080_run(&cpu, 10000000);
  if (cpu.error) {
    fprintf(stderr, "\nunsupported opcode 0x%02x at 0x%04x\n", cpu.error_opcode,
            (uint16_t)(cpu.pc - 1));
    return 1;
  }
  return cpu.abended ? 1 : 0;
}
