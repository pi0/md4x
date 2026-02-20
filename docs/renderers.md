# Renderers

## HTML Renderer API (`md4x-html.h`)

Convenience library that wraps `md_parse()` and produces HTML output:

```c
int md_html(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

Only `<body>` contents are generated. Frontmatter blocks are suppressed from output.

Extended API with full-HTML document generation:

```c
typedef struct MD_HTML_OPTS {
    const char* title;      /* Document title override (NULL = use frontmatter) */
    const char* css_url;    /* CSS stylesheet URL (NULL = omit) */
} MD_HTML_OPTS;

int md_html_ex(const MD_CHAR* input, MD_SIZE input_size,
               void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
               void* userdata, unsigned parser_flags, unsigned renderer_flags,
               const MD_HTML_OPTS* opts);
```

When `MD_HTML_FLAG_FULL_HTML` is set, `md_html_ex()` generates a complete HTML document (`<!DOCTYPE html>`, `<head>`, `<body>`). If YAML frontmatter exists, `title` and `description` fields are used in `<head>`. The `opts->title` overrides the frontmatter title. `opts` may be NULL.

### Renderer Flags (`MD_HTML_FLAG_*`)

| Flag                             | Value    | Description                                         |
| -------------------------------- | -------- | --------------------------------------------------- |
| `MD_HTML_FLAG_DEBUG`             | `0x0001` | Send debug output from `md_parse()` to stderr       |
| `MD_HTML_FLAG_VERBATIM_ENTITIES` | `0x0002` | Do not translate HTML entities                      |
| `MD_HTML_FLAG_SKIP_UTF8_BOM`     | `0x0004` | Skip UTF-8 BOM at input start                       |
| `MD_HTML_FLAG_FULL_HTML`         | `0x0008` | Generate full HTML document (requires `md_html_ex`) |

### Rendering Details

- Frontmatter blocks are suppressed (not rendered in HTML output)
- Wiki links render as `<x-wikilink>` tags
- LaTeX math renders as `<x-equation>` tags
- Task lists render with `<input type="checkbox">` elements
- Table cells get `align` attribute when alignment is specified
- URL attributes are percent-encoded; HTML content is entity-escaped
- Alerts render as `<blockquote class="alert alert-{type}">` (type lowercased in class)

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

## AST Renderer API (`md4x-ast.h`)

Renders Markdown into a Comark AST (array-based JSON format):

```c
int md_ast(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

Produces `{"type":"comark","value":[...]}` where each node is either a plain JSON string (text) or a tuple array `["tag", {props}, ...children]`.

**Internal architecture:** Unlike the streaming HTML/ANSI renderers, the AST renderer builds an in-memory tree of `JSON_NODE` structs during parsing, then serializes the tree to JSON. Each node has a `detail` union for type-specific data (code block info, link href, component props, etc.). Nodes with `tag_is_dynamic = 1` are user-defined components — their tag name is heap-allocated and they use the `detail.component` union member exclusively. All dispatch on `node->tag` (in `json_node_free`, `json_write_props`, `json_serialize_node`) must check `tag_is_dynamic` first to avoid union misinterpretation when a component name collides with a built-in tag.

### AST Renderer Flags (`MD_AST_FLAG_*`)

| Flag                        | Value    | Description                                   |
| --------------------------- | -------- | --------------------------------------------- |
| `MD_AST_FLAG_DEBUG`         | `0x0001` | Send debug output from `md_parse()` to stderr |
| `MD_AST_FLAG_SKIP_UTF8_BOM` | `0x0002` | Skip UTF-8 BOM at input start                 |

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

## Shared JSON Writer (`md4x-json.h`)

Header-only utility providing JSON serialization and YAML-to-JSON conversion helpers. Used by both the AST and meta renderers.

```c
#include "md4x-json.h"
```

**Key components:**

- `JSON_WRITER` — Streaming JSON writer struct with callback-based output
- `json_write()` / `json_write_str()` — Raw and string output helpers
- `json_write_escaped()` / `json_write_string()` — JSON-escaped string output
- `json_write_yaml_props()` — Parses YAML frontmatter and writes key-value pairs as JSON properties (using libyaml)

## Meta Renderer API (`md4x-meta.h`)

Lightweight metadata extractor that parses frontmatter and headings from Markdown:

```c
int md_meta(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

Produces a flat JSON object with frontmatter properties spread at the top level plus a `headings` array. No AST construction — uses SAX callbacks to capture only frontmatter text and heading plain text.

**Example output:**

```json
{
  "title": "Hello",
  "tags": ["a", "b"],
  "headings": [
    { "level": 1, "text": "My Doc" },
    { "level": 2, "text": "Section 1" }
  ]
}
```

### Renderer Flags (`MD_META_FLAG_*`)

| Flag                         | Value    | Description                                   |
| ---------------------------- | -------- | --------------------------------------------- |
| `MD_META_FLAG_DEBUG`         | `0x0001` | Send debug output from `md_parse()` to stderr |
| `MD_META_FLAG_SKIP_UTF8_BOM` | `0x0002` | Skip UTF-8 BOM at input start                 |

### Rendering Details

- Frontmatter YAML properties are spread as top-level JSON keys (using libyaml for full YAML 1.1 support)
- Headings are collected as `{"level": N, "text": "..."}` objects in the `headings` array
- Heading text is extracted as plain text — inline formatting (bold, italic, code, etc.) is stripped
- HTML entities in headings are resolved to UTF-8 characters
- Uses streaming renderer pattern (like HTML renderer), no AST construction

## Text Renderer API (`md4x-text.h`)

Strips markdown formatting and produces plain text output:

```c
int md_text(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

### Renderer Flags (`MD_TEXT_FLAG_*`)

| Flag                         | Value    | Description                                   |
| ---------------------------- | -------- | --------------------------------------------- |
| `MD_TEXT_FLAG_DEBUG`         | `0x0001` | Send debug output from `md_parse()` to stderr |
| `MD_TEXT_FLAG_SKIP_UTF8_BOM` | `0x0002` | Skip UTF-8 BOM at input start                 |

### Rendering Details

- All inline formatting (bold, italic, underline, strikethrough, code spans) stripped — only text content remains
- Headings: plain text + newline
- Paragraphs: plain text + newline
- Lists: `- ` (unordered) or `1. ` (ordered) prefix with 2-space nesting indentation
- Task lists: `[x] ` / `[ ] ` prefix
- Code blocks: verbatim with 2-space indent
- Blockquotes: `> ` prefix (nested)
- Horizontal rules: `---`
- Tables: tab-separated cells
- Links: text content only (URL not shown)
- Images: alt text only
- Frontmatter: stripped (no output)
- Components/templates: transparent (children rendered normally)
- Alerts: type label + content with `> ` prefix
- Entities resolved to UTF-8 characters
- Raw HTML: stripped (no output)
- Uses streaming renderer pattern (like HTML renderer), no AST construction
