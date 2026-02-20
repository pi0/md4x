# Renderers

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
