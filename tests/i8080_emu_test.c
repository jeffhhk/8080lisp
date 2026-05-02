#include "i8080_asm.h"
#include "i8080_emu.h"
#include "lisp_host_io.h"

#include <stdio.h>
#include <string.h>

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

int main(void) {
  test_monitor_hooks_work_in_the_emulator();
  test_instruction_pointer_coverage_counts_executed_addresses();
  test_original_image_boots_to_the_monitor_loop();

  if (failures != 0) {
    fprintf(stderr, "3 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("3 tests 0 failures 0 skipped");
  return 0;
}
