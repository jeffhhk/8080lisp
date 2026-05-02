# Implementation Backlog

- [x] `PORT-0001`: Add a GAS runtime scaffold that preserves the original 8080
  register map, boot memory layout, and monitor I/O entry points for the local
  `gcc` validation path.
- [ ] `PORT-0002`: Translate the pair, atom, and allocator core from
  `orig/lisp_8080_rawocr_2026-04-13.asm` onto the scaffolded memory image.
- [ ] `PORT-0003`: Translate the reader, evaluator, and REPL loop, then extend
  the fixture so every Lisp primitive is exercised end-to-end.
