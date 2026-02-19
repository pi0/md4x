
# MD4C Change Log

## Next

### CLI renamed from `md2html` to `md4c`

The CLI tool has been renamed from `md2html` to `md4c` and moved from `md2html/` to `cli/`. It now supports a `--format` (`-t`) flag to select the output format (`html`, `text`, `json`, `ansi`). HTML remains the default. Text and JSON formats will be implemented in a future release. The CMake option is now `BUILD_MD4C_CLI` (`BUILD_MD2HTML_EXECUTABLE` still works as an alias).

### JSON AST renderer (`libmd4c-json`)

Added a new renderer library (`src/md4c-json.c`, `src/md4c-json.h`) that converts Markdown into a nested JSON AST tree compatible with the [commonmark.js](https://github.com/commonmark/commonmark.js) AST format. Each node has `"type"`, type-specific properties, and either `"children"` (container nodes) or `"literal"` (leaf nodes).

The library is available as `libmd4c-json` (pkg-config: `md4c-json`, CMake target: `md4c::md4c-json`). The CLI supports it via `--format=json`.

Node types follow commonmark.js conventions: `document`, `block_quote`, `list`, `item`, `heading`, `code_block`, `html_block`, `paragraph`, `thematic_break`, `emph`, `strong`, `link`, `image`, `code`, `delete`, `text`, `linebreak`, `softbreak`, `html_inline`. Extension types: `table`, `table_head`, `table_body`, `table_row`, `table_header_cell`, `table_cell`, `latex_math`, `latex_math_display`, `wikilink`, `underline`, `frontmatter`.

### ANSI terminal renderer (`libmd4c-ansi`)

Added a new renderer library (`src/md4c-ansi.c`, `src/md4c-ansi.h`) that converts Markdown into ANSI terminal output with escape codes for colors, bold, italic, underline, strikethrough, and other styling. Links use OSC 8 clickable hyperlinks with a dim URL fallback for unsupported terminals.

The library is available as `libmd4c-ansi` (pkg-config: `md4c-ansi`, CMake target: `md4c::md4c-ansi`). The CLI supports it via `--format=ansi`. Pass `MD_ANSI_FLAG_NO_COLOR` to suppress all escape codes.

### Frontmatter support (`MD_FLAG_FRONTMATTER`)

Added YAML-style frontmatter parsing as an opt-in extension. When enabled, a `---` fence on the very first line of the document opens a frontmatter block. Content is exposed verbatim via `MD_BLOCK_FRONTMATTER` callbacks and rendered as `<x-frontmatter>` in HTML. CLI flag: `--ffrontmatter`.
