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

## Lisp Interpreter

- The corrected OCR listing boots to the monitor loop at `0x0599` without
  `ABEND` or unsupported-opcode faults.
- The core list routines preserve the original pointer model:
  `CONS` allocates cons cells, `CAR` and `CDR` return the stored links,
  and `ATOM`, `ISLIST`, and `NULL` report their results through the `Z` flag.
- The corrected OCR listing aligns the opening dispatch block so the validated
  direct routine entries are `CAR` at `0x0009`, `CDR` at `0x0010`, and `CONS`
  at `0x0016`.
- `OUTPUT` renders `NIL` as `NIL`, proper lists as space-separated forms such as
  `(T F)`, and dotted pairs as forms such as `(T.F)`.
- `EQ` compares atom pointers, and `EQUAL` recursively compares tree structure.
- `PAIRLIS` zips two lists into an association list, and `ASSOC` returns the
  matching binding pair for both head and non-head lookups on that alist.
- `EVAL` validates the atomic environment lookup path, the `QUOTE` special form,
  the `COND` special form, and ordinary application through `CAR` on a quoted
  pair.
- `APPLY` validates both the `LAMBDA` and `LABEL` function-object paths.
- The definition-list routine at `0x05ff`, which underpins `DEFINE`, prepends a
  new `(name value)` binding to `EVQAL` and returns the list of defined names.
- The interpreter tests collect instruction-pointer coverage and assert hits on
  the exercised Lisp entry points while running the corrected OCR listing.

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
- Address-only line moves do not require provenance entries; a discrepancy is
  recorded only when the machine code or assembly text for the line changes.
- A `bytes` provenance entry may cover a shifted byte-column run by adding
  `line_number_end` and `address_end`, which lets one ledger item account for
  the concatenated byte discrepancies across that block.
- Provenance `basis` values are restricted to `instruction-encoding`,
  `cross-reference`, `control-flow`, `duplicate-pattern`, `runtime-behavior`,
  and `scan-review`.
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
- When `o/i8080emu` is invoked as
  `o/i8080emu --coverage-out coverage.ndjson program.asm`, it writes the
  coverage report to an NDJSON file.
- When `o/i8080emu` is invoked as
  `o/i8080emu --coverage-source-display-on-exit program.asm`, it prints a
  newline-prefixed `source coverage:` report to stdout after execution.
- When `--coverage-baseline coverage.ndjson` is also provided, the emulator
  subtracts the baseline address hits from the completed run before rendering
  source coverage, and only positive residual coverage is displayed.
- Each nonzero coverage line has the form `0xADDR COUNT`, where `COUNT` is how
  many times that address became the instruction pointer for an opcode fetch.
- Each NDJSON address record includes `kind`, `address`, `address_hex`, and
  `hits`, and the file ends with a `summary` record containing
  `covered_addresses` and `total_instruction_fetches`.
- Each source-coverage line includes the source line number, the aggregated hit
  count for that line, and the original source text; `...` compresses uncovered
  spans between covered lines.
- The report ends with `covered_addresses` and
  `total_instruction_fetches` summary lines.
- The current automated coverage validates that the original OCR image boots to
  its monitor loop at `0x0599` without unsupported-opcode faults.

### Emulator examples

    printf 'CAR ((T.F)) \n' | ./o/i8080emu src/lisp_8080_corrected.asm
    printf '(LAMBDA (X) X) (3) \n' | ./o/i8080emu src/lisp_8080_corrected.asm
    printf 'NULL (NIL) \nNULL ((NIL)) \n' | ./o/i8080emu src/lisp_8080_corrected.asm
    printf 'NULL (NIL) \n' | ./o/i8080emu --coverage-source-display-on-exit src/lisp_8080_corrected.asm
    printf 'NULL (NIL) \n' | ./o/i8080emu --coverage-baseline baseline.ndjson --coverage-source-display-on-exit src/lisp_8080_corrected.asm
