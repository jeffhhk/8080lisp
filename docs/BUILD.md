# Build Commands

- `make clean` removes build output under `o/`
- `make build` builds the default local `gcc` target plus the local `i8080asm`, `i8080emu`, and `ocr_validator` tools
- `make test` builds and runs the default local `gcc` target
- `make lisp-cosmo` builds the separate `cosmocc` target

The translated Lisp runtime lives under `src/`. C-based host shims, the local
entrypoint, and the automated fixture live under `tests/`.

The assembler binary is written to `o/i8080asm` and emits a flat binary from
either clean 8080 source or the OCR listing format used by
`orig/lisp_8080_rawocr_2026-04-13.asm`.

The emulator binary is written to `o/i8080emu` and assembles then runs an 8080
program with host-backed `INCH`, `OUTC`, `CRLF`, and `ABEND` monitor hooks.
Pass `--coverage` before the assembly path to emit an instruction-pointer
coverage report to stderr after execution. The report lists each executed
instruction address with its hit count, followed by summary totals.

The OCR validator binary is written to `o/ocr_validator`. With no arguments it
validates `orig/lisp_8080_rawocr_2026-04-13.asm`,
`src/lisp_8080_corrected.asm`, and `docs/OCR_CORRECTIONS.yaml`. It also
accepts explicit `raw.asm corrected.asm corrections.yaml` paths.
