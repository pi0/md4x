# 📄 MD4X

[![npm version](https://img.shields.io/npm/v/md4x?style=flat&colorA=18181B&colorB=F0DB4F)](https://npmx.dev/package/md4x)
![md4x.wasm gzip size](<https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fdeno.bundlejs.com%2F%3Fq%3Dmd4x%2Fbuild%2Fmd4x.wasm&query=%24.size.compressedSize&label=md4x.wasm%20(gzip)&style=flat&colorA=18181B&colorB=F0DB4F>)

Fast and Small markdown parser and renderer based on [mity/md4c](https://github.com/mity/md4c/).

> [!TIP]
> [Online Playground](https://md4x.pi0.io/#/playground)

## CLI

```sh
# Local files
npx md4x README.md                          # ANSI output
npx md4x README.md -t html                  # HTML output
npx md4x README.md -t text                  # Plain text output (strip markdown)
npx md4x README.md -t ast                   # JSON AST output (comark)
npx md4x README.md -t meta                  # Metadata JSON output

# Remote sources
npx md4x https://nitro.build/guide          # Fetch and render any URL
npx md4x gh:nitrojs/nitro                   # GitHub repo → README.md
npx md4x npm:vue@3                          # npm package at specific version

# Stdin
echo "# Hello" | npx md4x -t text
cat README.md | npx md4x  -t html

# Output to file
npx md4x README.md -t meta -o README.json

# Full HTML document
npx md4x README.md -t html -f --html-title="My Docs"  # Wrap in full HTML with <head>
npx md4x README.md -t html -f --html-css=style.css    # Add CSS link
```

## JavaScript

Available as a native Node.js addon (NAPI) for maximum performance, or as a portable WASM module that works in any JavaScript runtime (Node.js, Deno, Bun, browsers, edge workers, etc.).

The bare `md4x` import auto-selects NAPI on Node.js and WASM elsewhere.

```js
import {
  init,
  renderToHtml,
  renderToAST,
  parseAST,
  renderToAnsi,
  renderToText,
  renderToMeta,
  parseMeta,
} from "md4x";

// await init(); // required for WASM, optional for NAPI

const html = renderToHtml("# Hello, **world**!");
const json = renderToAST("# Hello, **world**!"); // raw JSON string
const ast = parseAST("# Hello, **world**!"); // parsed ComarkTree object
const ansi = renderToAnsi("# Hello, **world**!");
const text = renderToText("# Hello, **world**!"); // plain text (stripped)
const metaJson = renderToMeta("# Hello, **world**!"); // raw JSON string
const meta = parseMeta("# Hello, **world**!"); // parsed meta
```

Both NAPI and WASM export a unified API with `init()`. For WASM, `init()` must be called before rendering. For NAPI, it is optional (the native binding loads lazily on first render call).

#### NAPI (Node.js native)

Synchronous, zero-overhead native addon. Best performance for server-side use.

```js
import { renderToHtml } from "md4x/napi";
```

#### WASM (universal)

Works anywhere with WebAssembly support. Requires a one-time async initialization.

```js
import { init, renderToHtml } from "md4x/wasm";

await init(); // call once before rendering
const html = renderToHtml("# Hello");
```

`init()` accepts an optional options object with a `wasm` property (`ArrayBuffer`, `Response`, `WebAssembly.Module`, or `Promise<Response>`). When called with no arguments, it loads the bundled `.wasm` file automatically.

### Benchmarks

(source: [packages/md4x/bench](./packages/md4x/bench))

```
benchmark                      avg (min … max) p75 / p99    (min … top 1%)
---------------------------------------------- -------------------------------
md4x-napi                         3.27 µs/iter   3.28 µs   3.35 µs ▄▅▇█▅▇▂▂▂▂▂
md4x-wasm                         5.62 µs/iter   5.69 µs   8.39 µs █▅▅▂▂▁▁▁▁▁▁
md4w                              5.80 µs/iter   5.82 µs   9.24 µs ▁█▆▂▂▁▁▁▁▁▁
markdown-it                      21.28 µs/iter  20.95 µs  39.72 µs ▁█▄▁▁▁▁▁▁▁▁
markdown-exit                    23.14 µs/iter  23.40 µs  39.71 µs ▁▆█▃▁▁▁▁▁▁▁

summary
  md4x-napi
   1.72x faster than md4x-wasm
   1.78x faster than md4w
   6.52x faster than markdown-it
   7.08x faster than markdown-exit

---------------------------------------------- -------------------------------
md4x (napi) ast (medium)          6.85 µs/iter   6.92 µs   6.97 µs ▂▇█▂▂▁▂▂▃▂▃
md4x (wasm) ast (medium)          8.28 µs/iter   8.30 µs   8.43 µs ▄▄▂▂▄█▂▁▂▁▂

summary
  md4x (napi) ast (medium)
   1.21x faster than md4x (wasm) ast (medium)

---------------------------------------------- -------------------------------
md4x (napi) parseAST (medium)    11.42 µs/iter  11.39 µs  11.87 µs █▂▅▁▁▁▁▁▁▁▂
md4x (wasm) parseAST (medium)    12.91 µs/iter  13.01 µs  13.09 µs ▃█▁▁▁▁▁▃▃▃▃
md4w parseAST (medium)           11.92 µs/iter  12.04 µs  12.13 µs ▃█▁▁▃▁▁▆▆▁▃
markdown-it parseAST (medium)    65.32 µs/iter  66.78 µs 108.61 µs ▅█▄▂▁▁▁▁▁▁▁
markdown-exit parseAST (medium)  65.09 µs/iter  66.02 µs  86.35 µs ▂█▄▃▂▁▁▁▁▁▁

summary
  md4x (napi) parseAST (medium)
   1.04x faster than md4w parseAST (medium)
   1.13x faster than md4x (wasm) parseAST (medium)
   5.7x faster than markdown-exit parseAST (medium)
   5.72x faster than markdown-it parseAST (medium)
```

## Zig Package

MD4X can be consumed as a Zig package dependency via `build.zig.zon`.

## Building

Requires [Zig](https://ziglang.org/). No other external dependencies.

```sh
zig build                      # ReleaseFast (default)
zig build -Doptimize=Debug     # Debug build
zig build wasm                 # WASM target (~163K)
zig build napi                 # Node.js NAPI addon
```

## C Library

SAX-like streaming parser with no AST construction. Link against `libmd4x` and the renderer you need.

#### HTML Renderer

```c
#include "md4x.h"
#include "md4x-html.h"

void output(const MD_CHAR* text, MD_SIZE size, void* userdata) {
    fwrite(text, 1, size, (FILE*) userdata);
}

md_html(input, input_size, output, stdout, MD_DIALECT_GITHUB, 0);
```

#### JSON Renderer

```c
#include "md4x.h"
#include "md4x-ast.h"

md_ast(input, input_size, output, stdout, MD_DIALECT_GITHUB, 0);
```

#### ANSI Renderer

```c
#include "md4x.h"
#include "md4x-ansi.h"

md_ansi(input, input_size, output, stdout, MD_DIALECT_GITHUB, 0);
```

#### Text Renderer

Strips markdown formatting and produces plain text:

```c
#include "md4x.h"
#include "md4x-text.h"

md_text(input, input_size, output, stdout, MD_DIALECT_GITHUB, 0);
```

#### Meta Renderer

Extracts frontmatter and headings as a flat JSON object:

```c
#include "md4x.h"
#include "md4x-meta.h"

md_meta(input, input_size, output, stdout, MD_DIALECT_GITHUB, 0);
// {"title":"Hello","headings":[{"level":1,"text":"Hello"}]}
```

#### Low-Level Parser

For custom rendering, use the SAX-like parser directly:

```c
#include "md4x.h"

int enter_block(MD_BLOCKTYPE type, void* detail, void* userdata) { return 0; }
int leave_block(MD_BLOCKTYPE type, void* detail, void* userdata) { return 0; }
int enter_span(MD_SPANTYPE type, void* detail, void* userdata) { return 0; }
int leave_span(MD_SPANTYPE type, void* detail, void* userdata) { return 0; }
int text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) { return 0; }

MD_PARSER parser = {
    .abi_version = 0,
    .flags = MD_DIALECT_GITHUB,
    .enter_block = enter_block,
    .leave_block = leave_block,
    .enter_span = enter_span,
    .leave_span = leave_span,
    .text = text,
};

md_parse(input, input_size, &parser, NULL);
```

## License

[MIT](./LICENSE.md)
