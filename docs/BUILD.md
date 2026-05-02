# Build Commands

- `make clean` removes build output under `o/`
- `make build` builds the default local `gcc` target plus the local `i8080asm`, `i8080emu`, and `ocr_validator` tools
- `make test` builds and runs the default local `gcc` target
- `make build-cosmo` runs `make clean` and then rebuilds the shared `build` target with `CC=x86_64-unknown-cosmo-cc`, `BINEXT=.com`, and `PATH` augmented by `$(HOME)/bin/cosmo/bin`
- `make i8080web` optionally installs an embedded emsdk checkout and emits a standalone `o/i8080web.html` page for the corrected Lisp monitor
- pushing to `main` runs `.github/workflows/pages.yml`, which builds `o/i8080web.html`, stages it as `index.html`, and deploys it to GitHub Pages
- pushing to `main` also runs `.github/workflows/cosmo.yml`, which downloads `cosmocc-4.0.2.zip` into `$(HOME)/bin/cosmo`, runs `make build-cosmo`, and uploads `o/8080LISP.com` as a GitHub Actions artifact
- pushing a `v*` tag runs `.github/workflows/release.yml`, which installs `cosmocc`, runs `make build-cosmo`, creates or updates the matching GitHub Release, and uploads `o/8080LISP.com` as the release asset

The local `gcc` path keeps the existing Linux-native output names under `o/`.
The Cosmopolitan path emits cross-platform `.com` binaries under `o/`:

- `o/lisp_gcc.com`
- `o/i8080asm.com`
- `o/i8080emu.com`
- `o/ocr_validator.com`

The translated Lisp runtime lives under `src/`. C-based host shims, the local
entrypoint, and the automated fixture live under `tests/`.

The assembler binary is written to `o/i8080asm` for the local build and
`o/i8080asm.com` for `make build-cosmo`. It emits a flat binary from either
clean 8080 source or the OCR listing format used by
`orig/lisp_8080_rawocr_2026-04-13.asm`.

The emulator binary is written to `o/i8080emu` for the local build and
`o/i8080emu.com` for `make build-cosmo`. It assembles then runs an 8080 program
with host-backed `INCH`, `OUTC`, `CRLF`, and `ABEND` monitor hooks. The
Cosmopolitan build keeps the existing self-extracting archive behavior, so
`o/i8080emu.com` can still be unzipped to recover the bundled source and
documentation. Pass `--coverage` before the assembly path to emit an
instruction-pointer coverage report to stderr after execution. The report lists
each executed instruction address with its hit count, followed by summary
totals. Pass `--coverage-out path` before the assembly path to write the same
run's coverage to an NDJSON file. Each nonzero address is written as one
`address` record, and the file ends with one `summary` record. `--coverage` and
`--coverage-out` may be used together. Pass `--coverage-source-display-on-exit`
before the assembly path to print a newline-prefixed `source coverage:` report
to stdout after the program output; the report shows only covered source lines
with per-line hit totals followed by the original source text, and inserts
`...` between non-adjacent covered lines. Pass
`--coverage-baseline path` together with `--coverage-source-display-on-exit` to
subtract baseline per-address hits before the source lines are aggregated, so
only coverage in excess of the baseline is displayed. Pass
`--coverage-baseline-boolean path` together with
`--coverage-source-display-on-exit` to hide every source line touched by the
baseline while preserving the current run's original counts on the remaining
displayed lines.

The OCR validator binary is written to `o/ocr_validator` for the local build
and `o/ocr_validator.com` for `make build-cosmo`. With no arguments it
validates `orig/lisp_8080_rawocr_2026-04-13.asm`,
`src/lisp_8080_corrected.asm`, and `docs/OCR_CORRECTIONS.yaml`. It also accepts
explicit `raw.asm corrected.asm corrections.yaml` paths.

The optional web artifact is written to `o/i8080web.html`. It is built with
Emscripten as one self-contained HTML file, with the compiled emulator/runtime
payload and `src/lisp_8080_corrected.asm` bundled into the page. The page uses
textarea-based input and output so monitor expressions can be pasted into the
browser without shipping a separate assembler executable or auxiliary assets.

`ensure_emsdk_installed.sh [dir]` bootstraps an emsdk checkout, defaulting to
`toolchains/emsdk` under the repository root, and `ensure_emsdk_uninstalled.sh
[dir]` removes that checkout when it is no longer needed. `make i8080web`
respects `EMSDK_DIR` and `EMSDK_VERSION` overrides when invoking the install
script and `emcc`.

To activate the workflow-backed site, first set the repository's GitHub Pages
source to `GitHub Actions` in the repository Pages settings. Until that one-time
setting is enabled, `actions/configure-pages` fails with a `Get Pages site
failed` / `Not Found` error because the repository does not yet have a Pages
site configured. The workflow publishes the generated artifact from CI; it does
not require committing `o/i8080web.html`.

The Cosmopolitan artifact workflow uses the same default `COSMO_PATH` that the
Makefile expects locally, so the runner installs `cosmocc` under
`$HOME/bin/cosmo/bin` before invoking `make build-cosmo`. That workflow uploads
the packaged self-extracting archive from `o/8080LISP.com`; it does not commit
generated `o/` output.

The release workflow uses the same `make build-cosmo` path, but targets tagged
releases instead of workflow artifacts. Pushing a tag such as `v1.0.0` creates
or updates the GitHub Release for that tag and uploads `o/8080LISP.com` with
replacement semantics, so rerunning the workflow refreshes the attached asset
instead of failing on an existing upload. `workflow_dispatch` also accepts a
tag name for backfilling or republishing a release from the Actions UI.
