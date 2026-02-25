# MD4X

> **Always keep this file (`AGENTS.md`) and referenced `docs/*.md` files updated when making changes to the project.**
> **Always update `CHANGELOG.md` when adding removing user-facing features, APIs, build targets, CLI options, or library behavior.**

> C Markdown parser library (fork of [mity/md4c](https://github.com/mity/md4c))

- **Version:** 0.5.2 (next: 0.5.3 WIP)
- **License:** MIT
- **Language:** C (C89/C99 compatible)
- **Spec:** CommonMark 0.31.2
- **Build:** Zig (`zig build`)
- **JS Runtime:** Bun (do **not** use npm, pnpm, yarn, or npx — use `bun`/`bunx` exclusively)
- **Formatting:** Always run `bun fmt` after finishing code changes

## Project Structure

```
src/
  md4x.c              # Core parser (~6500 LoC)
  md4x.h              # Parser public API
  entity.c             # HTML entity lookup table (generated)
  entity.h             # Entity header
  md4x-wasm.c          # WASM exports (alloc/free + renderer wrappers)
  md4x-napi.c          # Node.js NAPI addon (module registration + renderer wrappers)
  renderers/
    md4x-props.h       # Shared component property parser (header-only)
    md4x-json.h        # Shared JSON writer + YAML-to-JSON helpers (header-only)
    md4x-html.c        # HTML renderer library (~500 LoC)
    md4x-html.h        # HTML renderer public API
    md4x-ast.c        # AST renderer library (~530 LoC)
    md4x-ast.h        # AST renderer public API
    md4x-ansi.c        # ANSI terminal renderer library (~450 LoC)
    md4x-ansi.h        # ANSI renderer public API
    md4x-meta.c        # Meta renderer library (~300 LoC)
    md4x-meta.h        # Meta renderer public API
    md4x-text.c        # Plain text renderer library (~350 LoC)
    md4x-text.h        # Plain text renderer public API
  cli/
    md4x-cli.c           # CLI utility (multi-format: html, text, json, ansi)
    cmdline.c            # Command-line parser (from c-reusables)
    cmdline.h            # Command-line parser API
    md4x.1               # Man page
packages/md4x/           # npm package
  package.json           # Package manifest (name: md4x)
  README.md              # Package README
  LICENSE.md             # MIT license
  build/
    md4x.wasm            # Prebuilt WASM binary
    md4x.*.node          # Prebuilt NAPI binaries (per-platform)
  lib/
    wasm.mjs             # JS entrypoint for WASM (async API, ESM)
    wasm.d.mts           # TypeScript declarations for WASM API
    napi.mjs             # JS entrypoint for NAPI (sync API, ESM)
    napi.d.mts           # TypeScript declarations for NAPI API
    types.d.ts           # Shared TypeScript types (ComarkTree, ComarkNode, ComarkElement, etc.)
  test/
    _suite.mjs           # Shared test suite (vitest, used by both NAPI and WASM tests)
    napi.test.mjs        # NAPI binding tests
    wasm.test.mjs        # WASM binding tests
  bench/
    _fixtures.mjs        # Benchmark fixture strings (small, medium, large)
    index.mjs            # Benchmark runner (mitata, compares napi/wasm/md4w/markdown-it)
test/
  spec.txt             # CommonMark 0.31.2 spec tests
  spec-*.txt           # Extension-specific tests (tables, strikethrough, frontmatter, etc.)
  regressions.txt      # Bug regression tests
  coverage.txt         # Code coverage tests
  run-testsuite.py     # Individual test suite runner
  pathological-tests.py # Stress tests for DoS resistance
  prog.py              # Program execution wrapper
  normalize.py         # HTML normalization for comparison
  fuzzers/             # LibFuzzer integration + seed corpus
scripts/
  run-tests.ts            # Main test runner (runs all suites)
  build-entity-map.ts     # Generates entity.c from WHATWG spec
  build-folding-map.ts    # Unicode case folding map generator
  build-punct-map.ts      # Punctuation character map generator
  build-whitespace-map.ts # Whitespace classification generator
  _unicode-map.ts         # Shared helper for punct/whitespace map generators
  coverity.sh             # Coverity Scan integration
  unicode/                # Unicode data files (CaseFolding.txt, DerivedGeneralCategory.txt)
website/                 # Docs + playground (Vite + Vue)
  package.json           # Website package manifest (md4x-demo)
  vite.config.mjs        # Vite config (base: /md4x/)
  index.html             # HTML entry
  index.ts               # App entry point
  index.css              # Styles
  App.vue                # Root component with navigation
  router.ts              # Vue Router config
  env.d.ts               # TypeScript env types
  pages/
    readme.vue           # Renders README.md via md4x/wasm
    playground.vue       # Interactive markdown editor + renderer
  samples/               # Example markdown files for playground
package.json             # Root workspace package (bun, workspaces: packages/*, website)
build.zig                # Zig build script
build.zig.zon            # Zig package manifest
.github/workflows/
  ci-build.yml         # Build + test (Linux/Windows, debug/release, coverage)
```

## Building

Uses Zig build system. External dependency: [libyaml](https://github.com/yaml/libyaml) 0.2.5 (YAML parser for AST/meta renderer frontmatter, fetched automatically via `build.zig.zon`).

```sh
zig build                          # build all (defaults to ReleaseFast)
zig build -Doptimize=Debug         # debug build
zig build run -- --help            # run md4x CLI
```

Outputs to `zig-out/` (`bin/md4x`, `lib/libmd4x*.a`, `include/md4x*.h`).

The project can also be consumed as a Zig package dependency via `build.zig.zon`.

Produces four static libraries, one executable, and optional WASM/NAPI targets:

- **libmd4x** — Parser library (compiled with `-DMD4X_USE_UTF8`)
- **libmd4x-html** — HTML renderer (links against libmd4x)
- **libmd4x-ast** — AST renderer (links against libmd4x)
- **libmd4x-ansi** — ANSI terminal renderer (links against libmd4x)
- **libmd4x-meta** — Meta renderer (links against libmd4x)
- **libmd4x-text** — Plain text renderer (links against libmd4x)
- **md4x** — CLI utility (supports `--format=html|text|json|ansi`)
- **md4x.wasm** — WASM library (`zig build wasm`, output: `packages/md4x/build/md4x.wasm`)
- **md4x.{platform}-{arch}[-musl].node** — Cross-compiled NAPI addons (`zig build napi-all`, 9 targets)

Compiler flags: `-Wall -Wextra -Wshadow -Wdeclaration-after-statement -O2`

## Testing

```sh
# Run all test suites:
bun scripts/run-tests.ts

# Individual test suite:
python3 test/run-testsuite.py -s test/spec.txt -p zig-out/bin/md4x

# Pathological inputs only:
python3 test/pathological-tests.py -p zig-out/bin/md4x
```

Test format: Markdown examples with `.` separator and expected HTML output. The test runner pipes input through `md4x` and compares normalized output.

Test suites: `spec.txt`, `spec-tables.txt`, `spec-strikethrough.txt`, `spec-tasklists.txt`, `spec-wiki-links.txt`, `spec-latex-math.txt`, `spec-permissive-autolinks.txt`, `spec-hard-soft-breaks.txt`, `spec-underline.txt`, `spec-frontmatter.txt`, `spec-components.txt`, `spec-attributes.txt`, `spec-alerts.txt`, `regressions.txt`, `coverage.txt`

## `md4x` CLI

```sh
md4x [OPTION]... [FILE]
# Reads from stdin if no FILE given
```

**General options:**

| Option                  | Description                                             |
| ----------------------- | ------------------------------------------------------- |
| `-o`, `--output=FILE`   | Output file (default: stdout)                           |
| `-t`, `--format=FORMAT` | Output format: `html` (default), `text`, `json`, `ansi` |
| `-s`, `--stat`          | Measure parsing time                                    |
| `-h`, `--help`          | Display help                                            |
| `-v`, `--version`       | Display version                                         |

All extensions are enabled by default (`MD_DIALECT_ALL`). No dialect preset flags.

**HTML output options:**

| Option               | Description                             |
| -------------------- | --------------------------------------- |
| `-f`, `--full-html`  | Generate full HTML document with header |
| `--html-title=TITLE` | Set document title (with `--full-html`) |
| `--html-css=URL`     | Add CSS link (with `--full-html`)       |

**ANSI output (`--format=ansi`):** Terminal-friendly output with ANSI escape codes for colors, bold, italic, underline, and other text styling.

**JSON output (`--format=json`):** Produces a Comark AST: `{"nodes":[...],"frontmatter":{...},"meta":{}}`. Each node is either a plain string (text) or a tuple array `[tag, props, ...children]`. Frontmatter YAML is parsed into the top-level `frontmatter` object. HTML comments are represented as `[null, {}, "comment body"]`.

## Code Generation Scripts

The `scripts/` directory contains TypeScript generators for lookup tables compiled into `md4x.c`:

- `build-entity-map.ts` — Fetches [WHATWG entities.json](https://html.spec.whatwg.org/entities.json), generates `entity.c`
- `build-folding-map.ts` — Unicode case folding from `scripts/unicode/CaseFolding.txt`
- `build-punct-map.ts` — Unicode punctuation categories from `scripts/unicode/DerivedGeneralCategory.txt`
- `build-whitespace-map.ts` — Unicode whitespace classification
- `_unicode-map.ts` — Shared helper for punct/whitespace map generators

These are run manually when updating Unicode compliance (currently Unicode 15.1).

## Development Best Practices

### AST Renderer: Union Safety with Dynamic Components

The AST renderer (`md4x-ast.c`) uses a `JSON_NODE` struct with a C union for type-specific detail data. **Dynamic components** (`tag_is_dynamic = 1`) always use `detail.component` and have a heap-allocated tag name. Since a user can create a component with any name (e.g. `::alert{...}`, `::pre{...}`, `::a{...}`), the tag name may collide with built-in static tags.

**Critical rule:** In `json_node_free`, `json_write_props`, and `json_serialize_node`, always check `node->tag_is_dynamic` **before** any `strcmp(node->tag, ...)` dispatch. If dynamic, use the component code path exclusively — never fall through to static tag handlers. Violating this causes:

1. **Double-free** — `json_node_free` frees the same union pointer via both the static tag cleanup and the dynamic cleanup (heap corruption, OOB in WASM)
2. **Wrong serialization** — `json_write_props` reads the union as the wrong type (e.g. interprets `raw_props` pointer as `alert.type_name`)

### WASM Binary

The WASM binary (`packages/md4x/build/md4x.wasm`) is gitignored and must be rebuilt with `zig build wasm` after C source changes. The `zig build wasm` step installs directly to `packages/md4x/build/`. Run `bun vitest run packages/md4x/test/wasm.test.mjs` to verify.

### Adding New Block/Span Types

When adding a new block or span type with its own detail struct:

1. Add the detail struct to the `JSON_NODE` union in `md4x-ast.c`
2. Handle it in `json_enter_block`/`json_enter_span` (build the node)
3. Handle it in `json_write_props` (serialize props) — place **after** the `tag_is_dynamic` check
4. Handle it in `json_node_free` (free heap strings) — place **after** the `tag_is_dynamic` check
5. If needed, handle it in `json_serialize_node` (special child rendering)
6. Update all three renderers (HTML, AST, ANSI) and the CLI
7. Add a test suite in `test/spec-*.txt` and update `scripts/run-tests.ts`
8. Add JS binding tests in `packages/md4x/test/_suite.mjs`
9. Rebuild WASM with `zig build wasm` and run `bun vitest run packages/md4x/test/wasm.test.mjs`

## Detailed Reference

@docs/parser-api.md
@docs/renderers.md
@docs/js-bindings.md
@docs/markdown-syntax.md
