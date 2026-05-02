# Synopsis

Reanimation of "A LISP Interpreter for the 8080", Darrel Van Buer, Dr. Dobb's Journal of Computer Calisthenics & Orthodontia, Vol 3 Number 30 (Nov 1978).

# Historical Background

The IBM PC would not come out for nearly 3 more years, (Aug 1981).  Because there was no standard keyboard/screen setup, the user had to supply 3 routines: INCH, OUTC, CRLF.

Typical setups of the time included second-hand ASR-33 teletypes (individuals couldn't just order them), VDM-1 style S-100 video cards, or some variation of the "TV Typewriter Cookbook".  Maxing out the 64k RAM address space was still expensive, roughly $1200 in 1978 (equivalent to $6000 today).

The port's target machine is Cosmopolitan binary, able to run on any 64-bit Intel Linux, Mac or Windows machine.

# Description

I took the 8080 LISP article, OCRed only the listing, and vibe coded an assembler and emulator, bug for bug with the original.  Examples below.

You can do things like this:

    $ printf 'NULL (NIL) \n' | ./o/i8080emulisp.com src/lisp_8080_corrected.asm

    >>T
    $ printf 'NULL ((NIL)) \n' | ./o/i8080emulisp.com src/lisp_8080_corrected.asm

    >>F

Running `./o/i8080emulisp.com src/lisp_8080_corrected.asm` gives a REPL, but note the eccentric syntax in the commands above.

There is also an optional browser target now: `make i8080web` produces
`o/i8080web.html`, a standalone page that bundles the compiled emulator and
`src/lisp_8080_corrected.asm` into one textarea-driven monitor UI.

I also implemented a coverage feature, so that using a REPL can help browse the working code.  Evaluating a very simple expression has lots of uninteresting coverage, like the reader, etc, so there's a mechanism for taking a baseline and then printing the residual.  Here's what happens if we ask if the list of NIL is NULL (false), and compare it what happens with doing the same for a list of two NILS:

    $ printf 'NULL ((NIL)) \n' | ./o/i8080emulisp.com  --coverage-out coverage-baseline.ndjson src/lisp_8080_corrected.asm

    >>F
    $ printf 'NULL ((NIL NIL)) \n' | ./o/i8080emulisp.com  --coverage-baseline-boolean coverage-baseline.ndjson --coverage-source-display-on-exit src/lisp_8080_corrected.asm

    >>F
    source coverage:
          1 | 037B CD A9 03       0501        CALL INPTL    GET REST OF LIST
          1 | 037E C3 90 03       0502        JMP  S135     THEN ')'
    ...
          2 | 03A9 CD DF 03       0520 INPTL  CALL CTRP
          2 | 03AC C2 B3 03       0521        JNZ  S144
          1 | 03AF 21 00 80       0522        LXI  H,8000H  NIL
          1 | 03B2 C9             0523        RET
          1 | 03B3 D5             0524 S144   PUSH D
          1 | 03B4 CD 54 03       0525        CALL INPUT
          1 | 03B7 EB             0526        XCHG
          1 | 03B8 CD ED 03       0527        CALL TAKEBL
          1 | 03BB CD A9 03       0528        CALL INPTL
          1 | 03BE EB             0529        XCHG
          1 | 03BF CD 16 00       0530        CALL CONS
          1 | 03C2 D1             0531        POP  D
          1 | 03C3 C9             0532        RET

The leading column is the number of times that instruction ran.

All original assembler comments are preserved.  So, as the comments show, this is how we `"GET REST OF LIST"`.

The article exhibits a mixed source and machine code listing, with source code to the right and machine code to the left.   In the emulator, such an .asm file serves as both source and binary.

A provenance report reconciles each of 6 minor differences between the raw OCR listing and the one that runs.

It is preserved in a cosmopolitan binary, which will run on any Mac, Linux or Windows machine that supports Intel 64 bit executables.  Cosmopolitan binaries are also zip files.  Source code and artifacts (including the original PDF) are preserved in its zip structure.  To run the examples above, you have to get the lisp binary out of it first, as noted in the usage string:

    unzip o/i8080emulisp.com src/lisp_8080_corrected.asm

Artifacts include the original PDF, and the automatically gathered prompts used to create the project.
