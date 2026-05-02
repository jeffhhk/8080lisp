# Reference

This manual reflects the behavior validated by the current `make test` fixture.

## Boot Memory Layout

- `lisp_boot` clears the 64 KiB emulated Lisp memory image.
- `CVTFQ` at `0x04ae` is initialized to `0x0700`.
- The first free queue entry at `0x0700` is initialized to `ffff:0040`.
- `EVQAL` at `0x04b4` is initialized to `0x8000` for `NIL`.

## Monitor I/O Shims

- `lisp_inch` returns the next input byte and maps end of input to `0x00`.
- `lisp_outc` emits one byte through the host output hook.
- `lisp_crlf` emits a line feed byte.

## Assembler

- `o/i8080asm` can ingest the OCR listing in `orig/lisp_8080_rawocr_2026-04-13.asm`
  by reconstructing the binary from the listing's address and object-byte columns.
- The same assembler also accepts a small clean-source subset used by the local
  tests: `ORG`, `EQU`, `DB`, `DW`, `DS`, labels, and the 8080 opcodes exercised
  by the new fixture.

## OCR Validator

- `o/ocr_validator` compares the raw OCR listing, the corrected listing, and the
  provenance ledger at field granularity.
- The validator recognizes discrepancies in `bytes`, `label`, `mnemonic`,
  `operand`, `comment`, and `whole_line`.
- Validation fails if a raw-vs-corrected discrepancy is missing from
  `docs/OCR_CORRECTIONS.yaml`.
- Validation also fails if a provenance entry does not match any current
  discrepancy.

## Emulator

- `o/i8080emu` emulates the 8080 instruction subset exercised by the current
  Lisp image and the local hook fixture.
- `CALL 0xF006`, `CALL 0xF009`, `CALL 0xF021`, and `CALL 0xF000` are patched to
  host `INCH`, `OUTC`, `CRLF`, and `ABEND` behaviors.
- When `o/i8080emu` is invoked as `o/i8080emu --coverage program.asm`, it emits
  an instruction-pointer coverage report to stderr after execution.
- Each nonzero coverage line has the form `0xADDR COUNT`, where `COUNT` is how
  many times that address became the instruction pointer for an opcode fetch.
- The report ends with `covered_addresses` and
  `total_instruction_fetches` summary lines.
- The current automated coverage validates that the original OCR image boots to
  its monitor loop at `0x0599` without unsupported-opcode faults.
