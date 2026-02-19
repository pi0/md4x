# Comark MD Syntax Implementation Plan

> **After each completed step:** update this file (check the box) and commit changes with `git commit && git push`.
>
> **Running tests:** always use `timeout 3 <command>` (3 second max) to prevent hangs.
> ```sh
> timeout 3 bun scripts/run-tests.ts           # C test suites
> timeout 3 pnpm vitest run packages/md4x/test  # JS binding tests
> ```

## Current State

The MD4X parser supports CommonMark 0.31.2 + GFM extensions (tables, strikethrough, task lists, frontmatter, wiki links, LaTeX math, underline, autolinks). **No Comark-specific syntax is implemented yet.** The JSON renderer outputs `{"type":"comark","value":[...]}` but only for standard markdown elements.

### Features to implement (from `spec/comark-md.md`)

1. **Code block metadata** — filename `[file.js]` and line highlights `{1-3,5}` in fenced code info strings
2. **Heading IDs** — auto-generated `id` attributes from heading text
3. **Inline components** — `:component-name`, `:component[content]{props}`
4. **Block components** — `::component-name{props}...::`, nesting `:::`, `::::`
5. **Component slots** — `#slot-name` inside block components
6. **Component properties** — `{prop="value" bool #id .class}` on components
7. **Attributes on native elements** — `**bold**{.class}`, `[link](url){target="_blank"}`
8. **Span attributes** — `[text]{.class}` creating styled spans
9. **Emoji shortcodes** — `:emoji_name:` → unicode

---

## Implementation Steps

### Phase 1: Code Block Metadata (JSON renderer only — no parser changes)

The parser already exposes `MD_BLOCK_CODE_DETAIL.info` which contains the full info string (e.g. `javascript [app.js] {1-3}`). We parse filename and highlights in the JSON renderer from the existing info string.

- [ ] **1.1** Parse `[filename]` from code block info string in JSON renderer (`md4x-json.c`)
  - Extract `[...]` from `info` string, store as `filename` in code detail
  - Handle escaped chars: `[@[...slug\].ts]`
  - Emit `"filename":"..."` in JSON props for `pre` nodes

- [ ] **1.2** Parse `{highlights}` from code block info string in JSON renderer
  - Extract `{...}` from `info` string
  - Parse ranges like `1-3`, individual lines `5`, and combos `1-3,5,7`
  - Emit `"highlights":[1,2,3,5,7]` as expanded integer array in JSON props

- [ ] **1.3** Parse remaining meta key-value pairs from info string
  - After extracting language, filename, highlights — remaining text is `meta`
  - Emit `"meta":"..."` in JSON props if non-empty

- [ ] **1.4** Add test cases for code block metadata
  - Add `test/spec-codeblock-meta.txt` with HTML test cases (HTML renderer should pass through)
  - Add JS test cases in `packages/md4x/test/_suite.mjs` for JSON/AST output

### Phase 2: Heading IDs

- [ ] **2.1** Generate heading IDs in JSON renderer
  - Slugify heading text content: lowercase, replace spaces with `-`, strip non-alphanumeric
  - Emit `"id":"slug"` in JSON props for `h1`–`h6` nodes
  - Handle duplicates by appending `-1`, `-2`, etc.

- [ ] **2.2** Add test cases for heading IDs
  - JS test cases for `parseAST` verifying `id` prop on headings

### Phase 3: Inline Components (parser-level)

- [ ] **3.1** Add `MD_SPAN_COMPONENT` to `MD_SPANTYPE` enum and detail struct
  - `MD_SPAN_COMPONENT` with `MD_SPAN_COMPONENT_DETAIL` (tag name, raw props string)
  - Add `MD_FLAG_COMPONENTS` flag (e.g. `0x20000`)
  - Update `MD_DIALECT_ALL` to include it

- [ ] **3.2** Implement inline component parsing in `md4x.c`
  - Recognize `:component-name` — alphanumeric + hyphens after `:`
  - Must not conflict with emoji shortcodes (`:smile:` — has closing `:`)
  - Parse optional `[content]` and `{props}` after component name
  - Emit `enter_span(MD_SPAN_COMPONENT)` / `leave_span(MD_SPAN_COMPONENT)`

- [ ] **3.3** Handle inline components in JSON renderer
  - Map `MD_SPAN_COMPONENT` to dynamic tag name from detail
  - Parse props string into individual `key="value"`, `bool`, `#id`, `.class` attributes
  - Emit as `["component-name", {props}, ...children]`

- [ ] **3.4** Handle inline components in HTML renderer
  - Render as `<component-name ...props>content</component-name>`

- [ ] **3.5** Add test cases for inline components
  - `test/spec-components.txt` — basic inline component tests
  - JS test cases for AST output

### Phase 4: Block Components (parser-level)

- [ ] **4.1** Add `MD_BLOCK_COMPONENT` to `MD_BLOCKTYPE` enum and detail struct
  - `MD_BLOCK_COMPONENT_DETAIL` (tag name, raw props string, nesting level / colon count)

- [ ] **4.2** Implement block component parsing in `md4x.c`
  - Recognize `::component-name{props}` as opening, `::` as closing
  - Handle nesting via colon count: `::` < `:::` < `::::` etc.
  - Content between open/close is parsed as normal markdown blocks
  - Empty components: `::divider\n::`

- [ ] **4.3** Implement component slots parsing
  - Inside a block component, `#slot-name` at line start creates a named slot
  - Content after `#slot-name` until next `#slot` or `::` closing is the slot body
  - Emit as `["template", {"name": "slot-name"}, ...children]` in JSON

- [ ] **4.4** Handle block components in JSON renderer
  - Map `MD_BLOCK_COMPONENT` to dynamic tag name
  - Parse and emit props (reuse prop parser from Phase 3)

- [ ] **4.5** Handle block components in HTML renderer
  - Render as `<component-name ...props>content</component-name>`

- [ ] **4.6** Add test cases for block components
  - Add to `test/spec-components.txt` — block components, nesting, slots
  - JS test cases for AST output

### Phase 5: Component Property Parser

- [ ] **5.1** Implement shared property parser utility
  - Parse `{prop="value" bool #id .class-name :data='{"key":"val"}'}` syntax
  - Handle string values (double/single quoted)
  - Handle boolean (bare word → `:word="true"` in AST)
  - Handle `#id` → `id` prop
  - Handle `.class` → `class` prop (merge multiple)
  - Handle `:key='value'` → `:key` prop (JSON passthrough)
  - This can be a shared C function used by both inline/block component renderers

- [ ] **5.2** Add test cases for property parsing edge cases

### Phase 6: Attributes on Native Elements

- [ ] **6.1** Implement attribute parsing for inline elements in parser
  - After `**bold**`, `*italic*`, `[link](url)`, `![img](src)`, `` `code` `` — check for `{...}`
  - Parse attributes using shared prop parser
  - Extend `MD_SPAN_*_DETAIL` structs or add generic attribute field

- [ ] **6.2** Handle inline element attributes in JSON renderer
  - Merge parsed attributes into element props

- [ ] **6.3** Implement span attribute syntax: `[text]{.class}`
  - When `[text]` is not followed by `(url)` but by `{attrs}`, treat as styled span
  - Emit as `["span", {attrs}, "text"]` in JSON

- [ ] **6.4** Add test cases for native element attributes
  - `test/spec-attributes.txt`
  - JS test cases

### Phase 7: Emoji Shortcodes

- [ ] **7.1** Add emoji lookup table
  - Generate from GitHub/Unicode emoji list or use a static subset
  - Map `:smile:` → `😄`, `:rocket:` → `🚀`, etc.

- [ ] **7.2** Implement emoji shortcode recognition in parser or renderers
  - Recognize `:name:` pattern (colon + alpha/underscore + colon)
  - Must not conflict with inline components (`:component-name` has no closing `:`)
  - Replace with UTF-8 emoji in text output

- [ ] **7.3** Add test cases for emoji shortcodes

### Phase 8: ANSI Renderer Updates

- [ ] **8.1** Handle new block/span types in ANSI renderer
  - Components → render as styled blocks/spans
  - Attributes → ignore or apply terminal-compatible attributes
  - Emojis → pass through (terminal handles them)

### Phase 9: Integration & Cleanup

- [ ] **9.1** Update `MD_DIALECT_ALL` to include all new flags
- [ ] **9.2** Update WASM exports if needed
- [ ] **9.3** Update NAPI bindings if needed
- [ ] **9.4** Run full test suite and fix regressions
- [ ] **9.5** Update `AGENTS.md` with new types, flags, and API docs
- [ ] **9.6** Update `CHANGELOG.md`

---

## Architecture Notes

- **Parser changes** (`md4x.c`, `md4x.h`) are needed for: inline components, block components, attributes on native elements, emoji shortcodes
- **Renderer-only changes** (`md4x-json.c`) are sufficient for: code block metadata parsing, heading IDs (these work from existing info already provided by the parser)
- **Shared prop parser** should be a reusable C function in a new file or within the JSON renderer
- The property syntax `{...}` is shared between components and native element attributes
- Inline components (`:name`) vs emoji shortcodes (`:name:`) are disambiguated by the trailing `:`
