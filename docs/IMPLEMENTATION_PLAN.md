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

### Register Mapping

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

### Memory mapping

Since addresses in the original app are based on 16 bits, but we are running in a 64 bit process, use the following memory mapping:
    - The host allocates one native object, lisp_memory, of size 65536 bytes.
    - Code keeps its base address in r15 while running. Any Lisp address x is interpreted as native
    address r15 + x.

### Instruction mapping

Make sure the translated file routines are listed in the same order as in the source file.

Mimic overlapping routines in the original code, example here CDR:
    0011 CADDR  CALL CDR
    0012 CADR   CALL CDR

Mimic intermediate entry points in the original code, example here CAR2:
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

- Preserve every original source comment during translation.
- If a translated instruction corresponds 1:1 with the source instruction, copy the comment onto that
translated instruction.
- If one source instruction expands into multiple translated instructions, preserve the original
comment on the first translated instruction of that block, or in an immediately preceding block
comment.
- If multiple adjacent source comments describe one idea, they may be merged into one nearby block
comment, but no original comment text may be dropped.
- Within each comment, translate register names according to Register Mapping.
- Comment preservation is mandatory completion criteria for each translated routine.

## Change constraints

- Before commit, validate the size of the program with "size --format=GNU --radix=10", and record the result in DEVELOPMENT_LOG.yaml as a verification.
- Update documentation according to Documentation Strategy

## Testing Strategy

The normative policy for this repository is:

- `make test` is the primary automated verification gate
- test every path in the original code unless it would involve adding parameters or entry points.
- when a test is significantly changed, follow the Test Double Check Procedure end-to-end before final delivery

## Documentation Strategy

Synthesize a reference manual docs/Reference.md from the behavior that you validate using your tests.
