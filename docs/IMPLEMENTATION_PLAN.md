# Implementation Plan

## Product Goal

Port the 8080 lisp implementation to a cosmopolitan binary.

## Official Build Commands

- `make clean`
- `make build` for the local `gcc` validation target
- `make test` for the local `gcc` validation target
- `make hello-cosmo` for the separate `cosmocc` target

## Implementation rules

- Keep the local validation path separate from the `cosmocc` build path
- Use `gcc` for the default local validation target
- Use GNU assembler via `~/bin/cosmo/bin/cosmocc` for the cosmopolitan target
- .intel_syntax noprefix
- Implement lisp completely in gas except as otherwise required by cosmo
- Test fixture must test every LISP primitive at least once
- Test fixtures are free to use C

### Register equivalence

Use the following register map to translate from 8080 to 8086 registers.

| **8080 (8-bit)** | **8080 Pair / Role** | **8086 (16-bit)** | **Notes**                              |
| ---------------- | -------------------- | ----------------- | -------------------------------------- |
| A                | Accumulator          | AL → AX           | Direct match (low byte of AX)          |
| B                | BC (high)            | BH → BX           | Forms BX with C                        |
| C                | BC (low)             | BL → BX           |                                        |
| D                | DE (high)            | DH → DX           | Forms DX with E                        |
| E                | DE (low)             | DL → DX           |                                        |
| H                | HL (high)            | (no exact)        | Often maps to BX/SI depending on usage |
| L                | HL (low)             | (no exact)        | Same as above                          |
| BC               | Register pair        | BX                | General-purpose                        |
| DE               | Register pair        | DX                | Often used for I/O                     |
| HL               | Memory pointer       | BX / SI / DI      | Depends on addressing mode             |
| SP               | Stack Pointer        | SP                | Same role, wider                       |
| PC               | Program Counter      | IP                | Same concept                           |
| F                | Flags                | FLAGS             | 8086 has more flags                    |

## Change constraints

- Before commit, validate the size of the program with "size --format=GNU --radix=10", and record the result in DEVELOPMENT_LOG.yaml as a verification.

## Testing Strategy

The normative policy for this repository is:

- `make test` is the primary automated verification gate
- when a test is significantly changed, follow the Test Double Check Procedure end-to-end before final delivery
