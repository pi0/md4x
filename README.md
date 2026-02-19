
# 📄 MD4X

Fast markdown parser and renderer based on [mity/md4c](https://github.com/mity/md4c/).

## Usage

### JavaScript (NAPI)

Sync API for Node.js using a native addon. Supports both ESM and CJS.

```js
import { renderToHtml, renderToJson, renderToAnsi } from "md4x/napi";

const html = renderToHtml("# Hello, **world**!");
const json = renderToJson("# Hello, **world**!");
const ansi = renderToAnsi("# Hello, **world**!");
```

### JavaScript (WASM)

Async API that works in Node.js and browsers. Lazy-loads the `.wasm` binary on first call.

```js
import { renderToHtml, renderToJson, renderToAnsi } from "md4x/wasm";

const html = await renderToHtml("# Hello, **world**!");
const json = await renderToJson("# Hello, **world**!");
const ansi = await renderToAnsi("# Hello, **world**!");
```



### C Library

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

### CLI

```sh
md4x [FILE]                    # HTML output (default)
md4x --format=json [FILE]     # JSON AST output
md4x --format=ansi [FILE]     # ANSI terminal output
echo "# Hi" | md4x            # Read from stdin
```

### Zig Package

MD4X can be consumed as a Zig package dependency via `build.zig.zon`.

## Building

Requires [Zig](https://ziglang.org/). No other external dependencies.

```sh
zig build                      # ReleaseFast (default)
zig build -Doptimize=Debug     # Debug build
zig build wasm                 # WASM target (~163K)
zig build napi                 # Node.js NAPI addon
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
