#include "i8080_web_app.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void expect_text(const char *label, const char *actual,
                        const char *expected) {
  if (strcmp(actual, expected) == 0) {
    return;
  }

  fprintf(stderr, "%s: expected \"%s\" but saw \"%s\"\n", label, expected,
          actual);
  failures += 1;
}

static void test_web_eval_runs_embedded_null_queries(void) {
  i8080web_restart();
  expect_text("web eval null queries",
              i8080web_eval("NULL (NIL) \nNULL ((NIL)) \n"), "\n>>T\n>>F");
}

static void test_web_eval_runs_embedded_lambda_query(void) {
  i8080web_restart();
  expect_text("web eval lambda",
              i8080web_eval("(LAMBDA (X) X) (3) \n"), "\n>>3");
}

static void test_web_eval_preserves_monitor_state(void) {
  i8080web_restart();
  expect_text("web eval define",
              i8080web_eval("DEFINE (((ID (LAMBDA (X) X)))) \n"), "\n>>(ID)");
  expect_text("web eval preserved definition", i8080web_eval("ID (3) \n"),
              "\n>>3");
}

static void test_web_restart_clears_monitor_output_state(void) {
  i8080web_restart();
  expect_text("web eval define before restart",
              i8080web_eval("DEFINE (((ID (LAMBDA (X) X)))) \n"), "\n>>(ID)");
  i8080web_restart();
  expect_text("web eval after restart", i8080web_eval("ID (3) \n"),
              "\nABEND 0x00\n");
}

int main(void) {
  test_web_eval_runs_embedded_null_queries();
  test_web_eval_runs_embedded_lambda_query();
  test_web_eval_preserves_monitor_state();
  test_web_restart_clears_monitor_output_state();

  if (failures != 0) {
    fprintf(stderr, "4 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("4 tests 0 failures 0 skipped");
  return 0;
}
