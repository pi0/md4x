# MD4C

> **Always keep this file (`AGENTS.md`) updated when making changes to the project.**
> **Also update `CHANGELOG.md` when adding or changing user-facing features.**

> C Markdown parser library by [Martin Mitáš](https://github.com/mity/md4c)

- **Version:** 0.5.2 (next: 0.5.3 WIP)
- **License:** MIT
- **Language:** C (C89/C99 compatible)
- **Spec:** CommonMark 0.31.2

## Project Structure

```
src/
  md4c.c              # Core parser (~6500 LoC)
  md4c.h              # Parser public API
  md4c-html.c          # HTML renderer library (~570 LoC)
  md4c-html.h          # HTML renderer public API
  entity.c             # HTML entity lookup table (generated)
  entity.h             # Entity header
  md4c.pc.in           # pkg-config for libmd4c
  md4c-html.pc.in      # pkg-config for libmd4c-html
  CMakeLists.txt       # Builds libmd4c + libmd4c-html
md2html/
  md2html.c            # CLI utility
  md2html.h            # (stub)
  cmdline.c            # Command-line parser (from c-reusables)
  cmdline.h            # Command-line parser API
  md2html.1            # Man page
  CMakeLists.txt       # Builds md2html executable
test/
  spec.txt             # CommonMark 0.31.2 spec tests
  spec-*.txt           # Extension-specific tests (tables, strikethrough, frontmatter, etc.)
  regressions.txt      # Bug regression tests
  coverage.txt         # Code coverage tests
  run-testsuite.py     # Individual test suite runner
  pathological-tests.py # Stress tests for DoS resistance
  prog.py              # Program execution wrapper
  normalize.py         # HTML normalization for comparison
  fuzzers/             # LibFuzzer integration + seed corpus
scripts/
  run-tests.py         # Main test runner (runs all suites)
  build_entity_map.py  # Generates entity.c from WHATWG spec
  build_folding_map.py # Unicode case folding map generator
  build_punct_map.py   # Punctuation character map generator
  build_whitespace_map.py # Whitespace classification generator
  coverity.sh          # Coverity Scan integration
  unicode/             # Unicode data files (CaseFolding.txt, DerivedGeneralCategory.txt)
.github/workflows/
  ci-build.yml         # Build + test (Linux/Windows, debug/release, coverage)
  ci-fuzz.yml          # OSS-Fuzz integration
```

## Building

Uses CMake. No external dependencies beyond the C standard library.

```sh
mkdir build && cd build
cmake ..
make
```

Produces two libraries and one executable:
- **libmd4c** — Parser library (compiled with `-DMD4C_USE_UTF8`)
- **libmd4c-html** — HTML renderer (links against libmd4c)
- **md2html** — CLI utility

CMake options:
- `BUILD_SHARED_LIBS=ON|OFF` — Shared (default Linux) vs static (default Windows)
- `BUILD_MD2HTML_EXECUTABLE=ON|OFF` — Whether to build the CLI (default ON)
- Default build type is `Release` (Debug is ~2x slower)

Compiler flags: `-Wall -Wextra -Wshadow -Wdeclaration-after-statement` (GCC/Clang), `/W3` (MSVC)

CMake exports `md4c::md4c` and `md4c::md4c-html` targets for `find_package(md4c)`.

## Testing

```sh
# From build directory:
python3 ../scripts/run-tests.py

# Individual test suite:
python3 test/run-testsuite.py -s test/spec.txt -p build/md2html/md2html

# Pathological inputs only:
python3 test/pathological-tests.py -p build/md2html/md2html
```

Test format: Markdown examples with `.` separator and expected HTML output. The test runner pipes input through `md2html` and compares normalized output.

Test suites: `spec.txt`, `spec-tables.txt`, `spec-strikethrough.txt`, `spec-tasklists.txt`, `spec-wiki-links.txt`, `spec-latex-math.txt`, `spec-permissive-autolinks.txt`, `spec-hard-soft-breaks.txt`, `spec-underline.txt`, `spec-frontmatter.txt`, `regressions.txt`, `coverage.txt`

## Parser API (`md4c.h`)

Core function:

```c
int md_parse(const MD_CHAR* text, MD_SIZE size, const MD_PARSER* parser, void* userdata);
```

Returns `0` on success, `-1` on runtime error (e.g. memory failure), or the non-zero return value of any callback that aborted parsing.

`MD_CHAR` is `char` by default, or `WCHAR` when `MD4C_USE_UTF16` is defined on Windows.

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

### Architecture

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

### Encoding

MD4C assumes ASCII-compatible encoding. Preprocessor macros:

| Macro | Effect |
|---|---|
| _(default)_ | UTF-8 support enabled (since v0.4.3, UTF-8 is default) |
| `MD4C_USE_ASCII` | Force ASCII-only mode |
| `MD4C_USE_UTF16` | Windows UTF-16 via `WCHAR` (Windows only) |

Unicode matters for: word boundary classification (emphasis), case-insensitive link reference matching (case-folding), entity translation (left to renderer).

### Block Types (`MD_BLOCKTYPE`)

| Type | HTML | Detail struct |
|---|---|---|
| `MD_BLOCK_DOC` | `<body>` | — |
| `MD_BLOCK_QUOTE` | `<blockquote>` | — |
| `MD_BLOCK_UL` | `<ul>` | `MD_BLOCK_UL_DETAIL` |
| `MD_BLOCK_OL` | `<ol>` | `MD_BLOCK_OL_DETAIL` |
| `MD_BLOCK_LI` | `<li>` | `MD_BLOCK_LI_DETAIL` |
| `MD_BLOCK_HR` | `<hr>` | — |
| `MD_BLOCK_H` | `<h1>`–`<h6>` | `MD_BLOCK_H_DETAIL` |
| `MD_BLOCK_CODE` | `<pre><code>` | `MD_BLOCK_CODE_DETAIL` |
| `MD_BLOCK_HTML` | _(raw HTML)_ | — |
| `MD_BLOCK_P` | `<p>` | — |
| `MD_BLOCK_TABLE` | `<table>` | `MD_BLOCK_TABLE_DETAIL` |
| `MD_BLOCK_THEAD` | `<thead>` | — |
| `MD_BLOCK_TBODY` | `<tbody>` | — |
| `MD_BLOCK_TR` | `<tr>` | — |
| `MD_BLOCK_TH` | `<th>` | `MD_BLOCK_TD_DETAIL` |
| `MD_BLOCK_TD` | `<td>` | `MD_BLOCK_TD_DETAIL` |
| `MD_BLOCK_FRONTMATTER` | `<x-frontmatter>` | — |

### Span Types (`MD_SPANTYPE`)

| Type | HTML | Detail struct |
|---|---|---|
| `MD_SPAN_EM` | `<em>` | — |
| `MD_SPAN_STRONG` | `<strong>` | — |
| `MD_SPAN_A` | `<a>` | `MD_SPAN_A_DETAIL` |
| `MD_SPAN_IMG` | `<img>` | `MD_SPAN_IMG_DETAIL` |
| `MD_SPAN_CODE` | `<code>` | — |
| `MD_SPAN_DEL` | `<del>` | — |
| `MD_SPAN_LATEXMATH` | _(inline math)_ | — |
| `MD_SPAN_LATEXMATH_DISPLAY` | _(display math)_ | — |
| `MD_SPAN_WIKILINK` | _(wiki link)_ | `MD_SPAN_WIKILINK_DETAIL` |
| `MD_SPAN_U` | `<u>` | — |

### Text Types (`MD_TEXTTYPE`)

| Type | Description |
|---|---|
| `MD_TEXT_NORMAL` | Normal text |
| `MD_TEXT_NULLCHAR` | NULL character (replace with U+FFFD) |
| `MD_TEXT_BR` | Hard line break (`<br>`) |
| `MD_TEXT_SOFTBR` | Soft line break |
| `MD_TEXT_ENTITY` | HTML entity (`&nbsp;`, `&#1234;`, `&#x12AB;`) |
| `MD_TEXT_CODE` | Text inside code block/span (`\n` for newlines, no BR events) |
| `MD_TEXT_HTML` | Raw HTML text (`\n` for newlines in block-level HTML) |
| `MD_TEXT_LATEXMATH` | Text inside LaTeX equation (processed like code spans) |

### Detail Structs

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

typedef struct MD_SPAN_A_DETAIL {
    MD_ATTRIBUTE href;
    MD_ATTRIBUTE title;
    int is_autolink;    /* Non-zero if autolink */
} MD_SPAN_A_DETAIL;

typedef struct MD_SPAN_IMG_DETAIL {
    MD_ATTRIBUTE src;
    MD_ATTRIBUTE title;
} MD_SPAN_IMG_DETAIL;

typedef struct MD_SPAN_WIKILINK_DETAIL {
    MD_ATTRIBUTE target;
} MD_SPAN_WIKILINK_DETAIL;
```

### `MD_ATTRIBUTE`

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

### Parser Flags

| Flag | Value | Description |
|---|---|---|
| `MD_FLAG_COLLAPSEWHITESPACE` | `0x0001` | Collapse non-trivial whitespace to single space |
| `MD_FLAG_PERMISSIVEATXHEADERS` | `0x0002` | Allow ATX headers without space (`###header`) |
| `MD_FLAG_PERMISSIVEURLAUTOLINKS` | `0x0004` | Recognize URLs as autolinks without `<>` |
| `MD_FLAG_PERMISSIVEEMAILAUTOLINKS` | `0x0008` | Recognize emails as autolinks without `<>` and `mailto:` |
| `MD_FLAG_NOINDENTEDCODEBLOCKS` | `0x0010` | Disable indented code blocks (fenced only) |
| `MD_FLAG_NOHTMLBLOCKS` | `0x0020` | Disable raw HTML blocks |
| `MD_FLAG_NOHTMLSPANS` | `0x0040` | Disable inline raw HTML |
| `MD_FLAG_TABLES` | `0x0100` | Enable tables extension |
| `MD_FLAG_STRIKETHROUGH` | `0x0200` | Enable strikethrough extension |
| `MD_FLAG_PERMISSIVEWWWAUTOLINKS` | `0x0400` | Enable `www.` autolinks |
| `MD_FLAG_TASKLISTS` | `0x0800` | Enable task list extension |
| `MD_FLAG_LATEXMATHSPANS` | `0x1000` | Enable `$` / `$$` LaTeX math |
| `MD_FLAG_WIKILINKS` | `0x2000` | Enable `[[wiki links]]` |
| `MD_FLAG_UNDERLINE` | `0x4000` | Enable underline (disables `_` emphasis) |
| `MD_FLAG_HARD_SOFT_BREAKS` | `0x8000` | Force all soft breaks to act as hard breaks |
| `MD_FLAG_FRONTMATTER` | `0x10000` | Enable frontmatter extension |

**Compound flags:**

- `MD_FLAG_PERMISSIVEAUTOLINKS` = email + URL + WWW autolinks
- `MD_FLAG_NOHTML` = no HTML blocks + no HTML spans
- `MD_DIALECT_COMMONMARK` = `0` (strict CommonMark)
- `MD_DIALECT_GITHUB` = permissive autolinks + tables + strikethrough + task lists

## HTML Renderer API (`md4c-html.h`)

Convenience library that wraps `md_parse()` and produces HTML output:

```c
int md_html(const MD_CHAR* input, MD_SIZE input_size,
            void (*process_output)(const MD_CHAR*, MD_SIZE, void*),
            void* userdata, unsigned parser_flags, unsigned renderer_flags);
```

Only `<body>` contents are generated — caller handles HTML header/footer.

### Renderer Flags (`MD_HTML_FLAG_*`)

| Flag | Value | Description |
|---|---|---|
| `MD_HTML_FLAG_DEBUG` | `0x0001` | Send debug output from `md_parse()` to stderr |
| `MD_HTML_FLAG_VERBATIM_ENTITIES` | `0x0002` | Do not translate HTML entities |
| `MD_HTML_FLAG_SKIP_UTF8_BOM` | `0x0004` | Skip UTF-8 BOM at input start |
| `MD_HTML_FLAG_XHTML` | `0x0008` | Generate XHTML (`<br />`, `<hr />`, `<img ... />`) |

### Rendering Details

- Wiki links render as `<x-wikilink>` tags
- LaTeX math renders as `<x-equation>` tags
- Task lists render with `<input type="checkbox">` elements
- Table cells get `align` attribute when alignment is specified
- URL attributes are percent-encoded; HTML content is entity-escaped

## `md2html` CLI

```sh
md2html [OPTION]... [FILE]
# Reads from stdin if no FILE given
```

**General options:**

| Option | Description |
|---|---|
| `-o`, `--output=FILE` | Output file (default: stdout) |
| `-f`, `--full-html` | Generate full HTML document with header |
| `-x`, `--xhtml` | Generate XHTML instead of HTML |
| `-s`, `--stat` | Measure parsing time |
| `-h`, `--help` | Display help |
| `-v`, `--version` | Display version |
| `--html-title=TITLE` | Set document title (with `--full-html`) |
| `--html-css=URL` | Add CSS link (with `--full-html`) |

**Dialect presets:** `--commonmark` (default), `--github`

**Extension flags:** `--fcollapse-whitespace`, `--flatex-math`, `--fpermissive-atx-headers`, `--fpermissive-autolinks`, `--fpermissive-url-autolinks`, `--fpermissive-www-autolinks`, `--fpermissive-email-autolinks`, `--fhard-soft-breaks`, `--fstrikethrough`, `--ftables`, `--ftasklists`, `--funderline`, `--fwiki-links`, `--ffrontmatter`

**Suppression flags:** `--fno-html-blocks`, `--fno-html-spans`, `--fno-html`, `--fno-indented-code`

**Renderer flags:** `--fverbatim-entities`

## Markdown Syntax Reference

### CommonMark Basics

**Block elements:** paragraphs, headings (`#` or setext), lists (unordered/ordered with nesting), blockquotes, code blocks (fenced or 4-space indent), horizontal rules, raw HTML blocks.

**Inline elements:** emphasis (`*`/`_`), strong (`**`/`__`), links, images, inline code, raw HTML spans, hard breaks (trailing spaces or `\`), soft breaks.

### Extension: Tables (`MD_FLAG_TABLES`)

```
| Column 1 | Column 2 |
|----------|----------|
| foo      | bar      |
```

- Alignment via colons: `:---` left, `:---:` center, `---:` right
- Leading/trailing pipes optional (except single-column)
- Max 128 columns (DoS protection)
- Cell content supports inline markdown

### Extension: Task Lists (`MD_FLAG_TASKLISTS`)

```
- [x] Completed
- [ ] Pending
```

### Extension: Strikethrough (`MD_FLAG_STRIKETHROUGH`)

`~text~` or `~~text~~`. Opener/closer must match length. Follows same flanking rules as emphasis.

### Extension: Permissive Autolinks

- **URL** (`MD_FLAG_PERMISSIVEURLAUTOLINKS`): `https://example.com`
- **Email** (`MD_FLAG_PERMISSIVEEMAILAUTOLINKS`): `john@example.com`
- **WWW** (`MD_FLAG_PERMISSIVEWWWAUTOLINKS`): `www.example.com`

### Extension: LaTeX Math (`MD_FLAG_LATEXMATHSPANS`)

Inline `$...$` and display `$$...$$`. Opener must not be preceded by alphanumeric; closer must not be followed by alphanumeric.

### Extension: Wiki Links (`MD_FLAG_WIKILINKS`)

`[[target]]` — Max 100 character destination.

### Extension: Underline (`MD_FLAG_UNDERLINE`)

`_text_` renders as underline instead of emphasis (disables `_` for emphasis).

### Extension: Frontmatter (`MD_FLAG_FRONTMATTER`)

YAML-style frontmatter delimited by `---` at the very start of the document. The opening `---` must be on the first line (no leading blank lines). Content is exposed as verbatim text via `MD_BLOCK_FRONTMATTER`. HTML renderer outputs `<x-frontmatter>...</x-frontmatter>`. If unclosed, the rest of the document is treated as frontmatter content.

## Code Generation Scripts

The `scripts/` directory contains Python generators for lookup tables compiled into `md4c.c`:

- `build_entity_map.py` — Fetches [WHATWG entities.json](https://html.spec.whatwg.org/entities.json), generates `entity.c`
- `build_folding_map.py` — Unicode case folding from `scripts/unicode/CaseFolding.txt`
- `build_punct_map.py` — Unicode punctuation categories from `scripts/unicode/DerivedGeneralCategory.txt`
- `build_whitespace_map.py` — Unicode whitespace classification

These are run manually when updating Unicode compliance (currently Unicode 15.1).
