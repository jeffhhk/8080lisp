#include "i8080_asm.h"
#include "i8080_emu.h"
#include "lisp_host_io.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures;

static void expect_int(const char *label, int actual, int expected) {
  if (actual == expected) {
    return;
  }
  fprintf(stderr, "%s: expected %d but saw %d\n", label, expected, actual);
  failures += 1;
}

static void expect_text(const char *label, const char *actual,
                        const char *expected) {
  if (strcmp(actual, expected) == 0) {
    return;
  }
  fprintf(stderr, "%s: expected \"%s\" but saw \"%s\"\n", label, expected,
          actual);
  failures += 1;
}

static int host_inch(void *ctx) {
  (void)ctx;
  return lisp_host_inch();
}

static void host_outc(void *ctx, uint8_t ch) {
  (void)ctx;
  lisp_host_outc(ch);
}

static void host_abend(void *ctx, uint8_t code) {
  uint8_t *store = ctx;
  *store = code;
}

static void load_program(i8080_cpu *cpu, const i8080_image *image,
                         uint8_t *abend_code) {
  i8080_hooks hooks = {
      .inch = host_inch,
      .outc = host_outc,
      .abend = host_abend,
      .ctx = abend_code,
  };
  i8080_init(cpu, &hooks);
  i8080_load_image(cpu, image);
}

static void run_until_pc(i8080_cpu *cpu, uint16_t expected_pc, size_t max_steps) {
  size_t steps = 0;
  while (steps < max_steps && !cpu->halted && cpu->pc != expected_pc) {
    i8080_step(cpu);
    steps += 1;
  }
}

static void test_monitor_hooks_work_in_the_emulator(void) {
  static const char *program =
      "ORG 0000H\n"
      "MVI A,'H'\n"
      "CALL 0F009H\n"
      "CALL 0F021H\n"
      "CALL 0F006H\n"
      "CALL 0F009H\n"
      "JMP 0F000H\n";
  i8080_image image;
  i8080_asm_error error;
  i8080_cpu cpu;
  uint8_t abend_code = 0;

  expect_int("assemble hook test",
             i8080_assemble_text("hook", program, &image, &error), 1);

  lisp_host_reset();
  lisp_host_set_input("Z");
  lisp_host_clear_output();
  load_program(&cpu, &image, &abend_code);
  expect_int("emulator run hook test", i8080_run(&cpu, 64), 1);
  expect_text("hook output", lisp_host_output(), "H\nZ");
  expect_int("hook abend", cpu.abended, 1);
  expect_int("hook abend code", abend_code, 'Z');
}

static void test_instruction_pointer_coverage_counts_executed_addresses(void) {
  static const char *program =
      "ORG 0000H\n"
      "MVI B,03H\n"
      "LOOP: DCR B\n"
      "JNZ LOOP\n"
      "HLT\n";
  i8080_image image;
  i8080_asm_error error;
  i8080_cpu cpu;
  i8080_coverage coverage;
  uint8_t abend_code = 0;

  expect_int("assemble coverage test",
             i8080_assemble_text("coverage", program, &image, &error), 1);

  load_program(&cpu, &image, &abend_code);
  i8080_coverage_reset(&coverage);
  i8080_set_coverage(&cpu, &coverage);
  expect_int("coverage run", i8080_run(&cpu, 32), 1);
  expect_int("coverage pc 0000", (int)i8080_coverage_count(&coverage, 0x0000), 1);
  expect_int("coverage pc 0001", (int)i8080_coverage_count(&coverage, 0x0001), 0);
  expect_int("coverage pc 0002", (int)i8080_coverage_count(&coverage, 0x0002), 3);
  expect_int("coverage pc 0003", (int)i8080_coverage_count(&coverage, 0x0003), 3);
  expect_int("coverage pc 0006", (int)i8080_coverage_count(&coverage, 0x0006), 1);
}

static void test_original_image_boots_to_the_monitor_loop(void) {
  i8080_image image;
  i8080_asm_error error;
  i8080_cpu cpu;
  uint8_t abend_code = 0;

  expect_int("assemble orig",
             i8080_assemble_file("orig/lisp_8080_rawocr_2026-04-13.asm", &image,
                                 &error),
             1);

  lisp_host_reset();
  lisp_host_set_input("");
  lisp_host_clear_output();
  load_program(&cpu, &image, &abend_code);
  run_until_pc(&cpu, 0x0599, 10000);
  expect_int("boot reaches inloop", cpu.pc, 0x0599);
  expect_int("boot no abend", cpu.abended, 0);
  expect_int("boot no cpu error", cpu.error, 0);
  expect_text("boot leaves output empty", lisp_host_output(), "");
}

static void expect_contains(const char *label, const char *actual,
                            const char *expected) {
  if (strstr(actual, expected) != NULL) {
    return;
  }
  fprintf(stderr, "%s: expected to find \"%s\" in \"%s\"\n", label, expected,
          actual);
  failures += 1;
}

static void expect_not_contains(const char *label, const char *actual,
                                const char *unexpected) {
  if (strstr(actual, unexpected) == NULL) {
    return;
  }
  fprintf(stderr, "%s: did not expect to find \"%s\" in \"%s\"\n", label,
          unexpected, actual);
  failures += 1;
}

static int read_file_text(const char *path, char *buffer, size_t size) {
  FILE *stream = fopen(path, "r");
  size_t read_len;

  if (stream == NULL) {
    fprintf(stderr, "read %s failed\n", path);
    failures += 1;
    return 0;
  }
  read_len = fread(buffer, 1, size - 1, stream);
  if (ferror(stream)) {
    fprintf(stderr, "read %s failed\n", path);
    fclose(stream);
    failures += 1;
    return 0;
  }
  buffer[read_len] = '\0';
  fclose(stream);
  return 1;
}

static int write_file_text(const char *path, const char *text) {
  FILE *stream = fopen(path, "w");

  if (stream == NULL) {
    fprintf(stderr, "write %s failed\n", path);
    failures += 1;
    return 0;
  }
  fputs(text, stream);
  fclose(stream);
  return 1;
}

static void test_cli_writes_coverage_ndjson(void) {
  static const char *program =
      "ORG 0000H\n"
      "MVI B,03H\n"
      "LOOP: DCR B\n"
      "JNZ LOOP\n"
      "HLT\n";
  char asm_path[128];
  char coverage_path[128];
  char command[320];
  char coverage_text[1024];
  int status;

  snprintf(asm_path, sizeof(asm_path), "/tmp/i8080_coverage_%ld.asm",
           (long)getpid());
  snprintf(coverage_path, sizeof(coverage_path), "/tmp/i8080_coverage_%ld.ndjson",
           (long)getpid());

  if (!write_file_text(asm_path, program)) {
    return;
  }

  snprintf(command, sizeof(command), "./o/i8080emu --coverage-out %s %s",
           coverage_path, asm_path);
  status = system(command);
  if (status == -1) {
    fprintf(stderr, "coverage-out command failed to launch\n");
    failures += 1;
    return;
  }
  if (!WIFEXITED(status)) {
    fprintf(stderr, "coverage-out command did not exit cleanly\n");
    failures += 1;
    return;
  }
  expect_int("coverage-out command exited", WEXITSTATUS(status), 0);
  if (!read_file_text(coverage_path, coverage_text, sizeof(coverage_text))) {
    return;
  }

  expect_contains("coverage-out address 0000", coverage_text,
                  "{\"kind\":\"address\",\"address\":0,\"address_hex\":\"0x0000\",\"hits\":1}");
  expect_contains("coverage-out address 0002", coverage_text,
                  "{\"kind\":\"address\",\"address\":2,\"address_hex\":\"0x0002\",\"hits\":3}");
  expect_contains("coverage-out address 0003", coverage_text,
                  "{\"kind\":\"address\",\"address\":3,\"address_hex\":\"0x0003\",\"hits\":3}");
  expect_contains("coverage-out address 0006", coverage_text,
                  "{\"kind\":\"address\",\"address\":6,\"address_hex\":\"0x0006\",\"hits\":1}");
  expect_contains("coverage-out summary", coverage_text,
                  "{\"kind\":\"summary\",\"covered_addresses\":4,\"total_instruction_fetches\":8}");
}

static void test_cli_displays_compressed_covered_source_on_exit(void) {
  static const char *program =
      "ORG 0000H\n"
      "MVI A,'A'\n"
      "CALL 0F009H\n"
      "JMP DONE\n"
      "MVI A,'B'\n"
      "DONE: HLT\n";
  char asm_path[128];
  char output_path[128];
  char command[320];
  char output_text[2048];
  int status;

  snprintf(asm_path, sizeof(asm_path), "/tmp/i8080_covsrc_%ld.asm",
           (long)getpid());
  snprintf(output_path, sizeof(output_path), "/tmp/i8080_covsrc_%ld.txt",
           (long)getpid());

  if (!write_file_text(asm_path, program)) {
    return;
  }

  snprintf(command, sizeof(command),
           "./o/i8080emu --coverage-source-display-on-exit %s > %s",
           asm_path, output_path);
  status = system(command);
  if (status == -1) {
    fprintf(stderr, "coverage-source-display command failed to launch\n");
    failures += 1;
    return;
  }
  if (!WIFEXITED(status)) {
    fprintf(stderr, "coverage-source-display command did not exit cleanly\n");
    failures += 1;
    return;
  }
  expect_int("coverage-source-display command exited", WEXITSTATUS(status), 0);
  if (!read_file_text(output_path, output_text, sizeof(output_text))) {
    return;
  }

  expect_contains("coverage-source-display output prefix", output_text,
                  "A\nsource coverage:\n");
  expect_contains("coverage-source-display line 2", output_text,
                  "| MVI A,'A'\n");
  expect_contains("coverage-source-display line 3", output_text,
                  "| CALL 0F009H\n");
  expect_contains("coverage-source-display line 4", output_text,
                  "| JMP DONE\n");
  expect_contains("coverage-source-display ellipsis", output_text, "...\n");
  expect_contains("coverage-source-display line 6", output_text,
                  "| DONE: HLT\n");
  expect_not_contains("coverage-source-display skipped line", output_text,
                      "| MVI A,'B'\n");
}

static void test_cli_evaluates_identity_lambda_of_three_from_stdin(void) {
  char output_path[128];
  char command[320];
  char output_text[1024];
  int status;

  snprintf(output_path, sizeof(output_path), "/tmp/i8080_cli_output_%ld.txt",
           (long)getpid());
  snprintf(command, sizeof(command),
           "printf '(LAMBDA (X) X) (3) \n' | ./o/i8080emu src/lisp_8080_corrected.asm > %s",
           output_path);
  status = system(command);
  if (status == -1) {
    fprintf(stderr, "stdin cli command failed to launch\n");
    failures += 1;
    return;
  }
  if (!WIFEXITED(status)) {
    fprintf(stderr, "stdin cli command did not exit cleanly\n");
    failures += 1;
    return;
  }
  expect_int("stdin cli command exited", WEXITSTATUS(status), 0);
  if (!read_file_text(output_path, output_text, sizeof(output_text))) {
    return;
  }
  expect_text("stdin cli output", output_text, "\n>>3");
}

static void test_cli_null_queries_print_boolean_results(void) {
  char output_path[128];
  char command[512];
  char output_text[1024];
  int status;

  snprintf(output_path, sizeof(output_path), "/tmp/i8080_null_output_%ld.txt",
           (long)getpid());
  snprintf(command, sizeof(command),
           "printf 'NULL (NIL) \nNULL ((NIL)) \n' | ./o/i8080emu src/lisp_8080_corrected.asm > %s",
           output_path);
  status = system(command);
  if (status == -1) {
    fprintf(stderr, "null query command failed to launch\n");
    failures += 1;
    return;
  }
  if (!WIFEXITED(status)) {
    fprintf(stderr, "null query command did not exit cleanly\n");
    failures += 1;
    return;
  }
  expect_int("null query command exited", WEXITSTATUS(status), 0);
  if (!read_file_text(output_path, output_text, sizeof(output_text))) {
    return;
  }
  expect_text("null query output", output_text, "\n>>T\n>>F");
}

int main(void) {
  test_monitor_hooks_work_in_the_emulator();
  test_instruction_pointer_coverage_counts_executed_addresses();
  test_original_image_boots_to_the_monitor_loop();
  test_cli_writes_coverage_ndjson();
  test_cli_displays_compressed_covered_source_on_exit();
  test_cli_evaluates_identity_lambda_of_three_from_stdin();
  test_cli_null_queries_print_boolean_results();

  if (failures != 0) {
    fprintf(stderr, "7 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("7 tests 0 failures 0 skipped");
  return 0;
}
