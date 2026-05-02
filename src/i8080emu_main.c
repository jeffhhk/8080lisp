#include "i8080_asm.h"
#include "i8080_emu.h"

#include <stdio.h>
#include <string.h>

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

static void write_coverage_report(FILE *stream, const i8080_coverage *coverage) {
  size_t address;
  size_t covered = 0;
  size_t total = 0;

  fputs("instruction coverage:\n", stream);
  for (address = 0; address < I8080_IMAGE_SIZE; ++address) {
    size_t hits = i8080_coverage_count(coverage, (uint16_t)address);
    if (hits == 0) {
      continue;
    }
    fprintf(stream, "0x%04zx %zu\n", address, hits);
    covered += 1;
    total += hits;
  }
  fprintf(stream, "covered_addresses %zu\n", covered);
  fprintf(stream, "total_instruction_fetches %zu\n", total);
}

int main(int argc, char **argv) {
  i8080_asm_error error;
  i8080_image image;
  i8080_cpu cpu;
  i8080_coverage coverage;
  i8080_hooks hooks = {
      .inch = host_inch,
      .outc = host_outc,
      .abend = host_abend,
      .ctx = NULL,
  };
  const char *program_path = NULL;
  int emit_coverage = 0;

  if (argc == 2) {
    program_path = argv[1];
  } else if (argc == 3 && strcmp(argv[1], "--coverage") == 0) {
    emit_coverage = 1;
    program_path = argv[2];
  } else {
    fprintf(stderr, "usage: %s [--coverage] program.asm\n", argv[0]);
    return 1;
  }

  if (!i8080_assemble_file(program_path, &image, &error)) {
    if (error.line_number != 0) {
      fprintf(stderr, "%s:%zu: %s\n", program_path, error.line_number, error.message);
    } else {
      fprintf(stderr, "%s\n", error.message);
    }
    return 1;
  }

  i8080_init(&cpu, &hooks);
  if (emit_coverage) {
    i8080_coverage_reset(&coverage);
    i8080_set_coverage(&cpu, &coverage);
  }
  i8080_load_image(&cpu, &image);
  i8080_run(&cpu, 10000000);
  if (emit_coverage) {
    write_coverage_report(stderr, &coverage);
  }
  if (cpu.error) {
    fprintf(stderr, "\nunsupported opcode 0x%02x at 0x%04x\n", cpu.error_opcode,
            (uint16_t)(cpu.pc - 1));
    return 1;
  }
  return cpu.abended ? 1 : 0;
}
