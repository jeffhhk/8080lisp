#include "lisp_runtime.h"

#include <stdio.h>
#include <string.h>

enum {
  MEM = 0x0700,
  FREEBLK = 64,
  CURCH_OFF = 0x049c,
  ATL_OFF = 0x049d,
  CVTFQ_OFF = 0x04ae,
  EVQAL_OFF = 0x04b4,
  NIL_PTR = 0x8000,
  MEMORY_SIZE = 65536,
};

extern int lisp_host_inch(void);
extern void lisp_host_outc(int ch);

static uint8_t lisp_memory[MEMORY_SIZE];

static void write_u16(unsigned offset, uint16_t value) {
  lisp_memory[offset] = (uint8_t)(value & 0xff);
  lisp_memory[offset + 1] = (uint8_t)(value >> 8);
}

void lisp_boot(void) {
  memset(lisp_memory, 0, sizeof(lisp_memory));
  write_u16(CVTFQ_OFF, MEM);
  lisp_memory[MEM] = 0xff;
  lisp_memory[MEM + 1] = 0xff;
  lisp_memory[MEM + 2] = 0x00;
  lisp_memory[MEM + 3] = FREEBLK;
  write_u16(EVQAL_OFF, NIL_PTR);
  lisp_memory[CURCH_OFF] = 0x00;
  lisp_memory[ATL_OFF] = 0x00;
}

uint8_t *lisp_memory_base(void) { return lisp_memory; }

void lisp_outc(unsigned int ch) { lisp_host_outc((int)(uint8_t)ch); }

void lisp_crlf(void) { lisp_outc('\n'); }

unsigned int lisp_inch(void) {
  int ch = lisp_host_inch();

  if (ch == EOF) {
    return 0;
  }
  return (unsigned int)(uint8_t)ch;
}
