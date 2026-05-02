#include "i8080_asm.h"
#include "i8080_coverage_source_display.h"
#include "i8080_emu.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  i8080_cpu *cpu;
  int saw_eof;
} cli_host_io;

static int host_inch(void *ctx) {
  cli_host_io *host = ctx;
  int ch = getchar();

  if (ch == EOF) {
    host->saw_eof = 1;
    if (host->cpu != NULL) {
      host->cpu->halted = 1;
    }
    return 0;
  }
  return ch;
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

static void summarize_coverage(const i8080_coverage *coverage, size_t *covered,
                               size_t *total) {
  size_t address;

  *covered = 0;
  *total = 0;

  for (address = 0; address < I8080_IMAGE_SIZE; ++address) {
    size_t hits = i8080_coverage_count(coverage, (uint16_t)address);
    if (hits == 0) {
      continue;
    }
    *covered += 1;
    *total += hits;
  }
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
  }
  summarize_coverage(coverage, &covered, &total);
  fprintf(stream, "covered_addresses %zu\n", covered);
  fprintf(stream, "total_instruction_fetches %zu\n", total);
}

static int write_coverage_ndjson(const char *path,
                                 const i8080_coverage *coverage) {
  FILE *stream;
  size_t address;
  size_t covered = 0;
  size_t total = 0;

  stream = fopen(path, "w");
  if (stream == NULL) {
    fprintf(stderr, "%s: %s\n", path, strerror(errno));
    return 0;
  }

  for (address = 0; address < I8080_IMAGE_SIZE; ++address) {
    size_t hits = i8080_coverage_count(coverage, (uint16_t)address);
    if (hits == 0) {
      continue;
    }
    fprintf(stream,
            "{\"kind\":\"address\",\"address\":%zu,\"address_hex\":"
            "\"0x%04zx\",\"hits\":%zu}\n",
            address, address, hits);
  }

  summarize_coverage(coverage, &covered, &total);
  fprintf(stream,
          "{\"kind\":\"summary\",\"covered_addresses\":%zu,"
          "\"total_instruction_fetches\":%zu}\n",
          covered, total);

  if (fclose(stream) != 0) {
    fprintf(stderr, "%s: %s\n", path, strerror(errno));
    return 0;
  }
  return 1;
}

static int parse_size_t_field(const char *line, const char *name,
                              size_t *value) {
  char pattern[32];
  char *field = NULL;
  char *end = NULL;
  unsigned long long parsed;

  snprintf(pattern, sizeof(pattern), "\"%s\":", name);
  field = strstr(line, pattern);
  if (field == NULL) {
    return 0;
  }
  field += strlen(pattern);
  parsed = strtoull(field, &end, 10);
  if (end == field) {
    return 0;
  }
  *value = (size_t)parsed;
  return 1;
}

static int read_coverage_ndjson(const char *path, i8080_coverage *coverage) {
  FILE *stream;
  char line[256];

  stream = fopen(path, "r");
  if (stream == NULL) {
    fprintf(stderr, "%s: %s\n", path, strerror(errno));
    return 0;
  }

  i8080_coverage_reset(coverage);
  while (fgets(line, sizeof(line), stream) != NULL) {
    size_t address;
    size_t hits;

    if (strstr(line, "\"kind\":\"summary\"") != NULL) {
      continue;
    }
    if (strstr(line, "\"kind\":\"address\"") == NULL) {
      fprintf(stderr, "%s: invalid coverage record\n", path);
      fclose(stream);
      return 0;
    }
    if (!parse_size_t_field(line, "address", &address) ||
        !parse_size_t_field(line, "hits", &hits) ||
        address >= I8080_IMAGE_SIZE) {
      fprintf(stderr, "%s: invalid coverage record\n", path);
      fclose(stream);
      return 0;
    }
    coverage->ip_hits[address] = hits;
  }

  if (ferror(stream)) {
    fprintf(stderr, "%s: %s\n", path, strerror(errno));
    fclose(stream);
    return 0;
  }
  fclose(stream);
  return 1;
}

static void subtract_coverage(i8080_coverage *coverage,
                              const i8080_coverage *baseline) {
  size_t address;

  for (address = 0; address < I8080_IMAGE_SIZE; ++address) {
    size_t baseline_hits = baseline->ip_hits[address];
    size_t hits = coverage->ip_hits[address];

    if (hits <= baseline_hits) {
      coverage->ip_hits[address] = 0;
      continue;
    }
    coverage->ip_hits[address] = hits - baseline_hits;
  }
}

static int parse_args(int argc, char **argv, int *emit_coverage,
                      int *capture_coverage,
                      int *display_coverage_source_on_exit,
                      const char **coverage_baseline_path,
                      const char **coverage_baseline_boolean_path,
                      const char **coverage_out_path,
                      const char **program_path) {
  int i;

  *emit_coverage = 0;
  *capture_coverage = 0;
  *display_coverage_source_on_exit = 0;
  *coverage_baseline_path = NULL;
  *coverage_baseline_boolean_path = NULL;
  *coverage_out_path = NULL;
  *program_path = NULL;

  for (i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--coverage") == 0) {
      *emit_coverage = 1;
      *capture_coverage = 1;
      continue;
    }
    if (strcmp(argv[i], "--coverage-out") == 0) {
      if (i + 1 >= argc) {
        return 0;
      }
      *capture_coverage = 1;
      *coverage_out_path = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "--coverage-source-display-on-exit") == 0) {
      *capture_coverage = 1;
      *display_coverage_source_on_exit = 1;
      continue;
    }
    if (strcmp(argv[i], "--coverage-baseline") == 0) {
      if (i + 1 >= argc) {
        return 0;
      }
      *capture_coverage = 1;
      *coverage_baseline_path = argv[++i];
      continue;
    }
    if (strcmp(argv[i], "--coverage-baseline-boolean") == 0) {
      if (i + 1 >= argc) {
        return 0;
      }
      *capture_coverage = 1;
      *coverage_baseline_boolean_path = argv[++i];
      continue;
    }
    if (*program_path != NULL) {
      return 0;
    }
    *program_path = argv[i];
  }

  return *program_path != NULL;
}

int main(int argc, char **argv) {
  i8080_asm_error error;
  i8080_image image;
  i8080_cpu cpu;
  i8080_coverage coverage;
  cli_host_io host_io = {
      .cpu = &cpu,
      .saw_eof = 0,
  };
  i8080_hooks hooks = {
      .inch = host_inch,
      .outc = host_outc,
      .abend = host_abend,
      .ctx = &host_io,
  };
  const char *program_path = NULL;
  const char *coverage_baseline_path = NULL;
  const char *coverage_baseline_boolean_path = NULL;
  const char *coverage_out_path = NULL;
  i8080_coverage display_coverage;
  i8080_coverage baseline_coverage;
  const i8080_coverage *display_line_filter = NULL;
  int capture_coverage = 0;
  int display_coverage_source_on_exit = 0;
  int emit_coverage = 0;
  int run_result;

  if (!parse_args(argc, argv, &emit_coverage, &capture_coverage,
                  &display_coverage_source_on_exit,
                  &coverage_baseline_path,
                  &coverage_baseline_boolean_path,
                  &coverage_out_path,
                  &program_path)) {
    fprintf(stderr,
            "usage: %s [--coverage] [--coverage-out path] "
            "[--coverage-source-display-on-exit] "
            "[--coverage-baseline path] "
            "[--coverage-baseline-boolean path] program.asm\n",
            argv[0]);
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
  if (capture_coverage) {
    i8080_coverage_reset(&coverage);
    i8080_set_coverage(&cpu, &coverage);
  }
  i8080_load_image(&cpu, &image);
  run_result = i8080_run(&cpu, 10000000);
  if (emit_coverage) {
    write_coverage_report(stderr, &coverage);
  }
  if (display_coverage_source_on_exit) {
    display_coverage = coverage;
    if (coverage_baseline_path != NULL) {
      if (!read_coverage_ndjson(coverage_baseline_path, &baseline_coverage)) {
        return 1;
      }
      subtract_coverage(&display_coverage, &baseline_coverage);
    }
    if (coverage_baseline_boolean_path != NULL) {
      if (!read_coverage_ndjson(coverage_baseline_boolean_path,
                                &baseline_coverage)) {
        return 1;
      }
      display_line_filter = &baseline_coverage;
    }
    putchar('\n');
    if (!i8080_write_coverage_source_display(stdout, program_path, &image,
                                             &display_coverage,
                                             display_line_filter, &error)) {
      fprintf(stderr, "%s: %s\n", program_path, error.message);
      return 1;
    }
    fflush(stdout);
  }
  if (coverage_out_path != NULL &&
      !write_coverage_ndjson(coverage_out_path, &coverage)) {
    return 1;
  }
  if (run_result == I8080_STEP_LIMIT) {
    fprintf(stderr, "\nstep limit reached after %zu steps\n", cpu.steps);
    return 1;
  }
  if (cpu.error) {
    fprintf(stderr, "\nunsupported opcode 0x%02x at 0x%04x\n", cpu.error_opcode,
            (uint16_t)(cpu.pc - 1));
    return 1;
  }
  return cpu.abended ? 1 : 0;
}
