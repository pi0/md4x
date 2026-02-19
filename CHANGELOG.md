
# MD4X Change Log

## Next

### Zig build system

Replaced CMake with Zig build system. Build with `zig build` (defaults to `ReleaseFast`). The project can also be consumed as a Zig package dependency via `build.zig.zon`.

### CLI renamed from `md2html` to `md4x`

The CLI tool has been renamed from `md2html` to `md4x` and moved from `md2html/` to `src/cli/`. It now supports a `--format` (`-t`) flag to select the output format (`html`, `text`, `json`, `ansi`). HTML remains the default.

### JSON AST renderer (`libmd4x-json`)

Added a new renderer library (`src/md4x-json.c`, `src/md4x-json.h`) that converts Markdown into a nested JSON AST tree compatible with the [commonmark.js](https://github.com/commonmark/commonmark.js) AST format. Each node has `"type"`, type-specific properties, and either `"children"` (container nodes) or `"literal"` (leaf nodes).

The CLI supports it via `--format=json`.

Node types follow commonmark.js conventions: `document`, `block_quote`, `list`, `item`, `heading`, `code_block`, `html_block`, `paragraph`, `thematic_break`, `emph`, `strong`, `link`, `image`, `code`, `delete`, `text`, `linebreak`, `softbreak`, `html_inline`. Extension types: `table`, `table_head`, `table_body`, `table_row`, `table_header_cell`, `table_cell`, `latex_math`, `latex_math_display`, `wikilink`, `underline`, `frontmatter`.

### ANSI terminal renderer (`libmd4x-ansi`)

Added a new renderer library (`src/md4x-ansi.c`, `src/md4x-ansi.h`) that converts Markdown into ANSI terminal output with escape codes for colors, bold, italic, underline, strikethrough, and other styling. Links use OSC 8 clickable hyperlinks with a dim URL fallback for unsupported terminals.

The CLI supports it via `--format=ansi`. Pass `MD_ANSI_FLAG_NO_COLOR` to suppress all escape codes.

### WASM target (`zig build wasm`)

Added a WebAssembly build target (`wasm32-wasi`) that produces a ~163K `.wasm` binary. Exposes `md4x_to_html`, `md4x_to_json`, and `md4x_to_ansi` functions callable from JavaScript, along with `md4x_alloc`/`md4x_free` for memory management and `md4x_result_ptr`/`md4x_result_size` for reading output.

### Node.js NAPI addon (`zig build napi`)

Added a Node-API native addon target that produces a `.node` shared library. Exposes `renderToHtml`, `renderToJson`, and `renderToAnsi` functions that take a string and optional parser/renderer flags, returning the rendered output as a string. Requires `node-api-headers` (`zig build napi -Dnapi-include=node_modules/node-api-headers/include`).

### Frontmatter support (`MD_FLAG_FRONTMATTER`)

Added YAML-style frontmatter parsing as an opt-in extension. When enabled, a `---` fence on the very first line of the document opens a frontmatter block. Content is exposed verbatim via `MD_BLOCK_FRONTMATTER` callbacks and rendered as `<x-frontmatter>` in HTML. CLI flag: `--ffrontmatter`.
