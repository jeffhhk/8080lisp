# Implementation Plan

## Product Goal

Make the 8080 lisp implementation run in a custom 8080 emulator with some instrumentation and visualization extras.

## Original Code

orig/lisp_8080_rawocr_2026-04-13.asm

## Official Build Commands

- `make clean`
- `make build` for the local `gcc` validation target
- `make test` for the local `gcc` validation target
- `make lisp-cosmo` for the separate `cosmocc` target (manual run only)
    - replaces `gcc` with `cosmocc` for the cosmopolitan target

## Implementation rules

- Keep the local validation path separate from the `gcc` build path
- Use `gcc` for the default local validation target
- Test fixture must test every LISP primitive at least once
- Test fixtures are free to use C

### Directories

src/ - ported program
tests/ - test code

### Assembler mapping

Make sure the translated file routines are listed in the same order as in the source file.

Preserve overlapping routines in the Original Code, example here CDR:
    0011 CADDR  CALL CDR
    0012 CADR   CALL CDR

Preserve intermediate entry points in the Original Code, example here CAR2:
    0013 CAR    PUSH PSW     SAVE A,F
    0014 CAR2   MOV  A,M     GET CAR/CDR PTR
    0015        INX  H
    0016        MOV  H,M     HL:=CAR(HL)
    0017        MOV  L,A
    0018        POP  PSW
    0019        RET
    0020 CDR    PUSH PSW
    0021        INX  H        SKIP CAR PTR
    0022        INX  H        FOR HL:=CDR(HL)
    0023        JMP  CAR2

Preserve label names when possible, e.g. CDR:
    000F C9             0020 CDR    PUSH PSW

### Comment preservation

Preserve all source comments in Original Code.
    TEST   CALL ATOM     LOOK FOR ATOMIC
should become:
    call ATOM          # LOOK FOR ATOMIC

- Preserve every Original Code comment during translation.
- If a translated instruction corresponds 1:1 with the source instruction, copy the comment onto that
translated instruction.
- If one source instruction expands into multiple translated instructions, preserve the Original Code 
comment on the first translated instruction of that block, or in an immediately preceding block
comment.
- If multiple adjacent source comments describe one idea, they may be merged into one nearby block
comment, but no Original Code comment text may be dropped.
- Within each comment, translate register names according to Register Mapping.
- Comment preservation is mandatory completion criteria for each translated routine.

## Change constraints

For each change, update documentation according to Documentation Strategy

## Testing Strategy

The normative policy for this repository is:

- `make test` is the primary automated verification gate
- test every path in the Original Code unless it would involve adding parameters or entry points.
- when a test is significantly changed, follow the Test Double Check Procedure end-to-end before final delivery

## Documentation Strategy

Synthesize a reference manual docs/Reference.md from the behavior that you validate using your tests.

For docs/Reference.md, use the following template, with style guidelines marked in <>:

    # Synopsis

    A brief description of all the confirmed capabiliites of the 8080 lisp interpreter and the 8080 emulator.

    ## Lisp Interpreter

    <Organize LISP forms and definitions by category primarily by increasing complexity, and secondarily by category.>

    ## Emulator

    ### Supported instructions

    <Format as a table>

    ### Usage

    <Format as a man page>

    ## Assembler

    <Note any differences in instruction support compared to the emulator>

    <Format as a man page>

    ## OCR Validator and validation process

    Provenance of the documents are as follows.  This is a RAW OCR of the original article listing:
        orig/lisp_8080_rawocr_2026-04-13.asm

    This file has minimal corrections required to run:
        src/lisp_8080_corrected.asm

    <Describe the makefile tasks related to the coverage report, and how it gets updated.>




