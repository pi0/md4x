import { describe, it, expect } from "vitest";

export function defineSuite({ renderToHtml, renderToJson, renderToAnsi }) {
  describe("renderToHtml", () => {
    it("renders a heading", async () => {
      expect(await renderToHtml("# Hello")).toBe("<h1>Hello</h1>\n");
    });

    it("renders a paragraph", async () => {
      expect(await renderToHtml("Hello world")).toBe("<p>Hello world</p>\n");
    });

    it("renders inline formatting", async () => {
      expect(await renderToHtml("**bold** and *italic*")).toBe(
        "<p><strong>bold</strong> and <em>italic</em></p>\n",
      );
    });

    it("renders a link", async () => {
      expect(await renderToHtml("[click](https://example.com)")).toBe(
        '<p><a href="https://example.com">click</a></p>\n',
      );
    });

    it("renders a code block", async () => {
      expect(await renderToHtml("```js\nconsole.log(1)\n```")).toBe(
        '<pre><code class="language-js">console.log(1)\n</code></pre>\n',
      );
    });

    it("renders empty input", async () => {
      expect(await renderToHtml("")).toBe("");
    });

    it("renders multiline content", async () => {
      const html = await renderToHtml("# Title\n\nParagraph\n\n- item");
      expect(html).toContain("<h1>Title</h1>");
      expect(html).toContain("<p>Paragraph</p>");
      expect(html).toContain("<li>item</li>");
    });

    it("supports tables", async () => {
      const html = await renderToHtml("| a | b |\n|---|---|\n| 1 | 2 |");
      expect(html).toContain("<table>");
      expect(html).toContain("<td>1</td>");
    });

    it("supports strikethrough", async () => {
      expect(await renderToHtml("~~strike~~")).toContain("<del>strike</del>");
    });

    it("supports task lists", async () => {
      expect(await renderToHtml("- [x] done\n- [ ] todo")).toContain(
        'type="checkbox"',
      );
    });

    it("supports latex math", async () => {
      expect(await renderToHtml("$E=mc^2$")).toContain("E=mc^2");
    });
  });

  describe("renderToJson", () => {
    it("returns valid JSON", async () => {
      const ast = await renderToJson("# Hello");
      expect(ast.type).toBe("document");
      expect(ast.children).toBeInstanceOf(Array);
    });

    it("parses heading structure", async () => {
      const ast = await renderToJson("# Hello");
      const heading = ast.children[0];
      expect(heading.type).toBe("heading");
      expect(heading.level).toBe(1);
    });

    it("parses paragraph with inline", async () => {
      const ast = await renderToJson("**bold**");
      const para = ast.children[0];
      expect(para.type).toBe("paragraph");
      const strong = para.children[0];
      expect(strong.type).toBe("strong");
    });

    it("parses code block", async () => {
      const ast = await renderToJson("```js\ncode\n```");
      const code = ast.children[0];
      expect(code.type).toBe("code_block");
      expect(code.info).toBe("js");
      expect(code.literal).toContain("code");
    });

    it("parses empty input", async () => {
      const ast = await renderToJson("");
      expect(ast.type).toBe("document");
      expect(ast.children).toHaveLength(0);
    });
  });

  describe("renderToAnsi", () => {
    it("renders heading with ansi codes", async () => {
      expect(await renderToAnsi("# Hello")).toContain("Hello");
    });

    it("renders empty input", async () => {
      expect(await renderToAnsi("")).toBe("");
    });
  });
}
