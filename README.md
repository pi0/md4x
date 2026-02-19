
# 📄 MD4X

Fast and Small markdown parser and renderer based on [mity/md4c](https://github.com/mity/md4c/).

**[> Online Playground](https://pi0.github.io/md4x)**

## CLI

```sh
# Local files
npx md4x README.md                          # ANSI output (default in TTY)
npx md4x -t html doc.md                     # HTML output
npx md4x -t json doc.md                     # JSON AST output (comark)

# Remote sources
npx md4x https://nitro.build/guide          # Fetch and render any URL
npx md4x gh:nitrojs/nitro                   # GitHub repo → README.md
npx md4x gh:pi0/md4c/blob/main/AGENTS.md    # GitHub file (blob URL)
npx md4x npm:vue@3                          # npm package at specific version

# Stdin
echo "# Hello" | npx md4x                   # Pipe markdown (HTML when piped)
curl -s https://example.com/doc.md | npx md4x

# Full HTML document
npx md4x -f --html-title="My Doc" doc.md    # Wrap in full HTML with <head>
npx md4x -f --html-css=style.css doc.md     # Add CSS link

# Output to file
npx md4x -t html -o out.html doc.md         # Write to file instead of stdout
```

## JavaScript

You  for Node.js using a native addon. Supports both ESM and CJS.

```js
import { renderToHtml, renderToJson, renderToAnsi } from "md4x";

// import { ... } from "md4x/napi";
// import { initWasm } from "md4x/wasm";

const html = renderToHtml("# Hello, **world**!");
const json = renderToJson("# Hello, **world**!");
const ansi = renderToAnsi("# Hello, **world**!");
```

### Benchmarks

(source: [packages/md4x/bench](./packages/md4x/bench))

```
clk: ~5.50 GHz
cpu: AMD Ryzen 9 9950X3D 16-Core Processor
runtime: bun 1.3.9 (x64-linux)

benchmark                   avg (min … max) p75 / p99    (min … top 1%)
_------------------------------------------ -------------------------------_
md4x-napi                      3.05 µs/iter   3.11 µs   3.17 µs ▅█▂▂▂▃▆▅▆▂▂
md4x-wasm                      5.41 µs/iter   5.42 µs   9.75 µs ██▄▂▁▁▁▁▁▁▁
md4w                           5.84 µs/iter   5.80 µs  10.08 µs ▂█▄▂▁▁▁▁▁▁▁
markdown-it                   21.53 µs/iter  21.02 µs  41.14 µs ▁█▃▁▁▁▁▁▁▁▁

summary
  md4x-napi
   1.77x faster than md4x-wasm
   1.91x faster than md4w
   7.05x faster than markdown-it
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
#include "md4x-json.h"

md_json(input, input_size, output, stdout, MD_DIALECT_GITHUB, 0);
```

#### ANSI Renderer

```c
#include "md4x.h"
#include "md4x-ansi.h"

md_ansi(input, input_size, output, stdout, MD_DIALECT_GITHUB, 0);
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


## Extensions

All extensions are enabled by default in the JS and CLI interfaces (`MD_DIALECT_ALL`).

| Extension | Flag |
|---|---|
| Tables | `MD_FLAG_TABLES` |
| Strikethrough | `MD_FLAG_STRIKETHROUGH` |
| Task lists | `MD_FLAG_TASKLISTS` |
| Autolinks | `MD_FLAG_PERMISSIVEAUTOLINKS` |
| LaTeX math | `MD_FLAG_LATEXMATHSPANS` |
| Wiki links | `MD_FLAG_WIKILINKS` |
| Underline | `MD_FLAG_UNDERLINE` |
| Frontmatter | `MD_FLAG_FRONTMATTER` |

Dialect presets: `MD_DIALECT_COMMONMARK` (strict), `MD_DIALECT_GITHUB`, `MD_DIALECT_ALL`.

## License

[MIT](./LICENSE.md)
