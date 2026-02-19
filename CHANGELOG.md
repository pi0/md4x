
# MD4C Change Log

## Next

### CLI renamed from `md2html` to `md4c`

The CLI tool has been renamed from `md2html` to `md4c` and moved from `md2html/` to `cli/`. It now supports a `--format` (`-t`) flag to select the output format (`html`, `text`, `json`). HTML remains the default. Text and JSON formats will be implemented in a future release. The CMake option is now `BUILD_MD4C_CLI` (`BUILD_MD2HTML_EXECUTABLE` still works as an alias).

### Frontmatter support (`MD_FLAG_FRONTMATTER`)

Added YAML-style frontmatter parsing as an opt-in extension. When enabled, a `---` fence on the very first line of the document opens a frontmatter block. Content is exposed verbatim via `MD_BLOCK_FRONTMATTER` callbacks and rendered as `<x-frontmatter>` in HTML. CLI flag: `--ffrontmatter`.
