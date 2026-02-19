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
2. **Inline components** — `:component-name`, `:component[content]{props}`
3. **Block components** — `::component-name{props}...::`, nesting `:::`, `::::`
4. **Component slots** — `#slot-name` inside block components
5. **Component properties** — `{prop="value" bool #id .class}` on components
6. **Attributes on native elements** — `**bold**{.class}`, `[link](url){target="_blank"}`
7. **Span attributes** — `[text]{.class}` creating styled spans

---

> **Architecture rule:** All syntax features (parsing, metadata extraction) belong in the **core parser** (`md4x.c` / `md4x.h`). Renderers consume parsed data via detail structs — they should NOT re-parse info strings or do their own syntax analysis.

## Implementation Steps

### Phase 1: Code Block Metadata (core parser + renderers)

Extend `MD_BLOCK_CODE_DETAIL` in the core parser to parse filename, highlights, and meta from the info string. Renderers then read these fields from the detail struct.

- [x] **1.1** Extend `MD_BLOCK_CODE_DETAIL` in `md4x.h` with `filename`, `highlights`, `highlight_count`, `meta` fields
- [x] **1.2** Implement info string parsing in `md4x.c` — extract `[filename]`, `{highlights}`, and remaining meta
  - Handle escaped chars in filename: `[@[...slug\].ts]`
  - Parse highlight ranges: `1-3,5,7` → expanded array
- [x] **1.3** Update JSON renderer to emit new fields from detail struct (no parsing in renderer)
- [x] **1.4** HTML/ANSI renderers — no changes needed (metadata is structural, consumed via JSON/AST)
- [x] **1.5** Add test cases for code block metadata
  - JS test cases in `packages/md4x/test/_suite.mjs` for JSON/AST output (5 new tests, WASM+NAPI)

### Phase 2: Inline Components (parser-level)

- [x] **2.1** Add `MD_SPAN_COMPONENT` to `MD_SPANTYPE` enum and detail struct
  - `MD_SPAN_COMPONENT` with `MD_SPAN_COMPONENT_DETAIL` (tag name, raw props string)
  - Add `MD_FLAG_COMPONENTS` flag (`0x20000`)
  - Update `MD_DIALECT_ALL` to include it

- [x] **2.2** Implement inline component parsing in `md4x.c`
  - Recognize `:component-name` — alphanumeric + hyphens after `:`
  - Parse optional `[content]` and `{props}` after component name
  - Emit `enter_span(MD_SPAN_COMPONENT)` / `leave_span(MD_SPAN_COMPONENT)`
  - Inline markdown parsed inside `[content]` (emphasis, links, etc.)
  - Standalone components (no `[content]` or `{props}`) require a hyphen in the name
  - `:` must not be preceded by alphanumeric to avoid URL conflicts

- [x] **2.3** Handle inline components in JSON renderer
  - Map `MD_SPAN_COMPONENT` to dynamic tag name from detail
  - Parse props string into individual `key="value"`, `bool`, `#id`, `.class` attributes
  - Emit as `["component-name", {props}, ...children]`

- [x] **2.4** Handle inline components in HTML renderer
  - Render as `<component-name ...props>content</component-name>`

- [x] **2.5** Handle inline components in ANSI renderer
  - Render with cyan color styling

- [x] **2.6** Add test cases for inline components
  - `test/spec-components.txt` — 12 test cases
  - JS test cases in `_suite.mjs` — 10 new tests (HTML + AST)

### Phase 3: Block Components (parser-level)

- [ ] **3.1** Add `MD_BLOCK_COMPONENT` to `MD_BLOCKTYPE` enum and detail struct
  - `MD_BLOCK_COMPONENT_DETAIL` (tag name, raw props string, nesting level / colon count)

- [ ] **3.2** Implement block component parsing in `md4x.c`
  - Recognize `::component-name{props}` as opening, `::` as closing
  - Handle nesting via colon count: `::` < `:::` < `::::` etc.
  - Content between open/close is parsed as normal markdown blocks
  - Empty components: `::divider\n::`

- [ ] **3.3** Implement component slots parsing
  - Inside a block component, `#slot-name` at line start creates a named slot
  - Content after `#slot-name` until next `#slot` or `::` closing is the slot body
  - Emit as `["template", {"name": "slot-name"}, ...children]` in JSON

- [ ] **3.4** Handle block components in JSON renderer
  - Map `MD_BLOCK_COMPONENT` to dynamic tag name
  - Parse and emit props (reuse prop parser from Phase 4)

- [ ] **3.5** Handle block components in HTML renderer
  - Render as `<component-name ...props>content</component-name>`

- [ ] **3.6** Add test cases for block components
  - Add to `test/spec-components.txt` — block components, nesting, slots
  - JS test cases for AST output

### Phase 4: Component Property Parser

- [ ] **4.1** Implement shared property parser utility
  - Parse `{prop="value" bool #id .class-name :data='{"key":"val"}'}` syntax
  - Handle string values (double/single quoted)
  - Handle boolean (bare word → `:word="true"` in AST)
  - Handle `#id` → `id` prop
  - Handle `.class` → `class` prop (merge multiple)
  - Handle `:key='value'` → `:key` prop (JSON passthrough)
  - This can be a shared C function used by both inline/block component renderers

- [ ] **4.2** Add test cases for property parsing edge cases

### Phase 5: Attributes on Native Elements

- [ ] **5.1** Implement attribute parsing for inline elements in parser
  - After `**bold**`, `*italic*`, `[link](url)`, `![img](src)`, `` `code` `` — check for `{...}`
  - Parse attributes using shared prop parser
  - Extend `MD_SPAN_*_DETAIL` structs or add generic attribute field

- [ ] **5.2** Handle inline element attributes in JSON renderer
  - Merge parsed attributes into element props

- [ ] **5.3** Implement span attribute syntax: `[text]{.class}`
  - When `[text]` is not followed by `(url)` but by `{attrs}`, treat as styled span
  - Emit as `["span", {attrs}, "text"]` in JSON

- [ ] **5.4** Add test cases for native element attributes
  - `test/spec-attributes.txt`
  - JS test cases

### Phase 6: ANSI Renderer Updates

- [ ] **6.1** Handle new block/span types in ANSI renderer
  - Components → render as styled blocks/spans
  - Attributes → ignore or apply terminal-compatible attributes

### Phase 7: Integration & Cleanup

- [ ] **7.1** Update `MD_DIALECT_ALL` to include all new flags
- [ ] **7.2** Update WASM exports if needed
- [ ] **7.3** Update NAPI bindings if needed
- [ ] **7.4** Run full test suite and fix regressions
- [ ] **7.5** Update `AGENTS.md` with new types, flags, and API docs
- [ ] **7.6** Update `CHANGELOG.md`

---

## Architecture Notes

- **Parser changes** (`md4x.c`, `md4x.h`) are needed for: inline components, block components, attributes on native elements
- **Renderer-only changes** (`md4x-json.c`) are sufficient for: code block metadata parsing (works from existing info already provided by the parser)
- **Shared prop parser** should be a reusable C function in a new file or within the JSON renderer
- The property syntax `{...}` is shared between components and native element attributes
