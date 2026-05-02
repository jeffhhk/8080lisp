# Synopsis

This manual summarizes the 8080 Lisp interpreter, emulator, assembler, and OCR
validation workflow that the current repository validates through `make test`
plus the recorded manual CLI checks in `docs/DEVELOPMENT_LOG.yaml`.

## Lisp Interpreter

A brief description of the confirmed capabilities of the 8080 Lisp interpreter
and the 8080 emulator follows.

### Boot state

- The corrected OCR listing assembles and boots to the monitor loop at `0x0599`
  without `ABEND` or unsupported-opcode faults.
- `lisp_boot` clears the 64 KiB emulated Lisp memory image.
- `CVTFQ` at `0x04ae` is initialized to `0x0700`.
- The first free-queue entry at `0x0700` is initialized to `ffff:0040`.
- `EVQAL` at `0x04b4` is initialized to `0x8000` for `NIL`.

### Atoms, pairs, and simple predicates

- The validated direct routine entries are `CAR` at `0x0009`, `CDR` at
  `0x0010`, and `CONS` at `0x0016`.
- `CONS` allocates cons cells, and `CAR` and `CDR` return the stored links.
- `ATOM`, `ISLIST`, and `NULL` report their result through the `Z` flag.
- The monitor-level `NULL` queries currently validated by CLI checks are:
  `NULL (NIL)` -> `T` and `NULL ((NIL))` -> `F`.

### Equality and printed form

- `EQ` compares atom pointers.
- `EQUAL` recursively compares tree structure.
- `OUTPUT` renders `NIL` as `NIL`, proper lists as space-separated forms such as
  `(T F)`, and dotted pairs as forms such as `(T.F)`.

### Environment construction and lookup

- `PAIRLIS` zips a names list and a values list into an association list.
- `ASSOC` returns the matching binding pair for both head and non-head lookups
  on a `PAIRLIS`-constructed alist.
- The definition-list routine at `0x05ff`, which underpins `DEFINE`, prepends a
  new `(name value)` binding to `EVQAL` and returns the list of defined names.

### Evaluation and application

- `EVAL` validates the atomic environment lookup path.
- `EVAL` validates the `QUOTE` special form.
- `EVAL` validates the `COND` special form.
- `EVAL` validates ordinary application through `CAR` on a quoted pair.
- `APPLY` validates both the `LAMBDA` and `LABEL` function-object paths.

## Emulator

### Supported instructions

The emulator executes the 8080 instruction subset required by the corrected Lisp
image and the local fixtures. Unsupported opcodes halt execution, set the CPU
error flag, and report the opcode and address on the CLI.

| Mnemonic family | Forms supported by the emulator | Notes |
| --- | --- | --- |
| Data movement | `NOP`, `MOV`, `MVI`, `LXI`, `LDAX`, `STAX`, `LDA`, `STA`, `LHLD`, `SHLD` | `MOV`/`MVI` include register and `M` forms. `LDAX`/`STAX` use `B` and `D` pairs. |
| Register-pair and stack | `INX`, `DCX`, `DAD`, `PUSH`, `POP`, `XCHG`, `XTHL`, `SPHL` | `PUSH`/`POP` include `PSW`. |
| Arithmetic and logic | `INR`, `DCR`, `ADD`, `ADC`, `SUB`, `SBB`, `ANA`, `XRA`, `ORA`, `CMP`, `ADI`, `ANI`, `ORI`, `CPI`, `STC` | Register ALU forms include `M`. Immediate support is limited to the four listed opcodes. |
| Control flow | `JMP`, `JNZ`, `JZ`, `JNC`, `JC`, `JP`, `CALL`, `RET`, `RNZ`, `RZ`, `RC`, `HLT` | Conditional call/jump/return coverage is limited to the listed conditions. |
| Monitor hooks | `CALL 0xF000`, `CALL 0xF006`, `CALL 0xF009`, `CALL 0xF021` | Patched to host `ABEND`, `INCH`, `OUTC`, and `CRLF`. |

### Usage

#### Name

`i8080emu` - assemble and run an 8080 source file with host-backed monitor
hooks and optional coverage reporting

#### Synopsis

`./o/i8080emu [--coverage] [--coverage-out path] [--coverage-source-display-on-exit] [--coverage-baseline path] [--coverage-baseline-boolean path] program.asm`

#### Description

- The CLI assembles `program.asm`, loads the image into the emulator, and runs
  it with host monitor hooks for `INCH`, `OUTC`, `CRLF`, and `ABEND`.
- EOF on stdin halts the run cleanly after the current `INCH` request.
- A CPU step-limit failure reports `step limit reached after N steps`.
- An unsupported opcode reports `unsupported opcode 0xXX at 0xYYYY`.
- `make i8080web` optionally emits `o/i8080web.html`, a single-file browser
  page that bundles the compiled emulator and the corrected Lisp listing behind
  textarea-based monitor input and output.

#### Options

- `--coverage` writes an instruction-pointer coverage report to stderr after the
  run. Each covered address is emitted as `0xADDR COUNT`, followed by
  `covered_addresses` and `total_instruction_fetches`.
- `--coverage-out path` writes the same run's coverage to an NDJSON file. Each
  covered address becomes one `{"kind":"address",...}` record, and the file
  ends with one `{"kind":"summary",...}` record.
- `--coverage-source-display-on-exit` prints a newline-prefixed
  `source coverage:` section to stdout after program output. The display shows
  only covered source lines with aggregated hit counts followed by the original
  source text, and uses `...` to compress uncovered gaps.
- `--coverage-baseline path` reads an NDJSON baseline and subtracts its
  per-address hits before rendering `--coverage-source-display-on-exit`.
- `--coverage-baseline-boolean path` reads an NDJSON baseline and hides every
  source line touched by the baseline while preserving the current run's counts
  on the remaining displayed lines.

#### Examples

- `printf '(LAMBDA (X) X) (3) \n' | ./o/i8080emu src/lisp_8080_corrected.asm`
- `printf 'NULL (NIL) \n' | ./o/i8080emu --coverage-source-display-on-exit src/lisp_8080_corrected.asm`

## Assembler

The assembler accepts the validated OCR listing format and a clean-source subset
used by the tests. Its accepted source language is broader than the emulator's
executed subset.

Supported directives:

- `ORG`
- `EQU`
- `DB`
- `DW`
- `DS`

Supported instruction families in assembled source:

- `NOP`, `HLT`
- `MOV`, `MVI`, `LXI`
- `INX`, `DCX`, `DAD`
- `PUSH`, `POP`
- `LDAX`, `STAX`
- `LDA`, `STA`, `LHLD`, `SHLD`
- `JMP`, `JNZ`, `JZ`, `JNC`, `JC`, `JP`, `JM`
- `CALL`, `CNZ`, `CZ`, `CNC`, `CC`, `CP`, `CM`
- `RET`, `RNZ`, `RZ`, `RNC`, `RC`, `RP`, `RM`
- `INR`, `DCR`
- `ADD`, `ADC`, `SUB`, `SBB`, `ANA`, `XRA`, `ORA`, `CMP`
- `ADI`, `ACI`, `SUI`, `SBI`, `ANI`, `XRI`, `ORI`, `CPI`
- `XCHG`, `XTHL`, `SPHL`, `STC`, `CMA`, `CMC`, `RLC`, `RRC`, `RAL`, `RAR`,
  `DAA`

Differences compared to the emulator:

- The assembler accepts `JM`, `CNZ`, `CZ`, `CNC`, `CC`, `CP`, `CM`, `RNC`,
  `RP`, `RM`, `ACI`, `SUI`, `SBI`, `XRI`, `CMA`, `CMC`, `RLC`, `RRC`, `RAL`,
  `RAR`, and `DAA`.
- The current emulator reference does not claim execution support for those
  opcodes because the current implementation and validation set do not cover
  them.

### Usage

#### Name

`i8080asm` - assemble an 8080 source file into a flat binary image

#### Synopsis

`./o/i8080asm input.asm output.bin`

#### Description

- The assembler can reconstruct a flat binary from the OCR listing in
  `orig/lisp_8080_rawocr_2026-04-13.asm`.
- The same tool also assembles the clean-source fixtures used by the local
  tests.
- Assembly errors report either `path:line: message` or a whole-file error when
  no source line applies.

## OCR Validator and validation process

Provenance of the maintained Lisp documents is:

- Raw OCR listing: `orig/lisp_8080_rawocr_2026-04-13.asm`
- Corrected working listing: `src/lisp_8080_corrected.asm`
- Provenance ledger: `docs/OCR_CORRECTIONS.yaml`

Validated OCR workflow:

- `./o/ocr_validator` validates the default raw listing, corrected listing, and
  provenance ledger.
- `./o/ocr_validator raw.asm corrected.asm corrections.yaml` validates an
  explicit triple of files.
- Validation compares raw and corrected listings at field granularity across
  `bytes`, `label`, `mnemonic`, `operand`, `comment`, and `whole_line`.
- Validation fails if any raw-vs-corrected discrepancy is missing from the
  ledger.
- Validation also fails if a ledger entry no longer matches a current
  discrepancy.
- Accepted provenance `basis` values are `instruction-encoding`,
  `cross-reference`, `control-flow`, `duplicate-pattern`, `runtime-behavior`,
  and `scan-review`.
- A `bytes` ledger entry may cover a shifted byte-column run by supplying
  `line_number_end` and `address_end`.

Make-related coverage workflow:

- `make build` rebuilds `o/i8080emu` and `o/ocr_validator`, which are the tools
  used to collect runtime coverage and validate OCR provenance.
- `make test` reruns the emulator, interpreter, assembler, and OCR-validator
  suites. The interpreter suite collects instruction-pointer coverage in memory
  and asserts hits on the exercised Lisp entry points.
- The repository does not currently have a make target that writes a persistent
  coverage report artifact or regenerates `docs/Reference.md`.
- Coverage files such as NDJSON baselines are updated only when
  `./o/i8080emu --coverage-out ...` is run explicitly; they are transient
  manual-analysis artifacts rather than tracked build outputs.
