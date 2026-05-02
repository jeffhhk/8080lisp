#include "i8080_asm.h"
#include "i8080_listing.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  MAX_SYMBOLS = 512,
  MAX_NAME = 32,
};

typedef struct {
  char name[MAX_NAME];
  uint16_t value;
} symbol_entry;

typedef struct {
  const char *name;
  const char *text;
  size_t line_number;
  symbol_entry symbols[MAX_SYMBOLS];
  size_t symbol_count;
} assembler;

static void set_error(i8080_asm_error *error, size_t line_number,
                      const char *message) {
  if (error == NULL) {
    return;
  }
  error->line_number = line_number;
  snprintf(error->message, sizeof(error->message), "%s", message);
}

static char *dup_text(const char *text) {
  size_t len = strlen(text);
  char *copy = malloc(len + 1);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, text, len + 1);
  return copy;
}

static char *next_line(char **cursor) {
  char *start;
  char *end;

  if (cursor == NULL || *cursor == NULL || **cursor == '\0') {
    return NULL;
  }

  start = *cursor;
  end = strchr(start, '\n');
  if (end == NULL) {
    *cursor = start + strlen(start);
    return start;
  }

  *end = '\0';
  *cursor = end + 1;
  return start;
}

void i8080_image_init(i8080_image *image) {
  memset(image, 0, sizeof(*image));
}

size_t i8080_image_size(const i8080_image *image) {
  return (size_t)(image->limit - image->origin);
}

static void trim(char *text) {
  size_t len;
  size_t start = 0;

  while (isspace((unsigned char)text[start])) {
    start += 1;
  }
  if (start != 0) {
    memmove(text, text + start, strlen(text + start) + 1);
  }

  len = strlen(text);
  while (len > 0 && isspace((unsigned char)text[len - 1])) {
    text[len - 1] = '\0';
    len -= 1;
  }
}

static void uppercase_copy(char *dst, size_t dst_size, const char *src) {
  size_t i;
  for (i = 0; src[i] != '\0' && i + 1 < dst_size; ++i) {
    dst[i] = (char)toupper((unsigned char)src[i]);
  }
  dst[i] = '\0';
}

static int parse_number_token(const char *token, int *value) {
  char upper[64];
  char *end = NULL;
  long parsed;
  size_t len = strlen(token);

  if (len >= sizeof(upper)) {
    return 0;
  }
  uppercase_copy(upper, sizeof(upper), token);

  if (upper[0] == '\'' && len >= 2 && upper[len - 1] == '\'') {
    size_t inner_len = len - 2;
    const unsigned char *inner = (const unsigned char *)(token + 1);
    if (inner_len == 1) {
      *value = inner[0];
      return 1;
    }
    if (inner_len == 2) {
      *value = inner[0] | ((int)inner[1] << 8);
      return 1;
    }
    return 0;
  }

  if (len > 1 && upper[len - 1] == 'H') {
    char hex[64];
    memcpy(hex, upper, len - 1);
    hex[len - 1] = '\0';
    parsed = strtol(hex, &end, 16);
    if (end != NULL && *end == '\0') {
      *value = (int)parsed;
      return 1;
    }
  }

  parsed = strtol(upper, &end, 0);
  if (end != NULL && *end == '\0') {
    *value = (int)parsed;
    return 1;
  }

  return 0;
}

static const symbol_entry *find_symbol(const assembler *state,
                                       const char *name) {
  char upper[MAX_NAME];
  size_t i;

  uppercase_copy(upper, sizeof(upper), name);
  for (i = 0; i < state->symbol_count; ++i) {
    if (strcmp(state->symbols[i].name, upper) == 0) {
      return &state->symbols[i];
    }
  }
  return NULL;
}

static int define_symbol(assembler *state, const char *name, uint16_t value,
                         i8080_asm_error *error) {
  symbol_entry *entry;
  char upper[MAX_NAME];

  if (state->symbol_count >= MAX_SYMBOLS) {
    set_error(error, state->line_number, "symbol table exhausted");
    return 0;
  }

  uppercase_copy(upper, sizeof(upper), name);
  if (find_symbol(state, upper) != NULL) {
    set_error(error, state->line_number, "duplicate symbol");
    return 0;
  }

  entry = &state->symbols[state->symbol_count++];
  snprintf(entry->name, sizeof(entry->name), "%s", upper);
  entry->value = value;
  return 1;
}

static int eval_expr(const assembler *state, const char *expr, int pass,
                     int *value, i8080_asm_error *error) {
  char buffer[256];
  char token[64];
  size_t pos = 0;
  int sign = 1;
  int total = 0;

  if (strlen(expr) >= sizeof(buffer)) {
    set_error(error, state->line_number, "expression too long");
    return 0;
  }
  snprintf(buffer, sizeof(buffer), "%s", expr);
  trim(buffer);

  while (buffer[pos] != '\0') {
    int number = 0;
    size_t tok_len = 0;
    token[0] = '\0';

    while (isspace((unsigned char)buffer[pos])) {
      pos += 1;
    }
    if (buffer[pos] == '+') {
      sign = 1;
      pos += 1;
      continue;
    }
    if (buffer[pos] == '-') {
      sign = -1;
      pos += 1;
      continue;
    }

    if (buffer[pos] == '\'') {
      size_t start = pos;
      pos += 1;
      while (buffer[pos] != '\0' && buffer[pos] != '\'') {
        pos += 1;
      }
      if (buffer[pos] != '\'') {
        set_error(error, state->line_number, "unterminated character literal");
        return 0;
      }
      pos += 1;
      tok_len = pos - start;
      memcpy(token, buffer + start, tok_len);
      token[tok_len] = '\0';
    } else {
      size_t start = pos;
      while (buffer[pos] != '\0' && !isspace((unsigned char)buffer[pos]) &&
             buffer[pos] != '+' && buffer[pos] != '-') {
        pos += 1;
      }
      tok_len = pos - start;
      memcpy(token, buffer + start, tok_len);
      token[tok_len] = '\0';
    }

    if (tok_len >= sizeof(token)) {
      set_error(error, state->line_number, "expression token too long");
      return 0;
    }
    if (token[0] == '\0') {
      memcpy(token, buffer + (pos - tok_len), tok_len);
      token[tok_len] = '\0';
    }

    if (parse_number_token(token, &number)) {
      total += sign * number;
      sign = 1;
      continue;
    }

    if (pass == 1) {
      number = 0;
    } else {
      const symbol_entry *symbol = find_symbol(state, token);
      if (symbol == NULL) {
        set_error(error, state->line_number, "unknown symbol");
        return 0;
      }
      number = symbol->value;
    }

    total += sign * number;
    sign = 1;
  }

  *value = total;
  return 1;
}

static int reg_code(const char *name) {
  static const char *names[] = {"B", "C", "D", "E", "H", "L", "M", "A"};
  int i;
  for (i = 0; i < 8; ++i) {
    if (strcmp(name, names[i]) == 0) {
      return i;
    }
  }
  return -1;
}

static int rp_code(const char *name, int allow_psw) {
  if (strcmp(name, "B") == 0 || strcmp(name, "BC") == 0) {
    return 0;
  }
  if (strcmp(name, "D") == 0 || strcmp(name, "DE") == 0) {
    return 1;
  }
  if (strcmp(name, "H") == 0 || strcmp(name, "HL") == 0) {
    return 2;
  }
  if (strcmp(name, "SP") == 0) {
    return allow_psw ? -1 : 3;
  }
  if (allow_psw && strcmp(name, "PSW") == 0) {
    return 3;
  }
  return -1;
}

static void write_byte(i8080_image *image, uint16_t address, uint8_t value,
                       size_t source_line) {
  image->bytes[address] = value;
  image->used[address] = 1;
  image->source_lines[address] = source_line;
  if (address < image->origin || image->limit == 0) {
    image->origin = address;
  }
  if ((uint16_t)(address + 1) > image->limit) {
    image->limit = (uint16_t)(address + 1);
  }
}

static void write_word(i8080_image *image, uint16_t address, uint16_t value,
                       size_t source_line) {
  write_byte(image, address, (uint8_t)(value & 0xff), source_line);
  write_byte(image, (uint16_t)(address + 1), (uint8_t)(value >> 8),
             source_line);
}

static int parse_operands(const char *text, char operands[][64], int max_ops) {
  char buffer[256];
  char *cursor = buffer;
  int count = 0;

  snprintf(buffer, sizeof(buffer), "%s", text);
  while (*cursor != '\0' && count < max_ops) {
    char *comma = strchr(cursor, ',');
    if (comma != NULL) {
      *comma = '\0';
    }
    trim(cursor);
    if (*cursor != '\0') {
      uppercase_copy(operands[count], 64, cursor);
      count += 1;
    }
    if (comma == NULL) {
      break;
    }
    cursor = comma + 1;
  }
  return count;
}

static int instruction_size(const char *opcode, int operand_count) {
  if (strcmp(opcode, "DB") == 0) {
    return operand_count;
  }
  if (strcmp(opcode, "DW") == 0) {
    return operand_count * 2;
  }
  if (strcmp(opcode, "DS") == 0) {
    return -2;
  }
  if (strcmp(opcode, "ORG") == 0 || strcmp(opcode, "EQU") == 0) {
    return 0;
  }
  if (strcmp(opcode, "LXI") == 0 || strcmp(opcode, "JMP") == 0 ||
      strcmp(opcode, "JNZ") == 0 || strcmp(opcode, "JZ") == 0 ||
      strcmp(opcode, "JNC") == 0 || strcmp(opcode, "JC") == 0 ||
      strcmp(opcode, "JP") == 0 || strcmp(opcode, "JM") == 0 ||
      strcmp(opcode, "CALL") == 0 || strcmp(opcode, "CNZ") == 0 ||
      strcmp(opcode, "CZ") == 0 || strcmp(opcode, "CNC") == 0 ||
      strcmp(opcode, "CC") == 0 || strcmp(opcode, "CP") == 0 ||
      strcmp(opcode, "CM") == 0 || strcmp(opcode, "LDA") == 0 ||
      strcmp(opcode, "STA") == 0 || strcmp(opcode, "LHLD") == 0 ||
      strcmp(opcode, "SHLD") == 0) {
    return 3;
  }
  if (strcmp(opcode, "MVI") == 0 || strcmp(opcode, "ADI") == 0 ||
      strcmp(opcode, "ACI") == 0 || strcmp(opcode, "SUI") == 0 ||
      strcmp(opcode, "SBI") == 0 || strcmp(opcode, "ANI") == 0 ||
      strcmp(opcode, "XRI") == 0 || strcmp(opcode, "ORI") == 0 ||
      strcmp(opcode, "CPI") == 0) {
    return 2;
  }
  return 1;
}

static int encode_instruction(const assembler *state, const char *opcode,
                              char operands[][64], int operand_count,
                              int pass, uint16_t pc, i8080_image *image,
                              uint16_t *next_pc, i8080_asm_error *error) {
  int value = 0;
  int r = 0;
  int rp = 0;
  int base = 0;
  *next_pc = pc;

  if (strcmp(opcode, "ORG") == 0) {
    if (!eval_expr(state, operands[0], pass, &value, error)) {
      return 0;
    }
    *next_pc = (uint16_t)value;
    return 1;
  }

  if (strcmp(opcode, "DS") == 0) {
    if (!eval_expr(state, operands[0], pass, &value, error)) {
      return 0;
    }
    if (pass == 2) {
      uint16_t addr;
      for (addr = pc; addr < (uint16_t)(pc + value); ++addr) {
        write_byte(image, addr, 0x00, state->line_number);
      }
    }
    *next_pc = (uint16_t)(pc + value);
    return 1;
  }

  if (strcmp(opcode, "DB") == 0) {
    int i;
    for (i = 0; i < operand_count; ++i) {
      if (!eval_expr(state, operands[i], pass, &value, error)) {
        return 0;
      }
      if (pass == 2) {
        write_byte(image, (uint16_t)(pc + i), (uint8_t)value,
                   state->line_number);
      }
    }
    *next_pc = (uint16_t)(pc + operand_count);
    return 1;
  }

  if (strcmp(opcode, "DW") == 0) {
    int i;
    for (i = 0; i < operand_count; ++i) {
      if (!eval_expr(state, operands[i], pass, &value, error)) {
        return 0;
      }
      if (pass == 2) {
        write_word(image, (uint16_t)(pc + i * 2), (uint16_t)value,
                   state->line_number);
      }
    }
    *next_pc = (uint16_t)(pc + operand_count * 2);
    return 1;
  }

  if (strcmp(opcode, "NOP") == 0) {
    base = 0x00;
  } else if (strcmp(opcode, "HLT") == 0) {
    base = 0x76;
  } else if (strcmp(opcode, "RET") == 0) {
    base = 0xc9;
  } else if (strcmp(opcode, "RNZ") == 0) {
    base = 0xc0;
  } else if (strcmp(opcode, "RZ") == 0) {
    base = 0xc8;
  } else if (strcmp(opcode, "RNC") == 0) {
    base = 0xd0;
  } else if (strcmp(opcode, "RC") == 0) {
    base = 0xd8;
  } else if (strcmp(opcode, "RP") == 0) {
    base = 0xf0;
  } else if (strcmp(opcode, "RM") == 0) {
    base = 0xf8;
  } else if (strcmp(opcode, "XCHG") == 0) {
    base = 0xeb;
  } else if (strcmp(opcode, "XTHL") == 0) {
    base = 0xe3;
  } else if (strcmp(opcode, "SPHL") == 0) {
    base = 0xf9;
  } else if (strcmp(opcode, "STC") == 0) {
    base = 0x37;
  } else if (strcmp(opcode, "CMA") == 0) {
    base = 0x2f;
  } else if (strcmp(opcode, "CMC") == 0) {
    base = 0x3f;
  } else if (strcmp(opcode, "RLC") == 0) {
    base = 0x07;
  } else if (strcmp(opcode, "RRC") == 0) {
    base = 0x0f;
  } else if (strcmp(opcode, "RAL") == 0) {
    base = 0x17;
  } else if (strcmp(opcode, "RAR") == 0) {
    base = 0x1f;
  } else if (strcmp(opcode, "DAA") == 0) {
    base = 0x27;
  }
  if (base != 0) {
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)base, state->line_number);
    }
    *next_pc = (uint16_t)(pc + 1);
    return 1;
  }

  if (strcmp(opcode, "JMP") == 0 || strcmp(opcode, "JNZ") == 0 ||
      strcmp(opcode, "JZ") == 0 || strcmp(opcode, "JNC") == 0 ||
      strcmp(opcode, "JC") == 0 || strcmp(opcode, "JP") == 0 ||
      strcmp(opcode, "JM") == 0 || strcmp(opcode, "CALL") == 0 ||
      strcmp(opcode, "CNZ") == 0 || strcmp(opcode, "CZ") == 0 ||
      strcmp(opcode, "CNC") == 0 || strcmp(opcode, "CC") == 0 ||
      strcmp(opcode, "CP") == 0 || strcmp(opcode, "CM") == 0 ||
      strcmp(opcode, "LDA") == 0 || strcmp(opcode, "STA") == 0 ||
      strcmp(opcode, "LHLD") == 0 || strcmp(opcode, "SHLD") == 0) {
    struct jump_entry {
      const char *name;
      uint8_t opcode;
    };
    static const struct jump_entry jumps[] = {
        {"JMP", 0xc3}, {"JNZ", 0xc2}, {"JZ", 0xca}, {"JNC", 0xd2},
        {"JC", 0xda},  {"JP", 0xf2},  {"JM", 0xfa}, {"CALL", 0xcd},
        {"CNZ", 0xc4}, {"CZ", 0xcc},  {"CNC", 0xd4}, {"CC", 0xdc},
        {"CP", 0xf4},  {"CM", 0xfc},  {"LDA", 0x3a}, {"STA", 0x32},
        {"LHLD", 0x2a}, {"SHLD", 0x22},
    };
    size_t i;
    for (i = 0; i < sizeof(jumps) / sizeof(jumps[0]); ++i) {
      if (strcmp(opcode, jumps[i].name) == 0) {
        if (!eval_expr(state, operands[0], pass, &value, error)) {
          return 0;
        }
        if (pass == 2) {
          write_byte(image, pc, jumps[i].opcode, state->line_number);
          write_word(image, (uint16_t)(pc + 1), (uint16_t)value,
                     state->line_number);
        }
        *next_pc = (uint16_t)(pc + 3);
        return 1;
      }
    }
  }

  if (strcmp(opcode, "MOV") == 0) {
    int dst = reg_code(operands[0]);
    int src = reg_code(operands[1]);
    if (dst < 0 || src < 0) {
      set_error(error, state->line_number, "invalid MOV register");
      return 0;
    }
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)(0x40 | (dst << 3) | src),
                 state->line_number);
    }
    *next_pc = (uint16_t)(pc + 1);
    return 1;
  }

  if (strcmp(opcode, "MVI") == 0) {
    r = reg_code(operands[0]);
    if (r < 0) {
      set_error(error, state->line_number, "invalid MVI register");
      return 0;
    }
    if (!eval_expr(state, operands[1], pass, &value, error)) {
      return 0;
    }
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)(0x06 | (r << 3)), state->line_number);
      write_byte(image, (uint16_t)(pc + 1), (uint8_t)value,
                 state->line_number);
    }
    *next_pc = (uint16_t)(pc + 2);
    return 1;
  }

  if (strcmp(opcode, "LXI") == 0) {
    rp = rp_code(operands[0], 0);
    if (rp < 0) {
      set_error(error, state->line_number, "invalid LXI register pair");
      return 0;
    }
    if (!eval_expr(state, operands[1], pass, &value, error)) {
      return 0;
    }
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)(0x01 | (rp << 4)), state->line_number);
      write_word(image, (uint16_t)(pc + 1), (uint16_t)value,
                 state->line_number);
    }
    *next_pc = (uint16_t)(pc + 3);
    return 1;
  }

  if (strcmp(opcode, "INX") == 0 || strcmp(opcode, "DCX") == 0 ||
      strcmp(opcode, "DAD") == 0) {
    rp = rp_code(operands[0], 0);
    if (rp < 0) {
      set_error(error, state->line_number, "invalid register pair");
      return 0;
    }
    base = strcmp(opcode, "INX") == 0 ? 0x03
           : strcmp(opcode, "DCX") == 0 ? 0x0b
                                         : 0x09;
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)(base | (rp << 4)), state->line_number);
    }
    *next_pc = (uint16_t)(pc + 1);
    return 1;
  }

  if (strcmp(opcode, "PUSH") == 0 || strcmp(opcode, "POP") == 0) {
    rp = rp_code(operands[0], 1);
    if (rp < 0) {
      set_error(error, state->line_number, "invalid stack register pair");
      return 0;
    }
    base = strcmp(opcode, "PUSH") == 0 ? 0xc5 : 0xc1;
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)(base | (rp << 4)), state->line_number);
    }
    *next_pc = (uint16_t)(pc + 1);
    return 1;
  }

  if (strcmp(opcode, "LDAX") == 0 || strcmp(opcode, "STAX") == 0) {
    rp = rp_code(operands[0], 0);
    if (rp < 0 || rp > 1) {
      set_error(error, state->line_number, "LDAX/STAX needs B or D");
      return 0;
    }
    base = strcmp(opcode, "LDAX") == 0 ? 0x0a : 0x02;
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)(base | (rp << 4)), state->line_number);
    }
    *next_pc = (uint16_t)(pc + 1);
    return 1;
  }

  if (strcmp(opcode, "INR") == 0 || strcmp(opcode, "DCR") == 0) {
    r = reg_code(operands[0]);
    if (r < 0) {
      set_error(error, state->line_number, "invalid single register");
      return 0;
    }
    base = strcmp(opcode, "INR") == 0 ? 0x04 : 0x05;
    if (pass == 2) {
      write_byte(image, pc, (uint8_t)(base | (r << 3)), state->line_number);
    }
    *next_pc = (uint16_t)(pc + 1);
    return 1;
  }

  {
    struct alu_entry {
      const char *name;
      uint8_t opcode;
    };
    static const struct alu_entry alur[] = {
        {"ADD", 0x80}, {"ADC", 0x88}, {"SUB", 0x90}, {"SBB", 0x98},
        {"ANA", 0xa0}, {"XRA", 0xa8}, {"ORA", 0xb0}, {"CMP", 0xb8},
    };
    static const struct alu_entry alui[] = {
        {"ADI", 0xc6}, {"ACI", 0xce}, {"SUI", 0xd6}, {"SBI", 0xde},
        {"ANI", 0xe6}, {"XRI", 0xee}, {"ORI", 0xf6}, {"CPI", 0xfe},
    };
    size_t i;

    for (i = 0; i < sizeof(alur) / sizeof(alur[0]); ++i) {
      if (strcmp(opcode, alur[i].name) == 0) {
        r = reg_code(operands[0]);
        if (r < 0) {
          set_error(error, state->line_number, "invalid ALU register");
          return 0;
        }
        if (pass == 2) {
          write_byte(image, pc, (uint8_t)(alur[i].opcode | r),
                     state->line_number);
        }
        *next_pc = (uint16_t)(pc + 1);
        return 1;
      }
    }

    for (i = 0; i < sizeof(alui) / sizeof(alui[0]); ++i) {
      if (strcmp(opcode, alui[i].name) == 0) {
        if (!eval_expr(state, operands[0], pass, &value, error)) {
          return 0;
        }
        if (pass == 2) {
          write_byte(image, pc, alui[i].opcode, state->line_number);
          write_byte(image, (uint16_t)(pc + 1), (uint8_t)value,
                     state->line_number);
        }
        *next_pc = (uint16_t)(pc + 2);
        return 1;
      }
    }
  }

  set_error(error, state->line_number, "unsupported opcode");
  return 0;
}

static int parse_statement(char *source, char *label, size_t label_size,
                           char *opcode, size_t opcode_size, char *operand_text,
                           size_t operand_size) {
  char first[64];
  char upper_first[64];
  char *cursor = source;
  char *space = NULL;

  label[0] = '\0';
  opcode[0] = '\0';
  operand_text[0] = '\0';

  trim(cursor);
  if (*cursor == '\0') {
    return 0;
  }

  if (strcmp(cursor, "*") == 0) {
    return 0;
  }

  space = strpbrk(cursor, " \t");
  if (space == NULL) {
    uppercase_copy(opcode, opcode_size, cursor);
    return 1;
  }

  memcpy(first, cursor, (size_t)(space - cursor));
  first[space - cursor] = '\0';
  trim(first);

  if (first[strlen(first) - 1] == ':') {
    first[strlen(first) - 1] = '\0';
    uppercase_copy(label, label_size, first);
    cursor = space + 1;
    trim(cursor);
    space = strpbrk(cursor, " \t");
    if (space == NULL) {
      uppercase_copy(opcode, opcode_size, cursor);
      return 1;
    }
    memcpy(first, cursor, (size_t)(space - cursor));
    first[space - cursor] = '\0';
    uppercase_copy(opcode, opcode_size, first);
    cursor = space + 1;
    trim(cursor);
    snprintf(operand_text, operand_size, "%s", cursor);
    return 1;
  }

  uppercase_copy(upper_first, sizeof(upper_first), first);
  if (instruction_size(upper_first, 1) != 1 || strcmp(upper_first, "DB") == 0 ||
      strcmp(upper_first, "DW") == 0 || strcmp(upper_first, "DS") == 0 ||
      strcmp(upper_first, "ORG") == 0 || strcmp(upper_first, "EQU") == 0 ||
      strcmp(upper_first, "JMP") == 0 || strcmp(upper_first, "CALL") == 0 ||
      strcmp(upper_first, "MOV") == 0 || strcmp(upper_first, "MVI") == 0 ||
      strcmp(upper_first, "LXI") == 0 || strcmp(upper_first, "PUSH") == 0 ||
      strcmp(upper_first, "POP") == 0 || strcmp(upper_first, "INX") == 0 ||
      strcmp(upper_first, "DCX") == 0 || strcmp(upper_first, "INR") == 0 ||
      strcmp(upper_first, "DCR") == 0 || strcmp(upper_first, "CMP") == 0 ||
      strcmp(upper_first, "CPI") == 0 || strcmp(upper_first, "ANI") == 0 ||
      strcmp(upper_first, "ORI") == 0 || strcmp(upper_first, "ANA") == 0 ||
      strcmp(upper_first, "ORA") == 0 || strcmp(upper_first, "XRA") == 0 ||
      strcmp(upper_first, "SUB") == 0 || strcmp(upper_first, "ADI") == 0 ||
      strcmp(upper_first, "STA") == 0 || strcmp(upper_first, "LDA") == 0 ||
      strcmp(upper_first, "LHLD") == 0 || strcmp(upper_first, "SHLD") == 0 ||
      strcmp(upper_first, "LDAX") == 0 || strcmp(upper_first, "STAX") == 0 ||
      strcmp(upper_first, "DAD") == 0 || strcmp(upper_first, "JNZ") == 0 ||
      strcmp(upper_first, "JZ") == 0 || strcmp(upper_first, "JC") == 0 ||
      strcmp(upper_first, "JNC") == 0 || strcmp(upper_first, "JP") == 0 ||
      strcmp(upper_first, "JM") == 0 || strcmp(upper_first, "CNZ") == 0 ||
      strcmp(upper_first, "CZ") == 0 || strcmp(upper_first, "CNC") == 0 ||
      strcmp(upper_first, "CC") == 0 || strcmp(upper_first, "CP") == 0 ||
      strcmp(upper_first, "CM") == 0 || strcmp(upper_first, "RET") == 0 ||
      strcmp(upper_first, "RNZ") == 0 || strcmp(upper_first, "RZ") == 0 ||
      strcmp(upper_first, "RNC") == 0 || strcmp(upper_first, "RC") == 0 ||
      strcmp(upper_first, "RP") == 0 || strcmp(upper_first, "RM") == 0 ||
      strcmp(upper_first, "XCHG") == 0 || strcmp(upper_first, "XTHL") == 0 ||
      strcmp(upper_first, "SPHL") == 0 || strcmp(upper_first, "HLT") == 0 ||
      strcmp(upper_first, "NOP") == 0 || strcmp(upper_first, "STC") == 0) {
    uppercase_copy(opcode, opcode_size, upper_first);
    cursor = space + 1;
    trim(cursor);
    snprintf(operand_text, operand_size, "%s", cursor);
    return 1;
  }

  uppercase_copy(label, label_size, first);
  cursor = space + 1;
  trim(cursor);
  space = strpbrk(cursor, " \t");
  if (space == NULL) {
    uppercase_copy(opcode, opcode_size, cursor);
    return 1;
  }
  memcpy(first, cursor, (size_t)(space - cursor));
  first[space - cursor] = '\0';
  uppercase_copy(opcode, opcode_size, first);
  cursor = space + 1;
  trim(cursor);
  snprintf(operand_text, operand_size, "%s", cursor);
  return 1;
}

static int assemble_text_internal(assembler *state, i8080_image *image,
                                  i8080_asm_error *error) {
  int pass;

  for (pass = 1; pass <= 2; ++pass) {
    char *content = NULL;
    char *cursor = NULL;
    char *line = NULL;
    uint16_t pc = 0;

    content = dup_text(state->text);
    if (content == NULL) {
      set_error(error, 0, "out of memory");
      return 0;
    }

    if (pass == 2) {
      i8080_image_init(image);
    }

    cursor = content;
    while ((line = next_line(&cursor)) != NULL) {
      char raw[256];
      char label[64];
      char opcode[64];
      char operand_text[256];
      char operands[8][64];
      int operand_count = 0;
      int size = 0;

      state->line_number += 1;
      snprintf(raw, sizeof(raw), "%s", line);
      trim(raw);
      {
        char *comment = strpbrk(raw, "#;");
        if (comment != NULL) {
          *comment = '\0';
        }
      }
      trim(raw);
      if (!parse_statement(raw, label, sizeof(label), opcode, sizeof(opcode),
                           operand_text, sizeof(operand_text))) {
        continue;
      }
      operand_count = parse_operands(operand_text, operands, 8);

      if (label[0] != '\0' && strcmp(opcode, "EQU") != 0 && pass == 1) {
        if (!define_symbol(state, label, pc, error)) {
          free(content);
          return 0;
        }
      }
      if (strcmp(opcode, "EQU") == 0) {
        int value;
        if (label[0] == '\0') {
          set_error(error, state->line_number, "EQU needs a label");
          free(content);
          return 0;
        }
        if (!eval_expr(state, operands[0], pass, &value, error)) {
          free(content);
          return 0;
        }
        if (pass == 1) {
          if (!define_symbol(state, label, (uint16_t)value, error)) {
            free(content);
            return 0;
          }
        }
        continue;
      }

      size = instruction_size(opcode, operand_count);
      if (size == -2) {
        int value;
        if (!eval_expr(state, operands[0], pass, &value, error)) {
          free(content);
          return 0;
        }
        size = value;
      }
      if (pass == 1 && strcmp(opcode, "ORG") != 0) {
        pc = (uint16_t)(pc + size);
        continue;
      }
      if (!encode_instruction(state, opcode, operands, operand_count, pass, pc,
                              image, &pc, error)) {
        free(content);
        return 0;
      }
    }

    free(content);
    state->line_number = 0;
  }

  return 1;
}

static int assemble_listing(const char *text, i8080_image *image,
                            i8080_asm_error *error) {
  char *content = dup_text(text);
  char *cursor = NULL;
  char *line = NULL;
  size_t line_number = 0;

  if (content == NULL) {
    set_error(error, 0, "out of memory");
    return 0;
  }

  i8080_image_init(image);

  cursor = content;
  while ((line = next_line(&cursor)) != NULL) {
    i8080_listing_record record;

    line_number += 1;
    if (!i8080_parse_listing_record(line, &record)) {
      continue;
    }

    if (record.byte_count != 0) {
      size_t i;
      for (i = 0; i < record.byte_count; ++i) {
        write_byte(image, (uint16_t)(record.address + i), record.bytes[i],
                   line_number);
      }
      continue;
    }

    if (record.source[0] != '\0') {
      char label[64];
      char opcode[64];
      char operand_text[256];
      char operands[8][64];

      if (!parse_statement(record.source, label, sizeof(label), opcode,
                           sizeof(opcode),
                           operand_text, sizeof(operand_text))) {
        continue;
      }
      parse_operands(operand_text, operands, 8);
      if (strcmp(opcode, "DS") == 0) {
        int value = 0;
        assembler state = {.name = "listing", .text = text, .line_number = line_number};
        if (!eval_expr(&state, operands[0], 1, &value, error)) {
          free(content);
          return 0;
        }
        while (value-- > 0) {
          write_byte(image, record.address++, 0x00, line_number);
        }
      }
    }
  }

  free(content);
  return 1;
}

static int read_file(const char *path, char **text, i8080_asm_error *error) {
  FILE *fp = fopen(path, "rb");
  long size;
  size_t read_count;

  if (fp == NULL) {
    set_error(error, 0, "could not open input file");
    return 0;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    set_error(error, 0, "could not seek input file");
    return 0;
  }
  size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    set_error(error, 0, "could not size input file");
    return 0;
  }
  if (fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    set_error(error, 0, "could not rewind input file");
    return 0;
  }

  *text = malloc((size_t)size + 1);
  if (*text == NULL) {
    fclose(fp);
    set_error(error, 0, "out of memory");
    return 0;
  }
  read_count = fread(*text, 1, (size_t)size, fp);
  fclose(fp);
  (*text)[read_count] = '\0';
  return 1;
}

int i8080_assemble_text(const char *name, const char *text, i8080_image *image,
                        i8080_asm_error *error) {
  assembler state;
  memset(&state, 0, sizeof(state));
  state.name = name;
  state.text = text;
  if (i8080_listing_looks_like_listing(text)) {
    return assemble_listing(text, image, error);
  }
  return assemble_text_internal(&state, image, error);
}

int i8080_assemble_file(const char *path, i8080_image *image,
                        i8080_asm_error *error) {
  char *text = NULL;
  int ok;

  if (!read_file(path, &text, error)) {
    return 0;
  }
  ok = i8080_assemble_text(path, text, image, error);
  free(text);
  return ok;
}

int i8080_write_binary(const char *path, const i8080_image *image,
                       i8080_asm_error *error) {
  FILE *fp = fopen(path, "wb");
  size_t expected = i8080_image_size(image);
  size_t written;

  if (fp == NULL) {
    set_error(error, 0, "could not open output file");
    return 0;
  }
  written = fwrite(image->bytes + image->origin, 1, expected, fp);
  fclose(fp);
  if (written != expected) {
    set_error(error, 0, "short write");
    return 0;
  }
  return 1;
}
