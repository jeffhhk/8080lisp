# Implementation Plan

## Product Goal

Port the 8080 lisp implementation to a cosmopolitan binary.

## Official Build Commands

- `make clean`
- `make build` for the local `gcc` validation target
- `make test` for the local `gcc` validation target
- `make lisp-cosmo` for the separate `cosmocc` target

## Implementation rules

- Keep the local validation path separate from the `cosmocc` build path
- Use `gcc` for the default local validation target
- Use GNU assembler via `~/bin/cosmo/bin/cosmocc` for the cosmopolitan target
- .intel_syntax noprefix
- Implement lisp completely in gas except as otherwise required by cosmo
- Test fixture must test every LISP primitive at least once
- Test fixtures are free to use C

### Architecture mapping

Use the following register map to translate from 8080 to 8086 registers.

| **8080 (8-bit)** | **8080 Pair / Role** | **8086 (16-bit)** | **Notes**                              |
| ---------------- | -------------------- | ----------------- | -------------------------------------- |
| A                | Accumulator          | AL → AX           | Direct match (low byte of AX)          |
| B                | BC (high)            | BH → BX           | Forms BX with C                        |
| C                | BC (low)             | BL → BX           |                                        |
| D                | DE (high)            | DH → DX           | Forms DX with E                        |
| E                | DE (low)             | DL → DX           |                                        |
| H                | HL (high)            | (no exact)        | SI                                     |
| BC               | Register pair        | BX                | General-purpose                        |
| DE               | Register pair        | DX                | Often used for I/O                     |
| SP               | Stack Pointer        | SP                | Same role, wider                       |
| PC               | Program Counter      | IP                | Same concept                           |
| F                | Flags                | FLAGS             | 8086 has more flags                    |

### Instruction mapping

Make sure the translated file routines are listed in the same order as in the source file.

Take advantage of overlapping routines, e.g.:
    0000 C3 B6 04       0011 CADDR  CALL CDR
    0003 CD 10 00       0012 CADR   CALL CDR

Preserve label names when possible, e.g. CDR:
    000F C9             0020 CDR    PUSH PSW

Preserve comments whenever they are more than a copy of the instruction, e.g.:
    EQU 0F000H  MONITOR REENTRY
    . . .
    INX  H        SKIP CAR PTR
    . . .
    LXI  D,4      NEED 4 BYTES

## Change constraints

- Before commit, validate the size of the program with "size --format=GNU --radix=10", and record the result in DEVELOPMENT_LOG.yaml as a verification.

## Testing Strategy

The normative policy for this repository is:

- `make test` is the primary automated verification gate
- test every path in the original code unless it would involve adding parameters or entry points.
- when a test is significantly changed, follow the Test Double Check Procedure end-to-end before final delivery
