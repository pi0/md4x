import { describe, it, expect } from "vitest";

export function defineSuite({ renderToHtml, renderToJson, renderToAnsi, parseAST }) {
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

    it("renders inline component", async () => {
      expect(await renderToHtml(":icon-star")).toContain("<icon-star>");
    });

    it("renders inline component with content", async () => {
      const html = await renderToHtml(":badge[New]");
      expect(html).toContain("<badge>New</badge>");
    });

    it("renders inline component with props", async () => {
      const html = await renderToHtml(':badge[New]{color="blue"}');
      expect(html).toContain('color="blue"');
      expect(html).toContain("<badge");
      expect(html).toContain("New</badge>");
    });
  });

  describe("renderToJson", () => {
    it("returns a string", async () => {
      const json = await renderToJson("# Hello");
      expect(typeof json).toBe("string");
    });

    it("returns valid JSON", async () => {
      const json = await renderToJson("# Hello");
      const parsed = JSON.parse(json);
      expect(parsed.type).toBe("comark");
    });

    it("returns empty string for empty input", async () => {
      const json = await renderToJson("");
      const parsed = JSON.parse(json);
      expect(parsed.value).toHaveLength(0);
    });
  });

  describe("parseAST", () => {
    it("returns comark tree", async () => {
      const ast = await parseAST("# Hello");
      expect(ast.type).toBe("comark");
      expect(ast.value).toBeInstanceOf(Array);
    });

    it("parses heading as h1 tuple", async () => {
      const ast = await parseAST("# Hello");
      const h1 = ast.value[0];
      expect(h1[0]).toBe("h1");
      expect(h1[1]).toEqual({});
      expect(h1[2]).toBe("Hello");
    });

    it("parses paragraph with inline formatting", async () => {
      const ast = await parseAST("**bold**");
      const p = ast.value[0];
      expect(p[0]).toBe("p");
      const strong = p[2];
      expect(strong[0]).toBe("strong");
      expect(strong[2]).toBe("bold");
    });

    it("parses code block as pre > code", async () => {
      const ast = await parseAST("```js\ncode\n```");
      const pre = ast.value[0];
      expect(pre[0]).toBe("pre");
      expect(pre[1].language).toBe("js");
      const code = pre[2];
      expect(code[0]).toBe("code");
      expect(code[1].class).toBe("language-js");
      expect(code[2]).toContain("code");
    });

    it("parses empty input", async () => {
      const ast = await parseAST("");
      expect(ast.type).toBe("comark");
      expect(ast.value).toHaveLength(0);
    });

    it("parses link as anchor tuple", async () => {
      const ast = await parseAST("[text](https://example.com)");
      const p = ast.value[0];
      const a = p[2];
      expect(a[0]).toBe("a");
      expect(a[1].href).toBe("https://example.com");
      expect(a[2]).toBe("text");
    });

    it("parses inline elements", async () => {
      const ast = await parseAST("hello **world**");
      const p = ast.value[0];
      expect(p[0]).toBe("p");
      expect(p[2]).toBe("hello ");
      expect(p[3][0]).toBe("strong");
      expect(p[3][2]).toBe("world");
    });

    it("text nodes are plain strings", async () => {
      const ast = await parseAST("hello");
      const p = ast.value[0];
      expect(typeof p[2]).toBe("string");
      expect(p[2]).toBe("hello");
    });

    it("parses code block filename", async () => {
      const ast = await parseAST("```js [app.js]\nconsole.log(1)\n```");
      const pre = ast.value[0];
      expect(pre[0]).toBe("pre");
      expect(pre[1].language).toBe("js");
      expect(pre[1].filename).toBe("app.js");
    });

    it("parses code block highlights", async () => {
      const ast = await parseAST("```js {1-3,5}\na\nb\nc\nd\ne\n```");
      const pre = ast.value[0];
      expect(pre[1].highlights).toEqual([1, 2, 3, 5]);
    });

    it("parses code block with filename, highlights and meta", async () => {
      const ast = await parseAST("```ts {1-2} [utils.ts] meta=value\ncode\n```");
      const pre = ast.value[0];
      expect(pre[1].language).toBe("ts");
      expect(pre[1].filename).toBe("utils.ts");
      expect(pre[1].highlights).toEqual([1, 2]);
      expect(pre[1].meta).toBe("meta=value");
    });

    it("parses code block with escaped filename", async () => {
      const ast = await parseAST("```ts [@[...slug\\].ts]\ncode\n```");
      const pre = ast.value[0];
      expect(pre[1].filename).toBe("@[...slug].ts");
    });

    it("code block without metadata has no extra props", async () => {
      const ast = await parseAST("```js\ncode\n```");
      const pre = ast.value[0];
      expect(pre[1].filename).toBeUndefined();
      expect(pre[1].highlights).toBeUndefined();
      expect(pre[1].meta).toBeUndefined();
    });

    it("parses standalone inline component", async () => {
      const ast = await parseAST(":icon-star");
      const p = ast.value[0];
      expect(p[0]).toBe("p");
      const comp = p[2];
      expect(comp[0]).toBe("icon-star");
      expect(comp[1]).toEqual({});
    });

    it("parses inline component with content", async () => {
      const ast = await parseAST(":badge[New]");
      const p = ast.value[0];
      const comp = p[2];
      expect(comp[0]).toBe("badge");
      expect(comp[2]).toBe("New");
    });

    it("parses inline component with content and props", async () => {
      const ast = await parseAST(':badge[New]{color="blue"}');
      const p = ast.value[0];
      const comp = p[2];
      expect(comp[0]).toBe("badge");
      expect(comp[1].color).toBe("blue");
      expect(comp[2]).toBe("New");
    });

    it("parses inline component with props only", async () => {
      const ast = await parseAST(':tooltip{text="Hover"}');
      const p = ast.value[0];
      const comp = p[2];
      expect(comp[0]).toBe("tooltip");
      expect(comp[1].text).toBe("Hover");
    });

    it("parses inline component with id and class props", async () => {
      const ast = await parseAST(":badge[Text]{#my-id .highlight}");
      const p = ast.value[0];
      const comp = p[2];
      expect(comp[0]).toBe("badge");
      expect(comp[1].id).toBe("my-id");
      expect(comp[1].class).toBe("highlight");
      expect(comp[2]).toBe("Text");
    });

    it("parses inline component with boolean prop", async () => {
      const ast = await parseAST(":alert{dismissible}");
      const p = ast.value[0];
      const comp = p[2];
      expect(comp[0]).toBe("alert");
      expect(comp[1].dismissible).toBe(true);
    });

    it("inline component with markdown content", async () => {
      const ast = await parseAST(":badge[**bold** text]");
      const p = ast.value[0];
      const comp = p[2];
      expect(comp[0]).toBe("badge");
      // First child is strong element
      expect(comp[2][0]).toBe("strong");
      expect(comp[2][2]).toBe("bold");
      // Second child is text
      expect(comp[3]).toBe(" text");
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
