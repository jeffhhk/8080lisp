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
  expect_text("web eval null queries",
              i8080web_eval("NULL (NIL) \nNULL ((NIL)) \n"), "\n>>T\n>>F");
}

static void test_web_eval_runs_embedded_lambda_query(void) {
  expect_text("web eval lambda",
              i8080web_eval("(LAMBDA (X) X) (3) \n"), "\n>>3");
}

int main(void) {
  test_web_eval_runs_embedded_null_queries();
  test_web_eval_runs_embedded_lambda_query();

  if (failures != 0) {
    fprintf(stderr, "2 tests %d failures 0 skipped\n", failures);
    return 1;
  }

  puts("2 tests 0 failures 0 skipped");
  return 0;
}
