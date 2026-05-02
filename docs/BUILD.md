# Build Commands

- `make clean` removes build output under `o/`
- `make build` builds the default local `gcc` target
- `make test` builds and runs the default local `gcc` target
- `make lisp-cosmo` builds the separate `cosmocc` target

The translated Lisp runtime lives under `src/`. C-based host shims, the local
entrypoint, and the automated fixture live under `tests/`.
