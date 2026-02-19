
# MD4C Change Log

## Next

### Frontmatter support (`MD_FLAG_FRONTMATTER`)

Added YAML-style frontmatter parsing as an opt-in extension. When enabled, a `---` fence on the very first line of the document opens a frontmatter block. Content is exposed verbatim via `MD_BLOCK_FRONTMATTER` callbacks and rendered as `<x-frontmatter>` in HTML. CLI flag: `--ffrontmatter`.
