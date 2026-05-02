# Build Commands

- `make clean` removes build output under `o/`
- `make build` builds the default local `gcc` target plus the local `i8080asm` and `i8080emu` tools
- `make test` builds and runs the default local `gcc` target
- `make lisp-cosmo` builds the separate `cosmocc` target

The translated Lisp runtime lives under `src/`. C-based host shims, the local
entrypoint, and the automated fixture live under `tests/`.

The assembler binary is written to `o/i8080asm` and emits a flat binary from
either clean 8080 source or the OCR listing format used by
`orig/lisp_8080_rawocr_2026-04-13.asm`.

The emulator binary is written to `o/i8080emu` and assembles then runs an 8080
program with host-backed `INCH`, `OUTC`, `CRLF`, and `ABEND` monitor hooks.
