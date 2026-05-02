# Implementation Backlog

- [x] `PORT-0001R`: Align the repository layout with the current plan by
  keeping assembler sources in `src/`, moving C fixtures into `tests/`, and
  synthesizing `docs/Reference.md` from validated behavior.
- [x] `PORT-0001`: Add a GAS runtime scaffold that preserves the original 8080
  register map, boot memory layout, and monitor I/O entry points for the local
  `gcc` validation path.
- [ ] `PORT-0002`: Translate the pair, atom, and allocator core from
  `orig/lisp_8080_rawocr_2026-04-13.asm` onto the scaffolded memory image.
- [ ] `PORT-0003`: Translate the reader character helpers, atom capture, and
  atom interning routines from `orig/lisp_8080_rawocr_2026-04-13.asm`.
- [ ] `PORT-0004`: Translate the recursive list reader from
  `orig/lisp_8080_rawocr_2026-04-13.asm`.
- [ ] `PORT-0005`: Translate the evaluator, apply path, and definition helpers
  from `orig/lisp_8080_rawocr_2026-04-13.asm`.
- [ ] `PORT-0006`: Translate the REPL loop and extend the fixture so every Lisp
  primitive is exercised end-to-end.
