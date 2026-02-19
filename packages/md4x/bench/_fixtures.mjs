export const small =
  "# Hello\n\nA paragraph with **bold** and *italic* text.\n";

export const medium = `# Document Title

A paragraph with **bold**, *italic*, \`code\`, and [a link](https://example.com).

## Section 1

- Item 1
- Item 2
  - Nested item
- Item 3

> Blockquote with **emphasis** inside.

## Section 2

\`\`\`js
function hello() {
  return "world";
}
\`\`\`

| Column A | Column B | Column C |
|----------|:--------:|---------:|
| left     | center   | right    |
| foo      | bar      | baz      |

---

1. First
2. Second
3. Third

Final paragraph with ~~strikethrough~~ and \`inline code\`.
`;

export const large = medium.repeat(50);
