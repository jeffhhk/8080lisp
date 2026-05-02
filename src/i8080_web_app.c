#include "i8080_web_app.h"

#include "i8080_asm.h"
#include "i8080_emu.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include "lisp_8080_corrected_program.h"

enum { I8080WEB_OUTPUT_CAPACITY = 131072 };

typedef struct {
  i8080_cpu *cpu;
  const unsigned char *input;
  size_t input_pos;
  char output[I8080WEB_OUTPUT_CAPACITY];
  size_t output_len;
} i8080web_session;

typedef struct {
  i8080_asm_error error;
  i8080_image image;
  i8080_cpu cpu;
  i8080web_session session;
  int image_ready;
  int emulator_ready;
} i8080web_state;

/* Keep the large emulator state off the wasm stack so browser calls do not
 * fault on the much smaller default WebAssembly stack. */
static i8080web_state i8080web;

static void session_reset(i8080web_session *session, i8080_cpu *cpu,
                          const char *input) {
  session->cpu = cpu;
  session->input = (const unsigned char *)(input != NULL ? input : "");
  session->input_pos = 0;
  session->output_len = 0;
  session->output[0] = '\0';
}

static void output_append_text(i8080web_session *session, const char *text) {
  size_t available;
  size_t text_len;

  if (session->output_len + 1 >= sizeof(session->output)) {
    return;
  }

  available = sizeof(session->output) - session->output_len - 1;
  text_len = strlen(text);
  if (text_len > available) {
    text_len = available;
  }
  memcpy(session->output + session->output_len, text, text_len);
  session->output_len += text_len;
  session->output[session->output_len] = '\0';
}

static void output_append_format(i8080web_session *session, const char *format,
                                 ...) {
  va_list args;
  int written;

  if (session->output_len + 1 >= sizeof(session->output)) {
    return;
  }

  va_start(args, format);
  written = vsnprintf(session->output + session->output_len,
                      sizeof(session->output) - session->output_len, format,
                      args);
  va_end(args);

  if (written < 0) {
    session->output[session->output_len] = '\0';
    return;
  }

  if ((size_t)written >= sizeof(session->output) - session->output_len) {
    session->output_len = sizeof(session->output) - 1;
    session->output[session->output_len] = '\0';
    return;
  }

  session->output_len += (size_t)written;
}

static void output_append_byte(i8080web_session *session, uint8_t ch) {
  char text[5];

  if (ch == '\n' || ch == '\t') {
    text[0] = (char)ch;
    text[1] = '\0';
    output_append_text(session, text);
    return;
  }
  if (isprint(ch)) {
    text[0] = (char)ch;
    text[1] = '\0';
    output_append_text(session, text);
    return;
  }
  output_append_format(session, "\\x%02x", ch);
}

static int host_inch(void *ctx) {
  i8080web_session *session = ctx;
  int ch;

  if (session->input == NULL) {
    if (session->cpu != NULL) {
      session->cpu->halted = 1;
    }
    return 0;
  }

  ch = session->input[session->input_pos];
  if (ch == '\0') {
    if (session->cpu != NULL) {
      session->cpu->halted = 1;
    }
    return 0;
  }

  session->input_pos += 1;
  return ch;
}

static void host_outc(void *ctx, uint8_t ch) {
  i8080web_session *session = ctx;
  output_append_byte(session, ch);
}

static void host_abend(void *ctx, uint8_t code) {
  (void)ctx;
  (void)code;
}

static int ensure_image(void) {
  memset(&i8080web.error, 0, sizeof(i8080web.error));
  memset(&i8080web.image, 0, sizeof(i8080web.image));
  if (!i8080_assemble_text("src/lisp_8080_corrected.asm",
                           i8080web_embedded_program, &i8080web.image,
                           &i8080web.error)) {
    i8080web.image_ready = 0;
    i8080web.emulator_ready = 0;
    return 0;
  }
  i8080web.image_ready = 1;
  return 1;
}

static int restart_emulator(void) {
  i8080_hooks hooks = {
      .inch = host_inch,
      .outc = host_outc,
      .abend = host_abend,
      .ctx = &i8080web.session,
  };

  if (!i8080web.image_ready && !ensure_image()) {
    return 0;
  }

  i8080_init(&i8080web.cpu, &hooks);
  i8080_load_image(&i8080web.cpu, &i8080web.image);
  i8080web.emulator_ready = 1;
  i8080web.session.cpu = &i8080web.cpu;
  return 1;
}

EMSCRIPTEN_KEEPALIVE void i8080web_restart(void) {
  session_reset(&i8080web.session, &i8080web.cpu, "");
  restart_emulator();
}

EMSCRIPTEN_KEEPALIVE const char *i8080web_eval(const char *input) {
  i8080web_session *session = &i8080web.session;
  int run_result;

  session_reset(session, &i8080web.cpu, input);

  if (!i8080web.image_ready && !ensure_image()) {
    if (i8080web.error.line_number != 0) {
      output_append_format(session, "src/lisp_8080_corrected.asm:%zu: %s\n",
                           i8080web.error.line_number,
                           i8080web.error.message);
    } else {
      output_append_format(session, "%s\n", i8080web.error.message);
    }
    return session->output;
  }

  if (!i8080web.emulator_ready && !restart_emulator()) {
    if (i8080web.error.line_number != 0) {
      output_append_format(session, "src/lisp_8080_corrected.asm:%zu: %s\n",
                           i8080web.error.line_number,
                           i8080web.error.message);
    } else {
      output_append_format(session, "%s\n", i8080web.error.message);
    }
    return session->output;
  }

  i8080web.cpu.halted = 0;
  run_result = i8080_run(&i8080web.cpu, 10000000);

  if (run_result == I8080_STEP_LIMIT) {
    output_append_format(session, "\nstep limit reached after %zu steps\n",
                         i8080web.cpu.steps);
    return session->output;
  }
  if (i8080web.cpu.error) {
    output_append_format(session, "\nunsupported opcode 0x%02x at 0x%04x\n",
                         i8080web.cpu.error_opcode,
                         (uint16_t)(i8080web.cpu.pc - 1));
    return session->output;
  }
  if (i8080web.cpu.abended) {
    output_append_format(session, "\nABEND 0x%02x\n",
                         i8080web.cpu.abend_code);
  }
  return session->output;
}
