#ifndef LISP_RUNTIME_H
#define LISP_RUNTIME_H

#include <stdint.h>

void lisp_boot(void);
uint8_t *lisp_memory_base(void);

void lisp_outc(unsigned int ch);
void lisp_crlf(void);
unsigned int lisp_inch(void);

#endif
