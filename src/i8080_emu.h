#ifndef I8080_EMU_H
#define I8080_EMU_H

#include "i8080_asm.h"

#include <stddef.h>
#include <stdint.h>

enum {
  I8080_HOOK_ABEND = 0xf000,
  I8080_HOOK_INCH = 0xf006,
  I8080_HOOK_OUTC = 0xf009,
  I8080_HOOK_CRLF = 0xf021,
};

enum {
  I8080_STEP_OK = 0,
  I8080_STEP_STOPPED = 1,
  I8080_STEP_LIMIT = 2,
};

typedef int (*i8080_inch_fn)(void *ctx);
typedef void (*i8080_outc_fn)(void *ctx, uint8_t ch);
typedef void (*i8080_abend_fn)(void *ctx, uint8_t code);

typedef struct {
  i8080_inch_fn inch;
  i8080_outc_fn outc;
  i8080_abend_fn abend;
  void *ctx;
} i8080_hooks;

typedef struct {
  size_t ip_hits[I8080_IMAGE_SIZE];
} i8080_coverage;

typedef struct {
  uint8_t z;
  uint8_t s;
  uint8_t p;
  uint8_t cy;
  uint8_t ac;
} i8080_flags;

typedef struct {
  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;
  uint8_t h;
  uint8_t l;
  uint16_t sp;
  uint16_t pc;
  uint8_t halted;
  uint8_t abended;
  uint8_t error;
  uint8_t error_opcode;
  uint8_t abend_code;
  size_t steps;
  i8080_coverage *coverage;
  i8080_flags flags;
  i8080_hooks hooks;
  uint8_t memory[I8080_IMAGE_SIZE];
} i8080_cpu;

void i8080_init(i8080_cpu *cpu, const i8080_hooks *hooks);
void i8080_load_image(i8080_cpu *cpu, const i8080_image *image);
void i8080_coverage_reset(i8080_coverage *coverage);
void i8080_set_coverage(i8080_cpu *cpu, i8080_coverage *coverage);
size_t i8080_coverage_count(const i8080_coverage *coverage, uint16_t address);
int i8080_step(i8080_cpu *cpu);
int i8080_run(i8080_cpu *cpu, size_t max_steps);

#endif
