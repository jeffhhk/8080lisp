#ifndef LISP_HOST_IO_H
#define LISP_HOST_IO_H

void lisp_host_reset(void);
void lisp_host_set_input(const char *text);
void lisp_host_clear_output(void);
const char *lisp_host_output(void);

void lisp_host_outc(int ch);
int lisp_host_inch(void);

#endif
