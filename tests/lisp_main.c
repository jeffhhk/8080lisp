#include "lisp_host_io.h"
#include "lisp_runtime.h"

int main(void) {
  lisp_host_reset();
  lisp_boot();
  return 0;
}
