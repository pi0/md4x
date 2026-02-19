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

  describe("block components", () => {
    it("renders block component HTML", async () => {
      const html = await renderToHtml("::alert\nHello world\n::");
      expect(html).toContain("<alert>");
      expect(html).toContain("<p>Hello world</p>");
      expect(html).toContain("</alert>");
    });

    it("renders block component with props HTML", async () => {
      const html = await renderToHtml('::alert{type="info"}\nMessage\n::');
      expect(html).toContain('<alert type="info">');
      expect(html).toContain("</alert>");
    });

    it("parses block component AST", async () => {
      const ast = await parseAST("::alert\nHello\n::");
      const comp = ast.value[0];
      expect(comp[0]).toBe("alert");
      expect(comp[1]).toEqual({});
      // Child is a paragraph
      const p = comp[2];
      expect(p[0]).toBe("p");
      expect(p[2]).toBe("Hello");
    });

    it("parses block component with props AST", async () => {
      const ast = await parseAST('::alert{type="info"}\nMessage\n::');
      const comp = ast.value[0];
      expect(comp[0]).toBe("alert");
      expect(comp[1].type).toBe("info");
    });

    it("parses nested block components AST", async () => {
      const ast = await parseAST(":::outer\nOuter\n\n::inner\nInner\n::\n\n:::");
      const outer = ast.value[0];
      expect(outer[0]).toBe("outer");
      // First child: paragraph "Outer"
      expect(outer[2][0]).toBe("p");
      expect(outer[2][2]).toBe("Outer");
      // Second child: inner component
      const inner = outer[3];
      expect(inner[0]).toBe("inner");
      expect(inner[2][0]).toBe("p");
      expect(inner[2][2]).toBe("Inner");
    });

    it("parses empty block component AST", async () => {
      const ast = await parseAST("::divider\n::");
      const comp = ast.value[0];
      expect(comp[0]).toBe("divider");
      expect(comp.length).toBe(2); // [tag, props] with no children
    });

    it("block component with markdown content AST", async () => {
      const ast = await parseAST("::card\n# Title\n\nParagraph\n::");
      const card = ast.value[0];
      expect(card[0]).toBe("card");
      expect(card[2][0]).toBe("h1");
      expect(card[2][2]).toBe("Title");
      expect(card[3][0]).toBe("p");
      expect(card[3][2]).toBe("Paragraph");
    });

    it("parses block component with id and class AST", async () => {
      const ast = await parseAST('::alert{#my-id .highlight}\nText\n::');
      const comp = ast.value[0];
      expect(comp[0]).toBe("alert");
      expect(comp[1].id).toBe("my-id");
      expect(comp[1].class).toBe("highlight");
    });

    it("parses block component with boolean prop AST", async () => {
      const ast = await parseAST("::alert{dismissible}\nText\n::");
      const comp = ast.value[0];
      expect(comp[0]).toBe("alert");
      expect(comp[1].dismissible).toBe(true);
    });
  });

  describe("component property parsing", () => {
    it("merges multiple classes", async () => {
      const ast = await parseAST(":badge[Text]{.foo .bar .baz}");
      const comp = ast.value[0][2];
      expect(comp[0]).toBe("badge");
      expect(comp[1].class).toBe("foo bar baz");
    });

    it("merges multiple classes on block component", async () => {
      const ast = await parseAST("::alert{.warning .large}\nMsg\n::");
      const comp = ast.value[0];
      expect(comp[0]).toBe("alert");
      expect(comp[1].class).toBe("warning large");
    });

    it("handles single-quoted string values", async () => {
      const ast = await parseAST(":badge[Text]{color='blue'}");
      const comp = ast.value[0][2];
      expect(comp[1].color).toBe("blue");
    });

    it("handles mixed props with id, classes, key-value, and boolean", async () => {
      const ast = await parseAST(':badge[T]{#myid .cls1 .cls2 key="val" flag}');
      const comp = ast.value[0][2];
      expect(comp[1].id).toBe("myid");
      expect(comp[1].class).toBe("cls1 cls2");
      expect(comp[1].key).toBe("val");
      expect(comp[1].flag).toBe(true);
    });

    it("handles empty props object", async () => {
      const ast = await parseAST(":badge[Text]{}");
      const comp = ast.value[0][2];
      expect(comp[0]).toBe("badge");
      expect(comp[1]).toEqual({});
    });

    it("handles bind syntax in props", async () => {
      const ast = await parseAST(":widget{:data='{\"x\":1}'}");
      const comp = ast.value[0][2];
      expect(comp[0]).toBe("widget");
      expect(comp[1][":data"]).toEqual({ x: 1 });
    });

    it("renders merged classes in HTML", async () => {
      const html = await renderToHtml(":badge[Text]{.foo .bar .baz}");
      expect(html).toContain('class="foo bar baz"');
    });

    it("renders mixed props in HTML", async () => {
      const html = await renderToHtml(':badge[T]{#myid .cls1 .cls2 key="val" flag}');
      expect(html).toContain('id="myid"');
      expect(html).toContain('class="cls1 cls2"');
      expect(html).toContain('key="val"');
      expect(html).toContain("flag");
    });
  });

  describe("inline attributes", () => {
    it("renders strong with class HTML", async () => {
      const html = await renderToHtml("**bold**{.highlight}");
      expect(html).toContain('<strong class="highlight">bold</strong>');
    });

    it("renders emphasis with id HTML", async () => {
      const html = await renderToHtml("*italic*{#myid}");
      expect(html).toContain('<em id="myid">italic</em>');
    });

    it("renders code span with class HTML", async () => {
      const html = await renderToHtml("`code`{.lang}");
      expect(html).toContain('<code class="lang">code</code>');
    });

    it("renders link with target HTML", async () => {
      const html = await renderToHtml('[Link](https://example.com){target="_blank"}');
      expect(html).toContain('target="_blank"');
      expect(html).toContain('<a href="https://example.com"');
    });

    it("renders image with class HTML", async () => {
      const html = await renderToHtml("![alt](img.png){.responsive}");
      expect(html).toContain('<img src="img.png" alt="alt" class="responsive">');
    });

    it("renders span syntax HTML", async () => {
      const html = await renderToHtml("[text]{.class}");
      expect(html).toContain('<span class="class">text</span>');
    });

    it("renders span with mixed attrs HTML", async () => {
      const html = await renderToHtml('[text]{#myid .cls key="val"}');
      expect(html).toContain('id="myid"');
      expect(html).toContain('class="cls"');
      expect(html).toContain('key="val"');
      expect(html).toContain("<span");
    });

    it("parses strong with attrs AST", async () => {
      const ast = await parseAST("**bold**{.highlight}");
      const p = ast.value[0];
      const strong = p[2];
      expect(strong[0]).toBe("strong");
      expect(strong[1].class).toBe("highlight");
      expect(strong[2]).toBe("bold");
    });

    it("parses emphasis with attrs AST", async () => {
      const ast = await parseAST("*italic*{#myid}");
      const p = ast.value[0];
      const em = p[2];
      expect(em[0]).toBe("em");
      expect(em[1].id).toBe("myid");
      expect(em[2]).toBe("italic");
    });

    it("parses code span with attrs AST", async () => {
      const ast = await parseAST("`code`{.lang}");
      const p = ast.value[0];
      const code = p[2];
      expect(code[0]).toBe("code");
      expect(code[1].class).toBe("lang");
      expect(code[2]).toBe("code");
    });

    it("parses link with attrs AST", async () => {
      const ast = await parseAST('[Link](https://example.com){target="_blank"}');
      const p = ast.value[0];
      const a = p[2];
      expect(a[0]).toBe("a");
      expect(a[1].href).toBe("https://example.com");
      expect(a[1].target).toBe("_blank");
      expect(a[2]).toBe("Link");
    });

    it("parses image with attrs AST", async () => {
      const ast = await parseAST("![alt](img.png){.responsive}");
      const p = ast.value[0];
      const img = p[2];
      expect(img[0]).toBe("img");
      expect(img[1].src).toBe("img.png");
      expect(img[1].class).toBe("responsive");
    });

    it("parses span syntax AST", async () => {
      const ast = await parseAST("[text]{.class}");
      const p = ast.value[0];
      const span = p[2];
      expect(span[0]).toBe("span");
      expect(span[1].class).toBe("class");
      expect(span[2]).toBe("text");
    });

    it("parses span with multiple classes AST", async () => {
      const ast = await parseAST("[text]{.foo .bar .baz}");
      const p = ast.value[0];
      const span = p[2];
      expect(span[0]).toBe("span");
      expect(span[1].class).toBe("foo bar baz");
    });

    it("span with inline markdown AST", async () => {
      const ast = await parseAST("[**bold** text]{.styled}");
      const p = ast.value[0];
      const span = p[2];
      expect(span[0]).toBe("span");
      expect(span[1].class).toBe("styled");
      expect(span[2][0]).toBe("strong");
      expect(span[2][2]).toBe("bold");
      expect(span[3]).toBe(" text");
    });

    it("element without attrs has no extra props", async () => {
      const ast = await parseAST("**bold**");
      const p = ast.value[0];
      const strong = p[2];
      expect(strong[0]).toBe("strong");
      expect(strong[1]).toEqual({});
    });

    it("empty attrs have no effect AST", async () => {
      const ast = await parseAST("**bold**{}");
      const p = ast.value[0];
      const strong = p[2];
      expect(strong[0]).toBe("strong");
      expect(strong[1]).toEqual({});
    });

    it("plain text followed by braces is not attrs", async () => {
      const html = await renderToHtml("hello{.class}");
      expect(html).toContain("hello{.class}");
      expect(html).not.toContain("<span");
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
