#include "i8080_emu.h"

#include <string.h>

enum {
  I8080_STEP_OK = 0,
  I8080_STEP_STOPPED = 1,
  I8080_STEP_LIMIT = 2,
};

static uint16_t hl(const i8080_cpu *cpu) { return (uint16_t)(cpu->h << 8) | cpu->l; }
static uint16_t bc(const i8080_cpu *cpu) { return (uint16_t)(cpu->b << 8) | cpu->c; }
static uint16_t de(const i8080_cpu *cpu) { return (uint16_t)(cpu->d << 8) | cpu->e; }

static void set_hl(i8080_cpu *cpu, uint16_t value) {
  cpu->h = (uint8_t)(value >> 8);
  cpu->l = (uint8_t)value;
}

static void set_bc(i8080_cpu *cpu, uint16_t value) {
  cpu->b = (uint8_t)(value >> 8);
  cpu->c = (uint8_t)value;
}

static void set_de(i8080_cpu *cpu, uint16_t value) {
  cpu->d = (uint8_t)(value >> 8);
  cpu->e = (uint8_t)value;
}

static uint8_t parity_even(uint8_t value) {
  uint8_t bits = 0;
  while (value != 0) {
    bits ^= (uint8_t)(value & 1u);
    value >>= 1;
  }
  return (uint8_t)(bits == 0);
}

static void set_szp(i8080_cpu *cpu, uint8_t value) {
  cpu->flags.z = (uint8_t)(value == 0);
  cpu->flags.s = (uint8_t)((value & 0x80) != 0);
  cpu->flags.p = parity_even(value);
}

static uint8_t pack_psw(const i8080_cpu *cpu) {
  return (uint8_t)((cpu->flags.s << 7) | (cpu->flags.z << 6) |
                   (cpu->flags.ac << 4) | (cpu->flags.p << 2) | 0x02 |
                   cpu->flags.cy);
}

static void unpack_psw(i8080_cpu *cpu, uint8_t value) {
  cpu->flags.s = (uint8_t)((value >> 7) & 1);
  cpu->flags.z = (uint8_t)((value >> 6) & 1);
  cpu->flags.ac = (uint8_t)((value >> 4) & 1);
  cpu->flags.p = (uint8_t)((value >> 2) & 1);
  cpu->flags.cy = (uint8_t)(value & 1);
}

static uint8_t read_mem(const i8080_cpu *cpu, uint16_t address) {
  return cpu->memory[address];
}

static void write_mem(i8080_cpu *cpu, uint16_t address, uint8_t value) {
  cpu->memory[address] = value;
}

static uint16_t read_word(const i8080_cpu *cpu, uint16_t address) {
  return (uint16_t)(read_mem(cpu, address) | (read_mem(cpu, (uint16_t)(address + 1)) << 8));
}

static void write_word(i8080_cpu *cpu, uint16_t address, uint16_t value) {
  write_mem(cpu, address, (uint8_t)(value & 0xff));
  write_mem(cpu, (uint16_t)(address + 1), (uint8_t)(value >> 8));
}

static void push_word(i8080_cpu *cpu, uint16_t value) {
  cpu->sp = (uint16_t)(cpu->sp - 2);
  write_word(cpu, cpu->sp, value);
}

static uint16_t pop_word(i8080_cpu *cpu) {
  uint16_t value = read_word(cpu, cpu->sp);
  cpu->sp = (uint16_t)(cpu->sp + 2);
  return value;
}

static uint8_t *reg_ptr(i8080_cpu *cpu, int code) {
  switch (code) {
  case 0: return &cpu->b;
  case 1: return &cpu->c;
  case 2: return &cpu->d;
  case 3: return &cpu->e;
  case 4: return &cpu->h;
  case 5: return &cpu->l;
  case 7: return &cpu->a;
  default: return NULL;
  }
}

static uint8_t get_reg(i8080_cpu *cpu, int code) {
  if (code == 6) {
    return read_mem(cpu, hl(cpu));
  }
  return *reg_ptr(cpu, code);
}

static void set_reg(i8080_cpu *cpu, int code, uint8_t value) {
  if (code == 6) {
    write_mem(cpu, hl(cpu), value);
    return;
  }
  *reg_ptr(cpu, code) = value;
}

static void set_logic_flags(i8080_cpu *cpu, uint8_t value, uint8_t ac) {
  set_szp(cpu, value);
  cpu->flags.cy = 0;
  cpu->flags.ac = ac;
}

static void add_byte(i8080_cpu *cpu, uint8_t value, uint8_t carry_in) {
  unsigned sum = (unsigned)cpu->a + value + carry_in;
  cpu->flags.ac = (uint8_t)((((cpu->a & 0x0f) + (value & 0x0f) + carry_in) & 0x10) != 0);
  cpu->flags.cy = (uint8_t)(sum > 0xff);
  cpu->a = (uint8_t)sum;
  set_szp(cpu, cpu->a);
}

static void sub_byte(i8080_cpu *cpu, uint8_t value, uint8_t borrow_in, int store) {
  int diff = (int)cpu->a - (int)value - (int)borrow_in;
  uint8_t result = (uint8_t)diff;
  cpu->flags.ac = (uint8_t)(((cpu->a & 0x0f) - (value & 0x0f) - borrow_in) < 0);
  cpu->flags.cy = (uint8_t)(diff < 0);
  set_szp(cpu, result);
  if (store) {
    cpu->a = result;
  }
}

static int handle_direct_hook(i8080_cpu *cpu) {
  if (cpu->pc == I8080_HOOK_ABEND) {
    cpu->halted = 1;
    cpu->abended = 1;
    cpu->abend_code = cpu->a;
    if (cpu->hooks.abend != NULL) {
      cpu->hooks.abend(cpu->hooks.ctx, cpu->a);
    }
    return 1;
  }
  return 0;
}

static int handle_call_hook(i8080_cpu *cpu, uint16_t target) {
  if (target == I8080_HOOK_OUTC) {
    if (cpu->hooks.outc != NULL) {
      cpu->hooks.outc(cpu->hooks.ctx, cpu->a);
    }
    return 1;
  }
  if (target == I8080_HOOK_CRLF) {
    if (cpu->hooks.outc != NULL) {
      cpu->hooks.outc(cpu->hooks.ctx, '\n');
    }
    return 1;
  }
  if (target == I8080_HOOK_INCH) {
    int ch = 0;
    if (cpu->hooks.inch != NULL) {
      ch = cpu->hooks.inch(cpu->hooks.ctx);
    }
    if (ch < 0) {
      ch = 0;
    }
    cpu->a = (uint8_t)ch;
    return 1;
  }
  if (target == I8080_HOOK_ABEND) {
    cpu->halted = 1;
    cpu->abended = 1;
    cpu->abend_code = cpu->a;
    if (cpu->hooks.abend != NULL) {
      cpu->hooks.abend(cpu->hooks.ctx, cpu->a);
    }
    return 1;
  }
  return 0;
}

void i8080_init(i8080_cpu *cpu, const i8080_hooks *hooks) {
  memset(cpu, 0, sizeof(*cpu));
  if (hooks != NULL) {
    cpu->hooks = *hooks;
  }
  cpu->sp = 0xfffe;
}

void i8080_load_image(i8080_cpu *cpu, const i8080_image *image) {
  size_t i;
  for (i = 0; i < I8080_IMAGE_SIZE; ++i) {
    if (image->used[i]) {
      cpu->memory[i] = image->bytes[i];
    }
  }
}

int i8080_step(i8080_cpu *cpu) {
  uint8_t opcode;
  cpu->steps += 1;

  if (cpu->halted) {
    return I8080_STEP_STOPPED;
  }
  if (handle_direct_hook(cpu)) {
    return I8080_STEP_STOPPED;
  }

  opcode = read_mem(cpu, cpu->pc++);

  if ((opcode & 0xc0) == 0x40 && opcode != 0x76) {
    int dst = (opcode >> 3) & 7;
    int src = opcode & 7;
    set_reg(cpu, dst, get_reg(cpu, src));
    return I8080_STEP_OK;
  }

  if ((opcode & 0xc7) == 0x04) {
    int reg = (opcode >> 3) & 7;
    uint8_t value = (uint8_t)(get_reg(cpu, reg) + 1);
    cpu->flags.ac = (uint8_t)(((get_reg(cpu, reg) & 0x0f) + 1) > 0x0f);
    set_reg(cpu, reg, value);
    set_szp(cpu, value);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xc7) == 0x05) {
    int reg = (opcode >> 3) & 7;
    uint8_t original = get_reg(cpu, reg);
    uint8_t value = (uint8_t)(original - 1);
    cpu->flags.ac = (uint8_t)((original & 0x0f) == 0);
    set_reg(cpu, reg, value);
    set_szp(cpu, value);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xc7) == 0x06) {
    int reg = (opcode >> 3) & 7;
    set_reg(cpu, reg, read_mem(cpu, cpu->pc++));
    return I8080_STEP_OK;
  }

  if ((opcode & 0xcf) == 0x01) {
    int rp = (opcode >> 4) & 3;
    uint16_t value = read_word(cpu, cpu->pc);
    cpu->pc = (uint16_t)(cpu->pc + 2);
    switch (rp) {
    case 0: set_bc(cpu, value); break;
    case 1: set_de(cpu, value); break;
    case 2: set_hl(cpu, value); break;
    case 3: cpu->sp = value; break;
    }
    return I8080_STEP_OK;
  }

  if ((opcode & 0xcf) == 0x03) {
    int rp = (opcode >> 4) & 3;
    switch (rp) {
    case 0: set_bc(cpu, (uint16_t)(bc(cpu) + 1)); break;
    case 1: set_de(cpu, (uint16_t)(de(cpu) + 1)); break;
    case 2: set_hl(cpu, (uint16_t)(hl(cpu) + 1)); break;
    case 3: cpu->sp = (uint16_t)(cpu->sp + 1); break;
    }
    return I8080_STEP_OK;
  }

  if ((opcode & 0xcf) == 0x0b) {
    int rp = (opcode >> 4) & 3;
    switch (rp) {
    case 0: set_bc(cpu, (uint16_t)(bc(cpu) - 1)); break;
    case 1: set_de(cpu, (uint16_t)(de(cpu) - 1)); break;
    case 2: set_hl(cpu, (uint16_t)(hl(cpu) - 1)); break;
    case 3: cpu->sp = (uint16_t)(cpu->sp - 1); break;
    }
    return I8080_STEP_OK;
  }

  if ((opcode & 0xcf) == 0x09) {
    int rp = (opcode >> 4) & 3;
    uint32_t sum = hl(cpu);
    uint16_t rhs = 0;
    switch (rp) {
    case 0: rhs = bc(cpu); break;
    case 1: rhs = de(cpu); break;
    case 2: rhs = hl(cpu); break;
    case 3: rhs = cpu->sp; break;
    }
    sum += rhs;
    cpu->flags.cy = (uint8_t)(sum > 0xffff);
    set_hl(cpu, (uint16_t)sum);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0x80) {
    add_byte(cpu, get_reg(cpu, opcode & 7), 0);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0x88) {
    add_byte(cpu, get_reg(cpu, opcode & 7), cpu->flags.cy);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0x90) {
    sub_byte(cpu, get_reg(cpu, opcode & 7), 0, 1);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0x98) {
    sub_byte(cpu, get_reg(cpu, opcode & 7), cpu->flags.cy, 1);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0xa0) {
    cpu->a &= get_reg(cpu, opcode & 7);
    set_logic_flags(cpu, cpu->a, 1);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0xa8) {
    cpu->a ^= get_reg(cpu, opcode & 7);
    set_logic_flags(cpu, cpu->a, 0);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0xb0) {
    cpu->a |= get_reg(cpu, opcode & 7);
    set_logic_flags(cpu, cpu->a, 0);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xf8) == 0xb8) {
    sub_byte(cpu, get_reg(cpu, opcode & 7), 0, 0);
    return I8080_STEP_OK;
  }

  if ((opcode & 0xcf) == 0xc1) {
    int rp = (opcode >> 4) & 3;
    uint16_t value = pop_word(cpu);
    switch (rp) {
    case 0: set_bc(cpu, value); break;
    case 1: set_de(cpu, value); break;
    case 2: set_hl(cpu, value); break;
    case 3: cpu->a = (uint8_t)(value >> 8); unpack_psw(cpu, (uint8_t)value); break;
    }
    return I8080_STEP_OK;
  }

  if ((opcode & 0xcf) == 0xc5) {
    int rp = (opcode >> 4) & 3;
    uint16_t value = 0;
    switch (rp) {
    case 0: value = bc(cpu); break;
    case 1: value = de(cpu); break;
    case 2: value = hl(cpu); break;
    case 3: value = (uint16_t)(cpu->a << 8) | pack_psw(cpu); break;
    }
    push_word(cpu, value);
    return I8080_STEP_OK;
  }

  switch (opcode) {
  case 0x00: return I8080_STEP_OK;
  case 0x02: write_mem(cpu, bc(cpu), cpu->a); return I8080_STEP_OK;
  case 0x0a: cpu->a = read_mem(cpu, bc(cpu)); return I8080_STEP_OK;
  case 0x12: write_mem(cpu, de(cpu), cpu->a); return I8080_STEP_OK;
  case 0x1a: cpu->a = read_mem(cpu, de(cpu)); return I8080_STEP_OK;
  case 0x22: write_word(cpu, read_word(cpu, cpu->pc), hl(cpu)); cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0x2a: set_hl(cpu, read_word(cpu, read_word(cpu, cpu->pc))); cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0x32: write_mem(cpu, read_word(cpu, cpu->pc), cpu->a); cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0x3a: cpu->a = read_mem(cpu, read_word(cpu, cpu->pc)); cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0x3c: cpu->a = (uint8_t)(cpu->a + 1); cpu->flags.ac = (uint8_t)(((cpu->a - 1) & 0x0f) == 0x0f); set_szp(cpu, cpu->a); return I8080_STEP_OK;
  case 0x3e: cpu->a = read_mem(cpu, cpu->pc++); return I8080_STEP_OK;
  case 0x76: cpu->halted = 1; return I8080_STEP_STOPPED;
  case 0xa7: set_logic_flags(cpu, cpu->a &= cpu->a, 1); return I8080_STEP_OK;
  case 0xaf: cpu->a = 0; set_logic_flags(cpu, cpu->a, 0); return I8080_STEP_OK;
  case 0xc0: if (!cpu->flags.z) cpu->pc = pop_word(cpu); return I8080_STEP_OK;
  case 0xc2: if (!cpu->flags.z) cpu->pc = read_word(cpu, cpu->pc); else cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0xc3: cpu->pc = read_word(cpu, cpu->pc); return I8080_STEP_OK;
  case 0xc6: add_byte(cpu, read_mem(cpu, cpu->pc++), 0); return I8080_STEP_OK;
  case 0xc8: if (cpu->flags.z) cpu->pc = pop_word(cpu); return I8080_STEP_OK;
  case 0xc9: cpu->pc = pop_word(cpu); return I8080_STEP_OK;
  case 0xca: if (cpu->flags.z) cpu->pc = read_word(cpu, cpu->pc); else cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0xcd: {
    uint16_t target = read_word(cpu, cpu->pc);
    cpu->pc = (uint16_t)(cpu->pc + 2);
    if (handle_call_hook(cpu, target)) {
      return cpu->halted ? I8080_STEP_STOPPED : I8080_STEP_OK;
    }
    push_word(cpu, cpu->pc);
    cpu->pc = target;
    return I8080_STEP_OK;
  }
  case 0xd1: set_de(cpu, pop_word(cpu)); return I8080_STEP_OK;
  case 0xd2: if (!cpu->flags.cy) cpu->pc = read_word(cpu, cpu->pc); else cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0xd5: push_word(cpu, de(cpu)); return I8080_STEP_OK;
  case 0xd8: if (cpu->flags.cy) cpu->pc = pop_word(cpu); return I8080_STEP_OK;
  case 0xda: if (cpu->flags.cy) cpu->pc = read_word(cpu, cpu->pc); else cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0xe1: set_hl(cpu, pop_word(cpu)); return I8080_STEP_OK;
  case 0xe3: {
    uint16_t tmp = hl(cpu);
    set_hl(cpu, read_word(cpu, cpu->sp));
    write_word(cpu, cpu->sp, tmp);
    return I8080_STEP_OK;
  }
  case 0xe5: push_word(cpu, hl(cpu)); return I8080_STEP_OK;
  case 0xe6: cpu->a &= read_mem(cpu, cpu->pc++); set_logic_flags(cpu, cpu->a, 1); return I8080_STEP_OK;
  case 0xeb: {
    uint16_t tmp = de(cpu);
    set_de(cpu, hl(cpu));
    set_hl(cpu, tmp);
    return I8080_STEP_OK;
  }
  case 0xf1: {
    uint16_t value = pop_word(cpu);
    cpu->a = (uint8_t)(value >> 8);
    unpack_psw(cpu, (uint8_t)value);
    return I8080_STEP_OK;
  }
  case 0xf2: if (!cpu->flags.s) cpu->pc = read_word(cpu, cpu->pc); else cpu->pc = (uint16_t)(cpu->pc + 2); return I8080_STEP_OK;
  case 0xf5: push_word(cpu, (uint16_t)(cpu->a << 8) | pack_psw(cpu)); return I8080_STEP_OK;
  case 0xf6: cpu->a |= read_mem(cpu, cpu->pc++); set_logic_flags(cpu, cpu->a, 0); return I8080_STEP_OK;
  case 0xf9: cpu->sp = hl(cpu); return I8080_STEP_OK;
  case 0xfe: sub_byte(cpu, read_mem(cpu, cpu->pc++), 0, 0); return I8080_STEP_OK;
  default:
    cpu->halted = 1;
    cpu->error = 1;
    cpu->error_opcode = opcode;
    return I8080_STEP_STOPPED;
  }
}

int i8080_run(i8080_cpu *cpu, size_t max_steps) {
  size_t steps = 0;
  while (!cpu->halted && steps < max_steps) {
    i8080_step(cpu);
    steps += 1;
  }
  if (!cpu->halted && steps >= max_steps) {
    return I8080_STEP_LIMIT;
  }
  return I8080_STEP_STOPPED;
}
