# MD4X

> **Always keep this file (`AGENTS.md`) updated when making changes to the project.**
> **Always update `CHANGELOG.md` when adding removing user-facing features, APIs, build targets, CLI options, or library behavior.**

> C Markdown parser library (fork of [mity/md4c](https://github.com/mity/md4c))

- **Version:** 0.5.2 (next: 0.5.3 WIP)
- **License:** MIT
- **Language:** C (C89/C99 compatible)
- **Spec:** CommonMark 0.31.2
- **Build:** Zig (`zig build`)
- **JS Runtime:** Bun (do **not** use npm, pnpm, yarn, or npx — use `bun`/`bunx` exclusively)

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
    md4x-html.c        # HTML renderer library (~500 LoC)
    md4x-html.h        # HTML renderer public API
    md4x-json.c        # JSON AST renderer library (~530 LoC)
    md4x-json.h        # JSON renderer public API
    md4x-ansi.c        # ANSI terminal renderer library (~450 LoC)
    md4x-ansi.h        # ANSI renderer public API
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
    md4x.node            # Prebuilt NAPI binary
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
package.json             # Root workspace package (bun, workspaces: packages/*)
build.zig                # Zig build script
build.zig.zon            # Zig package manifest
.github/workflows/
  ci-build.yml         # Build + test (Linux/Windows, debug/release, coverage)
```

## Building

No external dependencies beyond the C standard library. Uses Zig build system.

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
- **libmd4x-json** — JSON AST renderer (links against libmd4x)
- **libmd4x-ansi** — ANSI terminal renderer (links against libmd4x)
- **md4x** — CLI utility (supports `--format=html|text|json|ansi`)
- **md4x.wasm** — WASM library (`zig build wasm`, output: `packages/md4x/build/md4x.wasm`)
- **md4x.node** — Node.js NAPI addon (`zig build napi`, output: `packages/md4x/build/md4x.node`)
- **md4x.{platform}-{arch}.node** — Cross-compiled NAPI addons (`zig build napi-all`, 6 targets)

Compiler flags: `-Wall -Wextra -Wshadow -Wdeclaration-after-statement -O2`

### WASM Target

```sh
zig build wasm                     # outputs zig-out/bin/md4x.wasm (~163K)
```

Builds a `wasm32-wasi` WASM binary with exported functions. Uses `ReleaseSmall` optimization. The WASM module requires minimal WASI imports (`fd_close`, `fd_seek`, `fd_write`, `proc_exit`) which can be stubbed for browser use.

**Exported functions:**

| Function                         | Description                              |
| -------------------------------- | ---------------------------------------- |
| `md4x_alloc(size) -> ptr`        | Allocate memory in WASM linear memory    |
| `md4x_free(ptr)`                 | Free previously allocated memory         |
| `md4x_to_html(ptr, size) -> int` | Render to HTML (0=ok, -1=error)          |
| `md4x_to_json(ptr, size) -> int` | Render to JSON AST                       |
| `md4x_to_ansi(ptr, size) -> int` | Render to ANSI                           |
| `md4x_result_ptr() -> ptr`       | Get output buffer pointer (after render) |
| `md4x_result_size() -> size`     | Get output buffer size (after render)    |

**Usage from JS (via `lib/wasm.mjs` wrapper):**

```js
import { initWasm, renderToHtml } from "md4x/wasm";

await initWasm(); // load WASM binary (call once before using render methods)

const html = renderToHtml("# Hello"); // sync after init
```

`initWasm(input?)` accepts an optional input: `ArrayBuffer`, `Uint8Array`, `WebAssembly.Module`, `Response`, or `Promise<Response>`. When called with no arguments in Node.js, it reads the bundled `.wasm` file from disk. All render methods are **sync** after initialization. All extensions are enabled by default (`MD_DIALECT_ALL`).

### NAPI Target (Node.js)

```sh
bunx nypm add node-api-headers
zig build napi -Dnapi-include=node_modules/node-api-headers/include   # host platform only
zig build napi-all -Dnapi-include=node_modules/node-api-headers/include  # all 6 platforms
```

Individual platform targets:

```sh
zig build napi-linux-x64 -Dnapi-include=node_modules/node-api-headers/include
zig build napi-linux-arm64 -Dnapi-include=node_modules/node-api-headers/include
zig build napi-darwin-x64 -Dnapi-include=node_modules/node-api-headers/include
zig build napi-darwin-arm64 -Dnapi-include=node_modules/node-api-headers/include
zig build napi-win32-x64 -Dnapi-include=node_modules/node-api-headers/include
zig build napi-win32-arm64 -Dnapi-include=node_modules/node-api-headers/include
```

`zig build napi` outputs `packages/md4x/build/md4x.node` (host platform, for development).
`zig build napi-all` outputs platform-specific binaries to `packages/md4x/build/`:

| Output file              | Platform            |
| ------------------------ | ------------------- |
| `md4x.linux-x64.node`    | Linux x86_64        |
| `md4x.linux-arm64.node`  | Linux aarch64       |
| `md4x.darwin-x64.node`   | macOS x86_64        |
| `md4x.darwin-arm64.node` | macOS Apple Silicon |
| `md4x.win32-x64.node`    | Windows x86_64      |
| `md4x.win32-arm64.node`  | Windows ARM64       |

Windows targets use `zig dlltool` to generate import libraries from `node_modules/node-api-headers/def/node_api.def`. The `-Dnapi-def` build option can override the `.def` path.

**Exported functions (C-level, raw strings):**

| Function       | Signature                                 |
| -------------- | ----------------------------------------- |
| `renderToHtml` | `(input: string) => string`               |
| `renderToJson` | `(input: string) => string` (JSON string) |
| `renderToAnsi` | `(input: string) => string`               |

**Usage (via `lib/napi.mjs` wrapper, which parses JSON):**

```js
import { renderToHtml } from "md4x/napi";

const html = renderToHtml("# Hello");
```

The NAPI API is sync. All extensions are enabled by default (`MD_DIALECT_ALL`). `renderToJson` returns the raw JSON string from the C renderer. `parseAST` parses it into a `ComarkTree` object.

The JS loader (`lib/napi.mjs`) auto-detects the platform via `process.platform` and `process.arch`, loading `md4x.{platform}-{arch}.node` with a fallback to `md4x.node` for development builds.

### JS Package Exports

Configured in `packages/md4x/package.json` via `exports`:

| Subpath       | Module                                           | Description                                  |
| ------------- | ------------------------------------------------ | -------------------------------------------- |
| `md4x` (bare) | `lib/napi.mjs` (node) / `lib/wasm.mjs` (default) | Auto-selects NAPI on Node.js, WASM elsewhere |
| `md4x/wasm`   | `lib/wasm.mjs`                                   | WASM-based API (sync after `initWasm()`)     |
| `md4x/napi`   | `lib/napi.mjs`                                   | Sync NAPI-based API (ESM only)               |

All extensions (`MD_DIALECT_ALL`) are enabled by default. No parser/renderer flag configuration is exposed to JS consumers.

**JS API functions:**

| Function                      | NAPI         | WASM                                      |
| ----------------------------- | ------------ | ----------------------------------------- |
| `initWasm(input?)`            | —            | `Promise<void>` (call once before render) |
| `renderToHtml(input: string)` | `string`     | `string`                                  |
| `renderToJson(input: string)` | `string`     | `string`                                  |
| `parseAST(input: string)`     | `ComarkTree` | `ComarkTree`                              |
| `renderToAnsi(input: string)` | `string`     | `string`                                  |

`renderToJson` returns the raw JSON string from the C renderer. `parseAST` calls `renderToJson` and parses the result into a `ComarkTree` object. See `lib/types.d.ts` for the Comark AST types (`ComarkTree`, `ComarkNode`, `ComarkElement`, `ComarkText`, `ComarkElementAttributes`).

### TypeScript Types (`lib/types.d.ts`)

The package exports TypeScript types for the Comark AST:

- `ComarkTree` — Root container: `{ type: "comark", value: ComarkNode[] }`
- `ComarkNode` — Either a `ComarkElement` (tuple array) or `ComarkText` (plain string)
- `ComarkElement` — Tuple: `[tag: string, props: ComarkElementAttributes, ...children: ComarkNode[]]`
- `ComarkText` — Plain string representing text content
- `ComarkElementAttributes` — Key-value record: `{ [key: string]: unknown }`

### JS Package Testing

Tests use vitest with a shared test suite (`packages/md4x/test/_suite.mjs`) that validates both NAPI and WASM bindings:

```sh
pnpm vitest run packages/md4x/test/napi.test.mjs   # NAPI tests
pnpm vitest run packages/md4x/test/wasm.test.mjs   # WASM tests
```

### JS Package Benchmarks

Benchmarks use `mitata` and compare against `md4w` and `markdown-it`:

```sh
bun packages/md4x/bench/index.mjs
```

### Workspace Setup

The root `package.json` defines a bun workspace (`"workspaces": ["packages/*"]`) with:

- `node-api-headers` — Required for NAPI builds
- `prettier` — Code formatting
- Package manager: `bun@1.3.9`

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

Test suites: `spec.txt`, `spec-tables.txt`, `spec-strikethrough.txt`, `spec-tasklists.txt`, `spec-wiki-links.txt`, `spec-latex-math.txt`, `spec-permissive-autolinks.txt`, `spec-hard-soft-breaks.txt`, `spec-underline.txt`, `spec-frontmatter.txt`, `spec-components.txt`, `spec-attributes.txt`, `regressions.txt`, `coverage.txt`

## Parser API (`md4x.h`)

Core function:

```c
int md_parse(const MD_CHAR* text, MD_SIZE size, const MD_PARSER* parser, void* userdata);
```

Returns `0` on success, `-1` on runtime error (e.g. memory failure), or the non-zero return value of any callback that aborted parsing.

`MD_CHAR` is `char` by default, or `WCHAR` when `MD4X_USE_UTF16` is defined on Windows.

The `MD_PARSER` struct holds callbacks and flags:

```c
typedef struct MD_PARSER {
    unsigned abi_version;   // Reserved, set to 0
    unsigned flags;         // Bitmask of MD_FLAG_xxxx values
    int (*enter_block)(MD_BLOCKTYPE, void* detail, void* userdata);
    int (*leave_block)(MD_BLOCKTYPE, void* detail, void* userdata);
    int (*enter_span)(MD_SPANTYPE, void* detail, void* userdata);
    int (*leave_span)(MD_SPANTYPE, void* detail, void* userdata);
    int (*text)(MD_TEXTTYPE, const MD_CHAR*, MD_SIZE, void* userdata);
    void (*debug_log)(const char* msg, void* userdata);  // Optional (NULL ok)
    void (*syntax)(void);   // Reserved, set to NULL
} MD_PARSER;
```

`MD_RENDERER` is a deprecated typedef alias for `MD_PARSER` (backward compat).

### Architecture

**SAX-like callback design** — No AST construction. Streaming for efficiency and low memory.

- Callbacks are invoked in nested order (block > span > text)
- Strings passed to callbacks are **not** null-terminated — always use the size parameter
- Any callback may abort parsing by returning non-zero

**Linear time guarantee** — Protections against pathological inputs:

- Code span mark limits (32 backticks max)
- Table column limits (128 max)
- Link reference definition abuse limits

**Callback sequence example** for `* foo **bar [link](http://example.com) baz**`:

```
enter_block(MD_BLOCK_DOC)
  enter_block(MD_BLOCK_UL)
    enter_block(MD_BLOCK_LI)
      text("foo ")
      enter_span(MD_SPAN_STRONG)
        text("bar ")
        enter_span(MD_SPAN_A)
          text("link")
        leave_span(MD_SPAN_A)
        text(" baz")
      leave_span(MD_SPAN_STRONG)
    leave_block(MD_BLOCK_LI)
  leave_block(MD_BLOCK_UL)
leave_block(MD_BLOCK_DOC)
```

### Encoding

MD4X assumes ASCII-compatible encoding. Preprocessor macros:

| Macro            | Effect                                                 |
| ---------------- | ------------------------------------------------------ |
| _(default)_      | UTF-8 support enabled (since v0.4.3, UTF-8 is default) |
| `MD4X_USE_ASCII` | Force ASCII-only mode                                  |
| `MD4X_USE_UTF16` | Windows UTF-16 via `WCHAR` (Windows only)              |

Unicode matters for: word boundary classification (emphasis), case-insensitive link reference matching (case-folding), entity translation (left to renderer).

### Block Types (`MD_BLOCKTYPE`)

| Type                   | HTML              | Detail struct               |
| ---------------------- | ----------------- | --------------------------- |
| `MD_BLOCK_DOC`         | `<body>`          | —                           |
| `MD_BLOCK_QUOTE`       | `<blockquote>`    | —                           |
| `MD_BLOCK_UL`          | `<ul>`            | `MD_BLOCK_UL_DETAIL`        |
| `MD_BLOCK_OL`          | `<ol>`            | `MD_BLOCK_OL_DETAIL`        |
| `MD_BLOCK_LI`          | `<li>`            | `MD_BLOCK_LI_DETAIL`        |
| `MD_BLOCK_HR`          | `<hr>`            | —                           |
| `MD_BLOCK_H`           | `<h1>`–`<h6>`     | `MD_BLOCK_H_DETAIL`         |
| `MD_BLOCK_CODE`        | `<pre><code>`     | `MD_BLOCK_CODE_DETAIL`      |
| `MD_BLOCK_HTML`        | _(raw HTML)_      | —                           |
| `MD_BLOCK_P`           | `<p>`             | —                           |
| `MD_BLOCK_TABLE`       | `<table>`         | `MD_BLOCK_TABLE_DETAIL`     |
| `MD_BLOCK_THEAD`       | `<thead>`         | —                           |
| `MD_BLOCK_TBODY`       | `<tbody>`         | —                           |
| `MD_BLOCK_TR`          | `<tr>`            | —                           |
| `MD_BLOCK_TH`          | `<th>`            | `MD_BLOCK_TD_DETAIL`        |
| `MD_BLOCK_TD`          | `<td>`            | `MD_BLOCK_TD_DETAIL`        |
| `MD_BLOCK_FRONTMATTER` | `<x-frontmatter>` | —                           |
| `MD_BLOCK_COMPONENT`   | _(dynamic tag)_   | `MD_BLOCK_COMPONENT_DETAIL` |
| `MD_BLOCK_TEMPLATE`    | `<template>`      | `MD_BLOCK_TEMPLATE_DETAIL`  |

### Span Types (`MD_SPANTYPE`)

| Type                        | HTML             | Detail struct                    |
| --------------------------- | ---------------- | -------------------------------- |
| `MD_SPAN_EM`                | `<em>`           | `MD_SPAN_ATTRS_DETAIL` or `NULL` |
| `MD_SPAN_STRONG`            | `<strong>`       | `MD_SPAN_ATTRS_DETAIL` or `NULL` |
| `MD_SPAN_A`                 | `<a>`            | `MD_SPAN_A_DETAIL`               |
| `MD_SPAN_IMG`               | `<img>`          | `MD_SPAN_IMG_DETAIL`             |
| `MD_SPAN_CODE`              | `<code>`         | `MD_SPAN_ATTRS_DETAIL` or `NULL` |
| `MD_SPAN_DEL`               | `<del>`          | `MD_SPAN_ATTRS_DETAIL` or `NULL` |
| `MD_SPAN_LATEXMATH`         | _(inline math)_  | —                                |
| `MD_SPAN_LATEXMATH_DISPLAY` | _(display math)_ | —                                |
| `MD_SPAN_WIKILINK`          | _(wiki link)_    | `MD_SPAN_WIKILINK_DETAIL`        |
| `MD_SPAN_U`                 | `<u>`            | `MD_SPAN_ATTRS_DETAIL` or `NULL` |
| `MD_SPAN_COMPONENT`         | _(dynamic tag)_  | `MD_SPAN_COMPONENT_DETAIL`       |
| `MD_SPAN_SPAN`              | `<span>`         | `MD_SPAN_SPAN_DETAIL`            |

### Text Types (`MD_TEXTTYPE`)

| Type                | Description                                                   |
| ------------------- | ------------------------------------------------------------- |
| `MD_TEXT_NORMAL`    | Normal text                                                   |
| `MD_TEXT_NULLCHAR`  | NULL character (replace with U+FFFD)                          |
| `MD_TEXT_BR`        | Hard line break (`<br>`)                                      |
| `MD_TEXT_SOFTBR`    | Soft line break                                               |
| `MD_TEXT_ENTITY`    | HTML entity (`&nbsp;`, `&#1234;`, `&#x12AB;`)                 |
| `MD_TEXT_CODE`      | Text inside code block/span (`\n` for newlines, no BR events) |
| `MD_TEXT_HTML`      | Raw HTML text (`\n` for newlines in block-level HTML)         |
| `MD_TEXT_LATEXMATH` | Text inside LaTeX equation (processed like code spans)        |

### Detail Structs

```c
typedef struct MD_BLOCK_UL_DETAIL {
    int is_tight;       /* Non-zero if tight list, zero if loose */
    MD_CHAR mark;       /* Bullet character: '-', '+', '*' */
} MD_BLOCK_UL_DETAIL;

typedef struct MD_BLOCK_OL_DETAIL {
    unsigned start;         /* Start index of ordered list */
    int is_tight;           /* Non-zero if tight list, zero if loose */
    MD_CHAR mark_delimiter; /* '.' or ')' */
} MD_BLOCK_OL_DETAIL;

typedef struct MD_BLOCK_LI_DETAIL {
    int is_task;                /* Non-zero only with MD_FLAG_TASKLISTS */
    MD_CHAR task_mark;          /* 'x', 'X', or ' ' (if is_task) */
    MD_OFFSET task_mark_offset; /* Offset of char between '[' and ']' */
} MD_BLOCK_LI_DETAIL;

typedef struct MD_BLOCK_H_DETAIL {
    unsigned level;     /* Header level (1-6) */
} MD_BLOCK_H_DETAIL;

typedef struct MD_BLOCK_CODE_DETAIL {
    MD_ATTRIBUTE info;      /* Full info string */
    MD_ATTRIBUTE lang;      /* First word of info string (language) */
    MD_CHAR fence_char;     /* Fence character, or zero for indented code */
} MD_BLOCK_CODE_DETAIL;

typedef struct MD_BLOCK_TABLE_DETAIL {
    unsigned col_count;         /* Number of columns */
    unsigned head_row_count;    /* Header rows (currently always 1) */
    unsigned body_row_count;    /* Body rows */
} MD_BLOCK_TABLE_DETAIL;

typedef struct MD_BLOCK_TD_DETAIL {
    MD_ALIGN align;     /* MD_ALIGN_DEFAULT, _LEFT, _CENTER, or _RIGHT */
} MD_BLOCK_TD_DETAIL;

typedef struct MD_SPAN_ATTRS_DETAIL {
    const MD_CHAR* raw_attrs;       /* Raw attrs from trailing {...}, or NULL. Not null-terminated */
    MD_SIZE raw_attrs_size;         /* Size of raw_attrs */
} MD_SPAN_ATTRS_DETAIL;

/* Note: fields up to raw_attrs_size are binary-compatible with MD_SPAN_IMG_DETAIL. */
typedef struct MD_SPAN_A_DETAIL {
    MD_ATTRIBUTE href;
    MD_ATTRIBUTE title;
    const MD_CHAR* raw_attrs;       /* Raw attrs from trailing {...}, or NULL */
    MD_SIZE raw_attrs_size;         /* Size of raw_attrs */
    int is_autolink;                /* Non-zero if autolink */
} MD_SPAN_A_DETAIL;

typedef struct MD_SPAN_IMG_DETAIL {
    MD_ATTRIBUTE src;
    MD_ATTRIBUTE title;
    const MD_CHAR* raw_attrs;       /* Raw attrs from trailing {...}, or NULL */
    MD_SIZE raw_attrs_size;         /* Size of raw_attrs */
} MD_SPAN_IMG_DETAIL;

typedef struct MD_SPAN_SPAN_DETAIL {
    const MD_CHAR* raw_attrs;       /* Raw attrs string from {...}. Not null-terminated */
    MD_SIZE raw_attrs_size;         /* Size of raw_attrs */
} MD_SPAN_SPAN_DETAIL;

typedef struct MD_SPAN_WIKILINK_DETAIL {
    MD_ATTRIBUTE target;
} MD_SPAN_WIKILINK_DETAIL;

typedef struct MD_SPAN_COMPONENT_DETAIL {
    MD_ATTRIBUTE tag_name;          /* Component name (e.g. "badge", "icon-star") */
    const MD_CHAR* raw_props;       /* Raw props string from {...}, or NULL. Not null-terminated */
    MD_SIZE raw_props_size;         /* Size of raw_props */
} MD_SPAN_COMPONENT_DETAIL;

typedef struct MD_BLOCK_COMPONENT_DETAIL {
    MD_ATTRIBUTE tag_name;          /* Component name (e.g. "alert", "card") */
    const MD_CHAR* raw_props;       /* Raw props string from {...}, or NULL. Not null-terminated */
    MD_SIZE raw_props_size;         /* Size of raw_props */
} MD_BLOCK_COMPONENT_DETAIL;

typedef struct MD_BLOCK_TEMPLATE_DETAIL {
    MD_ATTRIBUTE name;              /* Slot name (e.g. "header", "footer") */
} MD_BLOCK_TEMPLATE_DETAIL;
```

### `MD_ATTRIBUTE`

String attribute for non-text-flow content (titles, URLs, etc.) that may contain mixed substrings (normal text + entities):

```c
typedef struct MD_ATTRIBUTE {
    const MD_CHAR* text;
    MD_SIZE size;
    const MD_TEXTTYPE* substr_types;    /* Array of substring types */
    const MD_OFFSET* substr_offsets;    /* Array of substring offsets */
} MD_ATTRIBUTE;
```

Invariants: `substr_offsets[0] == 0`, `substr_offsets[LAST+1] == size`. Only `MD_TEXT_NORMAL`, `MD_TEXT_ENTITY`, and `MD_TEXT_NULLCHAR` substrings appear.

### Parser Flags

| Flag                               | Value     | Description                                                                       |
| ---------------------------------- | --------- | --------------------------------------------------------------------------------- |
| `MD_FLAG_COLLAPSEWHITESPACE`       | `0x0001`  | Collapse non-trivial whitespace to single space                                   |
| `MD_FLAG_PERMISSIVEATXHEADERS`     | `0x0002`  | Allow ATX headers without space (`###header`)                                     |
| `MD_FLAG_PERMISSIVEURLAUTOLINKS`   | `0x0004`  | Recognize URLs as autolinks without `<>`                                          |
| `MD_FLAG_PERMISSIVEEMAILAUTOLINKS` | `0x0008`  | Recognize emails as autolinks without `<>` and `mailto:`                          |
| `MD_FLAG_NOINDENTEDCODEBLOCKS`     | `0x0010`  | Disable indented code blocks (fenced only)                                        |
| `MD_FLAG_NOHTMLBLOCKS`             | `0x0020`  | Disable raw HTML blocks                                                           |
| `MD_FLAG_NOHTMLSPANS`              | `0x0040`  | Disable inline raw HTML                                                           |
| `MD_FLAG_TABLES`                   | `0x0100`  | Enable tables extension                                                           |
| `MD_FLAG_STRIKETHROUGH`            | `0x0200`  | Enable strikethrough extension                                                    |
| `MD_FLAG_PERMISSIVEWWWAUTOLINKS`   | `0x0400`  | Enable `www.` autolinks                                                           |
| `MD_FLAG_TASKLISTS`                | `0x0800`  | Enable task list extension                                                        |
| `MD_FLAG_LATEXMATHSPANS`           | `0x1000`  | Enable `$` / `$$` LaTeX math                                                      |
| `MD_FLAG_WIKILINKS`                | `0x2000`  | Enable `[[wiki links]]`                                                           |
| `MD_FLAG_UNDERLINE`                | `0x4000`  | Enable underline (disables `_` emphasis)                                          |
| `MD_FLAG_HARD_SOFT_BREAKS`         | `0x8000`  | Force all soft breaks to act as hard breaks                                       |
| `MD_FLAG_FRONTMATTER`              | `0x10000` | Enable frontmatter extension                                                      |
| `MD_FLAG_COMPONENTS`               | `0x20000` | Enable components (inline `:name[content]{props}` and block `::name{props}...::`) |
| `MD_FLAG_ATTRIBUTES`               | `0x40000` | Enable `{...}` attributes on inline elements and `[text]{.class}` spans           |

**Compound flags:**

- `MD_FLAG_PERMISSIVEAUTOLINKS` = email + URL + WWW autolinks
- `MD_FLAG_NOHTML` = no HTML blocks + no HTML spans
- `MD_DIALECT_COMMONMARK` = `0` (strict CommonMark)
- `MD_DIALECT_GITHUB` = permissive autolinks + tables + strikethrough + task lists
- `MD_DIALECT_ALL` = all additive extensions (autolinks + tables + strikethrough + tasklists + latex math + wikilinks + underline + frontmatter + components + attributes)

## HTML Renderer API (`md4x-html.h`)

Convenience library that wraps `md_parse()` and produces HTML output:

```c
int md_html(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

Only `<body>` contents are generated — caller handles HTML header/footer.

### Renderer Flags (`MD_HTML_FLAG_*`)

| Flag                             | Value    | Description                                   |
| -------------------------------- | -------- | --------------------------------------------- |
| `MD_HTML_FLAG_DEBUG`             | `0x0001` | Send debug output from `md_parse()` to stderr |
| `MD_HTML_FLAG_VERBATIM_ENTITIES` | `0x0002` | Do not translate HTML entities                |
| `MD_HTML_FLAG_SKIP_UTF8_BOM`     | `0x0004` | Skip UTF-8 BOM at input start                 |

### Rendering Details

- Wiki links render as `<x-wikilink>` tags
- LaTeX math renders as `<x-equation>` tags
- Task lists render with `<input type="checkbox">` elements
- Table cells get `align` attribute when alignment is specified
- URL attributes are percent-encoded; HTML content is entity-escaped

## Shared Property Parser (`md4x-props.h`)

Header-only utility for parsing component property strings (`{key="value" bool #id .class :bind='json'}`). Used by both JSON and HTML renderers.

```c
#include "md4x-props.h"

MD_PARSED_PROPS parsed;
md_parse_props(raw, size, &parsed);
```

**Parsed output (`MD_PARSED_PROPS`):**

| Field                     | Type                         | Description                                    |
| ------------------------- | ---------------------------- | ---------------------------------------------- |
| `props[32]`               | `MD_PROP[]`                  | Parsed props (key/value pairs, booleans, bind) |
| `n_props`                 | `int`                        | Number of parsed props                         |
| `id` / `id_size`          | `const MD_CHAR*` / `MD_SIZE` | `#id` shorthand (last wins)                    |
| `class_buf` / `class_len` | `MD_CHAR[512]` / `MD_SIZE`   | Merged `.class` values (space-separated)       |

**Prop types (`MD_PROP_TYPE`):**

| Type              | Syntax                                    | Description              |
| ----------------- | ----------------------------------------- | ------------------------ |
| `MD_PROP_STRING`  | `key="value"`, `key='value'`, `key=value` | String prop              |
| `MD_PROP_BOOLEAN` | `flag`                                    | Boolean prop (bare word) |
| `MD_PROP_BIND`    | `:key='json'`                             | JSON passthrough         |

All `key`/`value` pointers are zero-copy references into the original raw string (not null-terminated — use `*_size` fields).

## JSON Renderer API (`md4x-json.h`)

Renders Markdown into a Comark AST (array-based JSON format):

```c
int md_json(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

Produces `{"type":"comark","value":[...]}` where each node is either a plain JSON string (text) or a tuple array `["tag", {props}, ...children]`.

### Renderer Flags (`MD_JSON_FLAG_*`)

| Flag                         | Value    | Description                                   |
| ---------------------------- | -------- | --------------------------------------------- |
| `MD_JSON_FLAG_DEBUG`         | `0x0001` | Send debug output from `md_parse()` to stderr |
| `MD_JSON_FLAG_SKIP_UTF8_BOM` | `0x0002` | Skip UTF-8 BOM at input start                 |

## ANSI Renderer API (`md4x-ansi.h`)

Renders Markdown into ANSI terminal output with escape codes for styling:

```c
int md_ansi(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

### Renderer Flags (`MD_ANSI_FLAG_*`)

| Flag                         | Value    | Description                                    |
| ---------------------------- | -------- | ---------------------------------------------- |
| `MD_ANSI_FLAG_DEBUG`         | `0x0001` | Send debug output from `md_parse()` to stderr  |
| `MD_ANSI_FLAG_SKIP_UTF8_BOM` | `0x0002` | Skip UTF-8 BOM at input start                  |
| `MD_ANSI_FLAG_NO_COLOR`      | `0x0004` | Suppress ANSI escape codes (plain text output) |

### Rendering Details

- Headings: bold magenta (`\033[1;35m`)
- Bold/strong: bold (`\033[1m`)
- Italic/emphasis: italic (`\033[3m`)
- Underline: underline (`\033[4m`)
- Strikethrough: strikethrough (`\033[9m`)
- Inline code: cyan (`\033[36m`)
- Code blocks: dim (`\033[2m`) with 2-space indent
- Links: underline blue (`\033[4;34m`) with OSC 8 clickable hyperlinks
- Blockquotes: dim vertical bar prefix (`│`)
- Horizontal rules: box-drawing line (`────────`)
- Lists: dim bullet/number prefix with nesting indentation
- Task lists: `[x]`/`[ ]` with green for checked items
- Images: `[image: alt]` in dim
- Entities resolved to UTF-8 characters

Uses streaming renderer pattern (like HTML renderer), no AST construction.

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

**ANSI output (`--format=ansi`):**

Produces terminal-friendly output with ANSI escape codes for colors, bold, italic, underline, and other text styling. Use `MD_ANSI_FLAG_NO_COLOR` (or pipe through `cat` to strip codes) for plain text.

**JSON output (`--format=json`):**

Produces a Comark AST: `{"type":"comark","value":[...]}`. Each node is either a plain string (text) or a tuple array `[tag, props, ...children]`.

Tag mappings — blocks: `blockquote`, `ul`, `ol` (start), `li` (task, checked), `hr`, `h1`–`h6`, `pre` (language) with inner `code`, `html_block`, `p`, `table`, `thead`, `tbody`, `tr`, `th` (align), `td` (align), `frontmatter`, block components (dynamic tag name with parsed props). Spans: `em`, `strong`, `a` (href, title), `img` (src, alt, title — void), `code`, `del`, `math`, `math-display`, `wikilink` (target), `u`, inline components (dynamic tag name with parsed props). Text: plain strings (merged), `["br",{}]` for hard breaks, `"\n"` for soft breaks.

Code blocks serialize as `["pre", {language}, ["code", {class: "language-X"}, literal]]`. Images are void elements with alt text in props: `["img", {src, alt}]`.

The JSON renderer is implemented as a library in `src/renderers/md4x-json.c` (public API in `src/renderers/md4x-json.h`). It builds an in-memory AST during SAX-like parsing callbacks, then serializes it after parsing completes.

## Markdown Syntax Reference

### CommonMark Basics

**Block elements:** paragraphs, headings (`#` or setext), lists (unordered/ordered with nesting), blockquotes, code blocks (fenced or 4-space indent), horizontal rules, raw HTML blocks.

**Inline elements:** emphasis (`*`/`_`), strong (`**`/`__`), links, images, inline code, raw HTML spans, hard breaks (trailing spaces or `\`), soft breaks.

### Extension: Tables (`MD_FLAG_TABLES`)

```
| Column 1 | Column 2 |
|----------|----------|
| foo      | bar      |
```

- Alignment via colons: `:---` left, `:---:` center, `---:` right
- Leading/trailing pipes optional (except single-column)
- Max 128 columns (DoS protection)
- Cell content supports inline markdown

### Extension: Task Lists (`MD_FLAG_TASKLISTS`)

```
- [x] Completed
- [ ] Pending
```

### Extension: Strikethrough (`MD_FLAG_STRIKETHROUGH`)

`~text~` or `~~text~~`. Opener/closer must match length. Follows same flanking rules as emphasis.

### Extension: Permissive Autolinks

- **URL** (`MD_FLAG_PERMISSIVEURLAUTOLINKS`): `https://example.com`
- **Email** (`MD_FLAG_PERMISSIVEEMAILAUTOLINKS`): `john@example.com`
- **WWW** (`MD_FLAG_PERMISSIVEWWWAUTOLINKS`): `www.example.com`

### Extension: LaTeX Math (`MD_FLAG_LATEXMATHSPANS`)

Inline `$...$` and display `$$...$$`. Opener must not be preceded by alphanumeric; closer must not be followed by alphanumeric.

### Extension: Wiki Links (`MD_FLAG_WIKILINKS`)

`[[target]]` — Max 100 character destination.

### Extension: Underline (`MD_FLAG_UNDERLINE`)

`_text_` renders as underline instead of emphasis (disables `_` for emphasis).

### Extension: Frontmatter (`MD_FLAG_FRONTMATTER`)

YAML-style frontmatter delimited by `---` at the very start of the document. The opening `---` must be on the first line (no leading blank lines). Content is exposed as verbatim text via `MD_BLOCK_FRONTMATTER`. HTML renderer outputs `<x-frontmatter>...</x-frontmatter>`. If unclosed, the rest of the document is treated as frontmatter content.

### Extension: Inline Components (`MD_FLAG_COMPONENTS`)

Inline components use the MDC syntax: `:component-name`, `:component[content]`, `:component[content]{props}`, `:component{props}`.

- **Standalone**: `:icon-star` — requires hyphen in name (to avoid URL/email conflicts)
- **With content**: `:badge[New]` — content supports inline markdown (emphasis, links, etc.)
- **With props**: `:badge[New]{color="blue"}` — raw props passed to renderers
- **Props only**: `:tooltip{text="Hover"}`

Constraints:

- `:` must not be preceded by an alphanumeric character
- Component name: `[a-zA-Z][a-zA-Z0-9-]*`
- Standalone components (no `[content]` or `{props}`) require a hyphen in the name

Property syntax in `{...}`: `key="value"`, `key='value'`, `bool` (boolean true), `#id`, `.class`, `:key='json'` (JSON passthrough). Multiple `.class` values are merged.

HTML renderer: `<component-name ...attrs>content</component-name>`. JSON renderer: `["component-name", {props}, ...children]`. ANSI renderer: cyan-colored text.

### Extension: Block Components (`MD_FLAG_COMPONENTS`)

Block components use the MDC syntax with `::` fences. They are container blocks — content between open and close is parsed as normal markdown.

```
::alert{type="info"}
This is **important** content.
::
```

- **Basic**: `::name\ncontent\n::` — content is parsed as markdown blocks
- **With props**: `::name{key="value" bool #id .class}\ncontent\n::`
- **Empty**: `::divider\n::` — no content between open/close
- **Nested**: Use more colons for outer containers: `:::outer\n::inner\n::\n:::`
- **Deep nesting**: `::::` > `:::` > `::` (outer must have more colons than inner)

Constraints:

- Block components **cannot interrupt paragraphs** (require blank line before)
- Opening line: `::name` or `::name{props}` (2+ colons, component name, optional props)
- Closing line: `::` (2+ colons only, no name)
- A closer with N colons closes the innermost open component with ≤N colons
- Component name: `[a-zA-Z][a-zA-Z0-9-]*` (same as inline components)
- Content is always treated as loose (paragraphs wrapped in `<p>`)

Implementation: Block components use the container mechanism (`MD_CONTAINER` with `ch = ':'`). Component info (name/props source offsets) is stored in a growing array on `MD_CTX`, indexed by the block's `data` field.

HTML renderer: `<component-name ...attrs>content</component-name>`. JSON renderer: `["component-name", {props}, ...children]`. ANSI renderer: cyan-colored text.

### Component Slots (`MD_FLAG_COMPONENTS`)

Inside a block component, `#slot-name` at line start creates a named slot. Content after `#slot-name` until the next `#slot` or `::` closing is the slot body. Content before the first `#slot` stays as direct children (default slot).

```
::card
#header
## Card Title

#content
Main content

#footer
Footer text
::
```

Constraints:

- `#slot-name` must be at the start of a line (after container prefixes)
- Slot name: `[a-zA-Z][a-zA-Z0-9-]*` (same as component names)
- Slots **cannot interrupt paragraphs** (require blank line before)
- Slots are only recognized inside block component containers
- `#slot-name` outside a component is treated as literal text

Implementation: Slots use the container mechanism (`MD_CONTAINER` with `ch = '#'`). Slot info (name offsets) is stored in a growing array on `MD_CTX`, indexed by the block's `data` field. A new `#slot` implicitly closes any existing slot within the current component.

HTML renderer: `<template name="slot-name">...content...</template>`. JSON renderer: `["template", {"name": "slot-name"}, ...children]`. ANSI renderer: transparent (content renders normally).

### Extension: Inline Attributes (`MD_FLAG_ATTRIBUTES`)

Attributes can be added to inline elements using `{...}` syntax immediately after the closing delimiter:

```
**bold**{.highlight}       → <strong class="highlight">bold</strong>
*italic*{#myid}            → <em id="myid">italic</em>
`code`{.lang}              → <code class="lang">code</code>
~~del~~{.red}              → <del class="red">del</del>
_underline_{.accent}       → <u class="accent">underline</u>
[Link](url){target="_blank"} → <a href="url" target="_blank">Link</a>
![img](pic.png){.responsive} → <img src="pic.png" alt="img" class="responsive">
```

The `[text]{.class}` syntax (brackets NOT followed by `(url)`) creates a generic `<span>`:

```
[text]{.class}             → <span class="class">text</span>
[**bold** text]{.styled}   → <span class="styled"><strong>bold</strong> text</span>
```

Property syntax is shared with components: `{key="value" bool #id .class}`. Multiple `.class` values are merged. Empty `{}` is a no-op.

Constraints:

- `{...}` must immediately follow the closing delimiter (no space)
- Only applies to resolved inline elements (not plain text — `hello{.class}` is literal)
- Spans with `MD_FLAG_ATTRIBUTES`: em/strong/code/del/u pass `MD_SPAN_ATTRS_DETAIL*` (or `NULL` without attrs), links/images extend their detail structs with `raw_attrs`/`raw_attrs_size`
- `MD_SPAN_SPAN` is emitted for `[text]{attrs}` with `MD_SPAN_SPAN_DETAIL`

HTML renderer: attributes rendered on opening tags. JSON renderer: attrs merged into node props. ANSI renderer: transparent (ignores attrs).

## Code Generation Scripts

The `scripts/` directory contains TypeScript generators for lookup tables compiled into `md4x.c`:

- `build-entity-map.ts` — Fetches [WHATWG entities.json](https://html.spec.whatwg.org/entities.json), generates `entity.c`
- `build-folding-map.ts` — Unicode case folding from `scripts/unicode/CaseFolding.txt`
- `build-punct-map.ts` — Unicode punctuation categories from `scripts/unicode/DerivedGeneralCategory.txt`
- `build-whitespace-map.ts` — Unicode whitespace classification
- `_unicode-map.ts` — Shared helper for punct/whitespace map generators

These are run manually when updating Unicode compliance (currently Unicode 15.1).
