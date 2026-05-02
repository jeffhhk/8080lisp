#include "i8080_asm.h"
#include "i8080_emu.h"
#include "lisp_host_io.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
  LISP_NIL = 0x8000,
  LISP_EVQAL = 0x04b4,
  LISP_INLOOP = 0x0599,
  LISP_RET_STUB = 0x6f00,

  /* Encoded atom pointers observed in the original image. */
  LISP_ATOM_AT = 0x8431,
  LISP_CAR_AT = 0x8436,
  LISP_CDR_AT = 0x843a,
  LISP_COND_AT = 0x843e,
  LISP_CONS_AT = 0x8443,
  LISP_DEFINE_AT = 0x8448,
  LISP_EQ_AT = 0x844f,
  LISP_F_AT = 0x8452,
  LISP_LABEL_AT = 0x8454,
  LISP_LAMBDA_AT = 0x845a,
  LISP_QUOTE_AT = 0x8461,
  LISP_T_AT = 0x8467,
  LISP_NULL_AT = 0x8478,
  LISP_EQUAL_AT = 0x847d,
  LISP_PAIRLIS_AT = 0x8483,
  LISP_ASSOC_AT = 0x848b,
  LISP_EVAL_AT = 0x8491,
  LISP_APPLY_AT = 0x8496,

  /* Actual encoded call targets in the original image. */
  LISP_FN_CAR = 0x0009,
  LISP_FN_CDR = 0x0010,
  LISP_FN_CONS = 0x0016,
  LISP_FN_ATOM = 0x002b,
  LISP_FN_EQ = 0x0031,
  LISP_FN_ISLIST = 0x0037,
  LISP_FN_NULL = 0x0049,
  LISP_FN_OUTPUT = 0x0050,
  LISP_FN_EQUAL = 0x00b3,
  LISP_FN_PAIRLIS = 0x00eb,
  LISP_FN_ASSOC = 0x0116,
  LISP_FN_EVAL = 0x013c,
  LISP_FN_APPLY = 0x0196,
  LISP_FN_DEFNE = 0x05ff,
};

typedef struct {
  i8080_cpu cpu;
  i8080_coverage coverage;
  uint8_t abend_code;
} lisp_fixture;

static int failures;
static int original_image_loaded;
static i8080_image original_image;

static void expect_int(const char *label, int actual, int expected) {
  if (actual == expected) {
    return;
  }
  fprintf(stderr, "%s: expected %d but saw %d\n", label, expected, actual);
  failures += 1;
}

static void expect_u16(const char *label, uint16_t actual, uint16_t expected) {
  if (actual == expected) {
    return;
  }
  fprintf(stderr, "%s: expected 0x%04x but saw 0x%04x\n", label, expected,
          actual);
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

static uint16_t get_hl(const i8080_cpu *cpu) {
  return (uint16_t)((cpu->h << 8) | cpu->l);
}

static void set_hl(i8080_cpu *cpu, uint16_t value) {
  cpu->h = (uint8_t)(value >> 8);
  cpu->l = (uint8_t)value;
}

static void set_de(i8080_cpu *cpu, uint16_t value) {
  cpu->d = (uint8_t)(value >> 8);
  cpu->e = (uint8_t)value;
}

static void set_bc(i8080_cpu *cpu, uint16_t value) {
  cpu->b = (uint8_t)(value >> 8);
  cpu->c = (uint8_t)value;
}

static uint16_t read_u16(const i8080_cpu *cpu, uint16_t address) {
  return (uint16_t)(cpu->memory[address] | (cpu->memory[address + 1] << 8));
}

static void push_word(i8080_cpu *cpu, uint16_t value) {
  cpu->sp = (uint16_t)(cpu->sp - 2);
  cpu->memory[cpu->sp] = (uint8_t)(value & 0xff);
  cpu->memory[cpu->sp + 1] = (uint8_t)(value >> 8);
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

static int ensure_original_image(void) {
  i8080_asm_error error;
  if (original_image_loaded) {
    return 1;
  }
  if (!i8080_assemble_file("orig/lisp_8080_rawocr_2026-04-13.asm",
                           &original_image, &error)) {
    fprintf(stderr, "assemble original image: %s\n", error.message);
    failures += 1;
    return 0;
  }
  original_image_loaded = 1;
  return 1;
}

static int boot_fixture(lisp_fixture *fixture) {
  i8080_hooks hooks = {
      .inch = host_inch,
      .outc = host_outc,
      .abend = host_abend,
      .ctx = &fixture->abend_code,
  };
  size_t steps = 0;

  if (!ensure_original_image()) {
    return 0;
  }

  lisp_host_reset();
  fixture->abend_code = 0;
  i8080_init(&fixture->cpu, &hooks);
  i8080_load_image(&fixture->cpu, &original_image);
  i8080_coverage_reset(&fixture->coverage);
  i8080_set_coverage(&fixture->cpu, &fixture->coverage);

  while (steps < 10000 && fixture->cpu.halted == 0 &&
         fixture->cpu.pc != LISP_INLOOP) {
    i8080_step(&fixture->cpu);
    steps += 1;
  }

  expect_u16("boot pc", fixture->cpu.pc, LISP_INLOOP);
  expect_int("boot halted", fixture->cpu.halted, 0);
  expect_int("boot abended", fixture->cpu.abended, 0);
  expect_int("boot error", fixture->cpu.error, 0);
  return failures == 0;
}

static void expect_clean_call(const char *label, const lisp_fixture *fixture) {
  expect_int(label, fixture->cpu.error, 0);
  expect_int("unexpected abend", fixture->cpu.abended, 0);
  expect_int("unexpected abend code", fixture->abend_code, 0);
}

static void call_routine(lisp_fixture *fixture, uint16_t address) {
  fixture->cpu.memory[LISP_RET_STUB] = 0x76;
  fixture->cpu.halted = 0;
  fixture->cpu.error = 0;
  fixture->cpu.abended = 0;
  fixture->abend_code = 0;
  push_word(&fixture->cpu, LISP_RET_STUB);
  fixture->cpu.pc = address;
  i8080_run(&fixture->cpu, 100000);
}

static uint16_t cons_value(lisp_fixture *fixture, uint16_t car, uint16_t cdr) {
  set_hl(&fixture->cpu, car);
  set_de(&fixture->cpu, cdr);
  call_routine(fixture, LISP_FN_CONS);
  expect_clean_call("cons call", fixture);
  return get_hl(&fixture->cpu);
}

static uint16_t car_value(lisp_fixture *fixture, uint16_t pair) {
  set_hl(&fixture->cpu, pair);
  call_routine(fixture, LISP_FN_CAR);
  expect_clean_call("car call", fixture);
  return get_hl(&fixture->cpu);
}

static uint16_t cdr_value(lisp_fixture *fixture, uint16_t pair) {
  set_hl(&fixture->cpu, pair);
  call_routine(fixture, LISP_FN_CDR);
  expect_clean_call("cdr call", fixture);
  return get_hl(&fixture->cpu);
}

static int predicate_z_hl(lisp_fixture *fixture, uint16_t address,
                          uint16_t hl_value) {
  set_hl(&fixture->cpu, hl_value);
  call_routine(fixture, address);
  expect_clean_call("predicate call", fixture);
  return fixture->cpu.flags.z;
}

static int predicate_z_hl_de(lisp_fixture *fixture, uint16_t address,
                             uint16_t hl_value, uint16_t de_value) {
  set_hl(&fixture->cpu, hl_value);
  set_de(&fixture->cpu, de_value);
  call_routine(fixture, address);
  expect_clean_call("binary predicate call", fixture);
  return fixture->cpu.flags.z;
}

static const char *capture_output(lisp_fixture *fixture, uint16_t value) {
  lisp_host_clear_output();
  set_hl(&fixture->cpu, value);
  call_routine(fixture, LISP_FN_OUTPUT);
  expect_clean_call("output call", fixture);
  return lisp_host_output();
}

static uint16_t pairlis_value(lisp_fixture *fixture, uint16_t names,
                              uint16_t values, uint16_t env) {
  set_hl(&fixture->cpu, names);
  set_de(&fixture->cpu, values);
  set_bc(&fixture->cpu, env);
  call_routine(fixture, LISP_FN_PAIRLIS);
  expect_clean_call("pairlis call", fixture);
  return get_hl(&fixture->cpu);
}

static uint16_t assoc_value(lisp_fixture *fixture, uint16_t key,
                            uint16_t alist) {
  set_hl(&fixture->cpu, key);
  set_de(&fixture->cpu, alist);
  call_routine(fixture, LISP_FN_ASSOC);
  expect_clean_call("assoc call", fixture);
  return get_hl(&fixture->cpu);
}

static uint16_t eval_value(lisp_fixture *fixture, uint16_t expr,
                           uint16_t env) {
  set_hl(&fixture->cpu, expr);
  set_de(&fixture->cpu, env);
  call_routine(fixture, LISP_FN_EVAL);
  expect_clean_call("eval call", fixture);
  return get_hl(&fixture->cpu);
}

static uint16_t apply_value(lisp_fixture *fixture, uint16_t fn, uint16_t args,
                            uint16_t env) {
  set_hl(&fixture->cpu, fn);
  set_de(&fixture->cpu, args);
  set_bc(&fixture->cpu, env);
  call_routine(fixture, LISP_FN_APPLY);
  expect_clean_call("apply call", fixture);
  return get_hl(&fixture->cpu);
}

static uint16_t defne_value(lisp_fixture *fixture, uint16_t definitions) {
  set_hl(&fixture->cpu, definitions);
  call_routine(fixture, LISP_FN_DEFNE);
  expect_clean_call("defne call", fixture);
  return get_hl(&fixture->cpu);
}

static void expect_coverage_hit(const lisp_fixture *fixture, const char *label,
                                uint16_t address) {
  if (i8080_coverage_count(&fixture->coverage, address) != 0) {
    return;
  }
  fprintf(stderr, "%s: expected coverage at 0x%04x\n", label, address);
  failures += 1;
}

static void test_list_primitives_and_output(void) {
  lisp_fixture fixture;
  uint16_t tail;
  uint16_t list;
  uint16_t pair;

  if (!boot_fixture(&fixture)) {
    return;
  }

  tail = cons_value(&fixture, LISP_F_AT, LISP_NIL);
  list = cons_value(&fixture, LISP_T_AT, tail);
  pair = cons_value(&fixture, LISP_T_AT, LISP_F_AT);

  expect_text("list output", capture_output(&fixture, list), "(T F)");
  expect_u16("car(list)", car_value(&fixture, list), LISP_T_AT);
  expect_text("cdr(list) output", capture_output(&fixture, cdr_value(&fixture, list)),
              "(F)");
  expect_text("pair output", capture_output(&fixture, pair), "(T.F)");

  expect_int("ATOM(T)", predicate_z_hl(&fixture, LISP_FN_ATOM, LISP_T_AT), 1);
  expect_int("ATOM((T F))", predicate_z_hl(&fixture, LISP_FN_ATOM, list), 0);
  expect_int("ISLIST((T F))", predicate_z_hl(&fixture, LISP_FN_ISLIST, list), 1);
  expect_int("ISLIST((T.F))", predicate_z_hl(&fixture, LISP_FN_ISLIST, pair), 0);
  expect_int("NULL(NIL)", predicate_z_hl(&fixture, LISP_FN_NULL, LISP_NIL), 1);
  expect_int("NULL(T)", predicate_z_hl(&fixture, LISP_FN_NULL, LISP_T_AT), 0);

  expect_coverage_hit(&fixture, "cons coverage", LISP_FN_CONS);
  expect_coverage_hit(&fixture, "car coverage", LISP_FN_CAR);
  expect_coverage_hit(&fixture, "cdr coverage", LISP_FN_CDR);
  expect_coverage_hit(&fixture, "atom coverage", LISP_FN_ATOM);
  expect_coverage_hit(&fixture, "islist coverage", LISP_FN_ISLIST);
  expect_coverage_hit(&fixture, "null coverage", LISP_FN_NULL);
  expect_coverage_hit(&fixture, "output coverage", LISP_FN_OUTPUT);
}

static void test_comparison_and_environment_primitives(void) {
  lisp_fixture fixture;
  uint16_t pair_left;
  uint16_t pair_right;
  uint16_t names;
  uint16_t values;
  uint16_t alist;

  if (!boot_fixture(&fixture)) {
    return;
  }

  pair_left = cons_value(&fixture, LISP_T_AT, LISP_F_AT);
  pair_right = cons_value(&fixture, LISP_T_AT, LISP_F_AT);
  names = cons_value(&fixture, LISP_T_AT, cons_value(&fixture, LISP_F_AT, LISP_NIL));
  values = cons_value(&fixture, LISP_F_AT, cons_value(&fixture, LISP_T_AT, LISP_NIL));
  alist = pairlis_value(&fixture, names, values, LISP_NIL);

  expect_int("EQ(T,T)",
             predicate_z_hl_de(&fixture, LISP_FN_EQ, LISP_T_AT, LISP_T_AT), 1);
  expect_int("EQ(T,F)",
             predicate_z_hl_de(&fixture, LISP_FN_EQ, LISP_T_AT, LISP_F_AT), 0);
  expect_int("EQUAL((T.F),(T.F))",
             predicate_z_hl_de(&fixture, LISP_FN_EQUAL, pair_left, pair_right), 1);
  expect_text("PAIRLIS output", capture_output(&fixture, alist),
              "((T.F) (F.T))");
  expect_text("ASSOC(T)", capture_output(&fixture, assoc_value(&fixture, LISP_T_AT, alist)),
              "(T.F)");

  expect_coverage_hit(&fixture, "eq coverage", LISP_FN_EQ);
  expect_coverage_hit(&fixture, "equal coverage", LISP_FN_EQUAL);
  expect_coverage_hit(&fixture, "pairlis coverage", LISP_FN_PAIRLIS);
  expect_coverage_hit(&fixture, "assoc coverage", LISP_FN_ASSOC);
}

static void test_eval_quote_cond_and_application_paths(void) {
  lisp_fixture fixture;
  uint16_t quote_t;
  uint16_t quote_f;
  uint16_t quoted_pair;
  uint16_t cond_clause;
  uint16_t cond_expr;
  uint16_t car_expr;

  if (!boot_fixture(&fixture)) {
    return;
  }

  quote_t = cons_value(&fixture, LISP_QUOTE_AT,
                       cons_value(&fixture, LISP_T_AT, LISP_NIL));
  quote_f = cons_value(&fixture, LISP_QUOTE_AT,
                       cons_value(&fixture, LISP_F_AT, LISP_NIL));
  quoted_pair = cons_value(&fixture, LISP_QUOTE_AT,
                           cons_value(&fixture,
                                      cons_value(&fixture, LISP_T_AT, LISP_F_AT),
                                      LISP_NIL));
  cond_clause = cons_value(&fixture, quote_t, cons_value(&fixture, quote_f, LISP_NIL));
  cond_expr = cons_value(&fixture, LISP_COND_AT,
                         cons_value(&fixture, cond_clause, LISP_NIL));
  car_expr = cons_value(&fixture, LISP_CAR_AT,
                        cons_value(&fixture, quoted_pair, LISP_NIL));

  expect_u16("EVAL(QUOTE T)", eval_value(&fixture, quote_t, LISP_NIL), LISP_T_AT);
  expect_u16("EVAL(COND ...)", eval_value(&fixture, cond_expr, LISP_NIL), LISP_F_AT);
  expect_u16("EVAL(CAR (QUOTE (T.F)))", eval_value(&fixture, car_expr, LISP_NIL),
             LISP_T_AT);

  expect_coverage_hit(&fixture, "eval coverage", LISP_FN_EVAL);
  expect_coverage_hit(&fixture, "apply coverage", LISP_FN_APPLY);
  expect_coverage_hit(&fixture, "car coverage through eval", LISP_FN_CAR);
}

static void test_eval_atoms_and_apply_lambda_label(void) {
  lisp_fixture fixture;
  uint16_t env_names;
  uint16_t env_values;
  uint16_t env;
  uint16_t lambda;
  uint16_t args;
  uint16_t label_fn;

  if (!boot_fixture(&fixture)) {
    return;
  }

  env_names = cons_value(&fixture, LISP_T_AT, LISP_NIL);
  env_values = cons_value(&fixture, LISP_F_AT, LISP_NIL);
  env = pairlis_value(&fixture, env_names, env_values, LISP_NIL);
  lambda = cons_value(&fixture, LISP_LAMBDA_AT,
                      cons_value(&fixture,
                                 cons_value(&fixture, LISP_T_AT, LISP_NIL),
                                 cons_value(&fixture, LISP_T_AT, LISP_NIL)));
  args = cons_value(&fixture, LISP_F_AT, LISP_NIL);
  label_fn = cons_value(&fixture, LISP_LABEL_AT,
                        cons_value(&fixture, LISP_ATOM_AT,
                                   cons_value(&fixture, lambda, LISP_NIL)));

  expect_u16("EVAL(T) with env", eval_value(&fixture, LISP_T_AT, env), LISP_F_AT);
  expect_u16("APPLY(lambda,(F),NIL)", apply_value(&fixture, lambda, args, LISP_NIL),
             LISP_F_AT);
  expect_u16("APPLY(label,(F),NIL)",
             apply_value(&fixture, label_fn, args, LISP_NIL), LISP_F_AT);

  expect_coverage_hit(&fixture, "pairlis coverage from lambda", LISP_FN_PAIRLIS);
  expect_coverage_hit(&fixture, "apply coverage from lambda", LISP_FN_APPLY);
  expect_coverage_hit(&fixture, "eval coverage from env lookup", LISP_FN_EVAL);
}

static void test_definition_routine_updates_global_environment(void) {
  lisp_fixture fixture;
  uint16_t quote_nil;
  uint16_t definition;
  uint16_t definition_list;
  uint16_t result;
  uint16_t updated_env;

  if (!boot_fixture(&fixture)) {
    return;
  }

  quote_nil = cons_value(&fixture, LISP_QUOTE_AT,
                         cons_value(&fixture, LISP_NIL, LISP_NIL));
  definition = cons_value(&fixture, LISP_T_AT,
                          cons_value(&fixture, quote_nil, LISP_NIL));
  definition_list = cons_value(&fixture, definition, LISP_NIL);
  result = defne_value(&fixture, definition_list);
  updated_env = read_u16(&fixture.cpu, LISP_EVQAL);

  expect_text("definition result", capture_output(&fixture, result), "(T)");
  expect_text("updated EVQAL binding",
              capture_output(&fixture,
                             assoc_value(&fixture, LISP_T_AT, updated_env)),
              "(T QUOTE NIL)");

  expect_coverage_hit(&fixture, "defne coverage", LISP_FN_DEFNE);
}

int main(void) {
  test_list_primitives_and_output();
  test_comparison_and_environment_primitives();
  test_eval_quote_cond_and_application_paths();
  test_eval_atoms_and_apply_lambda_label();
  test_definition_routine_updates_global_environment();

  if (failures != 0) {
    fprintf(stderr, "5 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("5 tests 0 failures 0 skipped");
  return 0;
}
