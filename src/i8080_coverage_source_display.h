#ifndef I8080_COVERAGE_SOURCE_DISPLAY_H
#define I8080_COVERAGE_SOURCE_DISPLAY_H

#include "i8080_asm.h"
#include "i8080_emu.h"

#include <stdio.h>

int i8080_write_coverage_source_display(FILE *stream, const char *path,
                                        const i8080_image *image,
                                        const i8080_coverage *coverage,
                                        i8080_asm_error *error);

#endif
