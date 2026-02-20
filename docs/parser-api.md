# Parser API (`md4x.h`)

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

## Architecture

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

## Encoding

MD4X assumes ASCII-compatible encoding. Preprocessor macros:

| Macro            | Effect                                                 |
| ---------------- | ------------------------------------------------------ |
| _(default)_      | UTF-8 support enabled (since v0.4.3, UTF-8 is default) |
| `MD4X_USE_ASCII` | Force ASCII-only mode                                  |
| `MD4X_USE_UTF16` | Windows UTF-16 via `WCHAR` (Windows only)              |

Unicode matters for: word boundary classification (emphasis), case-insensitive link reference matching (case-folding), entity translation (left to renderer).

## Block Types (`MD_BLOCKTYPE`)

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

## Span Types (`MD_SPANTYPE`)

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

## Text Types (`MD_TEXTTYPE`)

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

## Detail Structs

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

## `MD_ATTRIBUTE`

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

## Parser Flags

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
