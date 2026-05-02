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
