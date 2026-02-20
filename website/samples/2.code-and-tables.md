# Code & Tables

## Inline Code

Use `const x = 42;` for inline code snippets.

## Fenced Code Block

```js
function greet(name) {
  return `Hello, ${name}!`;
}
```

## Indented Code Block

    four spaces
    indented code

## Code Block Metadata

Code with filename and highlighted lines:

```js [greet.js] {1,3}
function greet(name) {
  return `Hello, ${name}!`;
}
```

```ts [server.ts] {2-4}
import { serve } from "bun";

serve({
  port: 3000,
  fetch(req) {
    return new Response("Hello!");
  },
});
```

## Tables

| Feature       | Status |              Notes |
| :------------ | :----: | -----------------: |
| CommonMark    |   ✓    |        Spec 0.31.2 |
| Tables        |   ✓    |  Alignment support |
| Strikethrough |   ✓    |        `~` or `~~` |
| Task Lists    |   ✓    |      `[x]` / `[ ]` |
| LaTeX Math    |   ✓    | Inline and display |
| Wiki Links    |   ✓    |       `[[target]]` |
| Underline     |   ✓    |         `__text__` |
| Frontmatter   |   ✓    |         YAML `---` |
| Components    |   ✓    |     Inline + block |
| Attributes    |   ✓    |     `{.class #id}` |
