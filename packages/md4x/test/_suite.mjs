import { describe, it, expect } from "vitest";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));
const nitroIndex = readFileSync(
  join(__dirname, "fixtures/nitro-index.md"),
  "utf-8",
);

export function defineSuite({
  renderToHtml,
  renderToAST,
  renderToAnsi,
  parseAST,
  renderToMeta,
  parseMeta,
  renderToText,
}) {
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

    it("renders bold and italic combined", async () => {
      const html = await renderToHtml("***Bold and italic***");
      expect(html).toContain("<strong>");
      expect(html).toContain("<em>");
      expect(html).toContain("Bold and italic");
    });

    it("renders a basic image", async () => {
      const html = await renderToHtml("![Alt text](image.png)");
      expect(html).toContain('<img src="image.png" alt="Alt text"');
    });

    it("renders image with title", async () => {
      const html = await renderToHtml('![Alt](image.png "Image title")');
      expect(html).toContain('title="Image title"');
      expect(html).toContain('src="image.png"');
    });

    it("renders link with title", async () => {
      const html = await renderToHtml(
        '[Link](https://example.com "Link title")',
      );
      expect(html).toContain('title="Link title"');
      expect(html).toContain('href="https://example.com"');
    });

    it("renders a blockquote", async () => {
      const html = await renderToHtml("> This is a blockquote");
      expect(html).toContain("<blockquote>");
      expect(html).toContain("This is a blockquote");
    });

    it("renders horizontal rule", async () => {
      expect(await renderToHtml("***")).toContain("<hr");
    });

    it("renders ordered list", async () => {
      const html = await renderToHtml("1. First\n2. Second\n3. Third");
      expect(html).toContain("<ol>");
      expect(html).toContain("<li>First</li>");
    });

    it("renders unordered list with nesting", async () => {
      const html = await renderToHtml("- Item 1\n  - Nested\n- Item 2");
      expect(html).toContain("<ul>");
      expect(html).toContain("Nested");
    });

    it("renders frontmatter", async () => {
      const html = await renderToHtml("---\ntitle: Test\n---\n\n# Content");
      expect(html).not.toContain("<x-frontmatter>");
      expect(html).not.toContain("title: Test");
      expect(html).toContain("<h1>Content</h1>");
    });

    it("renders aligned table", async () => {
      const html = await renderToHtml(
        "| Left | Right |\n|:-----|------:|\n| a    | b     |",
      );
      expect(html).toContain("<table>");
      expect(html).toContain("align");
    });

    it("renders inline markdown in table cells", async () => {
      const html = await renderToHtml("| a |\n|---|\n| **bold** |");
      expect(html).toContain("<strong>bold</strong>");
      expect(html).toContain("<td>");
    });

    it("renders deep nested block components", async () => {
      const html = await renderToHtml(
        "::::outer\n\n:::middle\n\n::inner\nContent\n::\n\n:::\n\n::::",
      );
      expect(html).toContain("<outer>");
      expect(html).toContain("<middle>");
      expect(html).toContain("<inner>");
      expect(html).toContain("Content");
    });
  });

  describe("renderToAST", () => {
    it("returns a string", async () => {
      const json = await renderToAST("# Hello");
      expect(typeof json).toBe("string");
    });

    it("returns valid JSON", async () => {
      const json = await renderToAST("# Hello");
      const parsed = JSON.parse(json);
      expect(parsed.type).toBe("comark");
    });

    it("returns empty string for empty input", async () => {
      const json = await renderToAST("");
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
      const ast = await parseAST(
        "```ts {1-2} [utils.ts] meta=value\ncode\n```",
      );
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

    it("parses basic image AST", async () => {
      const ast = await parseAST("![Alt text](image.png)");
      const p = ast.value[0];
      const img = p[2];
      expect(img[0]).toBe("img");
      expect(img[1].src).toBe("image.png");
      expect(img[1].alt).toBe("Alt text");
    });

    it("parses link with title AST", async () => {
      const ast = await parseAST('[Link](https://example.com "title")');
      const p = ast.value[0];
      const a = p[2];
      expect(a[0]).toBe("a");
      expect(a[1].href).toBe("https://example.com");
      expect(a[1].title).toBe("title");
    });

    it("parses blockquote AST", async () => {
      const ast = await parseAST("> Quote text");
      const bq = ast.value[0];
      expect(bq[0]).toBe("blockquote");
    });

    it("parses horizontal rule AST", async () => {
      const ast = await parseAST("***");
      const hr = ast.value[0];
      expect(hr[0]).toBe("hr");
    });

    it("parses ordered list AST", async () => {
      const ast = await parseAST("1. First\n2. Second");
      const ol = ast.value[0];
      expect(ol[0]).toBe("ol");
    });

    it("parses unordered list AST", async () => {
      const ast = await parseAST("- Item 1\n- Item 2");
      const ul = ast.value[0];
      expect(ul[0]).toBe("ul");
    });

    it("parses frontmatter AST with YAML props", async () => {
      const ast = await parseAST(
        "---\ntitle: Hello\ncount: 42\ndraft: true\n---\n\n# Content",
      );
      const fm = ast.value[0];
      expect(fm[0]).toBe("frontmatter");
      expect(fm[1]).toEqual({ title: "Hello", count: 42, draft: true });
      expect(typeof fm[2]).toBe("string");
      expect(ast.value[1][0]).toBe("h1");
    });

    it("parses frontmatter YAML type coercion", async () => {
      const ast = await parseAST(
        '---\nstr: hello\nquoted: "world"\nnum: 3.14\nneg: -1\nbool_yes: yes\nbool_no: no\nnull_val: null\ntilde: ~\nempty:\n---',
      );
      const props = ast.value[0][1];
      expect(props.str).toBe("hello");
      expect(props.quoted).toBe("world");
      expect(props.num).toBe(3.14);
      expect(props.neg).toBe(-1);
      expect(props.bool_yes).toBe(true);
      expect(props.bool_no).toBe(false);
      expect(props.null_val).toBe(null);
      expect(props.tilde).toBe(null);
      expect(props.empty).toBe(null);
    });

    it("parses frontmatter with nested objects", async () => {
      const ast = await parseAST(
        "---\nauthor:\n  name: John\n  email: john@example.com\n---",
      );
      const props = ast.value[0][1];
      expect(props.author).toEqual({
        name: "John",
        email: "john@example.com",
      });
    });

    it("parses frontmatter with arrays", async () => {
      const ast = await parseAST(
        "---\ntags:\n  - javascript\n  - typescript\n---",
      );
      const props = ast.value[0][1];
      expect(props.tags).toEqual(["javascript", "typescript"]);
    });

    it("parses frontmatter with inline flow sequence", async () => {
      const ast = await parseAST("---\ntags: [javascript, typescript]\n---");
      const props = ast.value[0][1];
      expect(props.tags).toEqual(["javascript", "typescript"]);
    });

    it("parses frontmatter with mixed types in array", async () => {
      const ast = await parseAST(
        "---\ndata:\n  - hello\n  - 42\n  - true\n---",
      );
      const props = ast.value[0][1];
      expect(props.data).toEqual(["hello", 42, true]);
    });

    it("parses frontmatter with deeply nested structure", async () => {
      const ast = await parseAST(
        "---\nmeta:\n  author:\n    name: John\n  date: 2024\n---",
      );
      const props = ast.value[0][1];
      expect(props.meta).toEqual({ author: { name: "John" }, date: 2024 });
    });

    it("parses frontmatter with multi-line literal block", async () => {
      const ast = await parseAST(
        "---\ndescription: |\n  Line 1\n  Line 2\n---",
      );
      const props = ast.value[0][1];
      expect(props.description).toContain("Line 1");
      expect(props.description).toContain("Line 2");
    });

    it.skip("parses excerpt with <!-- more --> separator", async () => {
      const ast = await parseAST(
        "# Title\n\nIntro paragraph\n\n<!-- more -->\n\nFull content",
      );
      expect(ast.excerpt).toBeDefined();
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
      const ast = await parseAST(
        ":::outer\nOuter\n\n::inner\nInner\n::\n\n:::",
      );
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
      const ast = await parseAST("::alert{#my-id .highlight}\nText\n::");
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

    it("parses deep nested block components AST", async () => {
      const ast = await parseAST(
        "::::l1\n\n:::l2\n\n::l3\nDeep\n::\n\n:::\n\n::::",
      );
      const l1 = ast.value[0];
      expect(l1[0]).toBe("l1");
      const l2 = l1[2];
      expect(l2[0]).toBe("l2");
      const l3 = l2[2];
      expect(l3[0]).toBe("l3");
    });

    it("parses block component with named slots AST", async () => {
      const ast = await parseAST(
        "::card\n#header\n## Card Title\n\n#content\nMain content\n\n#footer\nFooter text\n::",
      );
      const card = ast.value[0];
      expect(card[0]).toBe("card");
      const header = card[2];
      expect(header[0]).toBe("template");
      expect(header[1].name).toBe("header");
      const content = card[3];
      expect(content[0]).toBe("template");
      expect(content[1].name).toBe("content");
      const footer = card[4];
      expect(footer[0]).toBe("template");
      expect(footer[1].name).toBe("footer");
    });

    it("renders single slot HTML", async () => {
      const html = await renderToHtml("::card\n#title\nCard Title\n::");
      expect(html).toContain('<template name="title">');
      expect(html).toContain("</template>");
      expect(html).toContain("<p>Card Title</p>");
    });

    it("parses default content before named slot AST", async () => {
      const ast = await parseAST(
        "::card\nDefault content\n\n#title\nCard Title\n::",
      );
      const card = ast.value[0];
      expect(card[0]).toBe("card");
      // Default content is a direct child (paragraph)
      expect(card[2][0]).toBe("p");
      expect(card[2][2]).toBe("Default content");
      // Named slot follows
      const tmpl = card[3];
      expect(tmpl[0]).toBe("template");
      expect(tmpl[1].name).toBe("title");
    });

    it("parses empty slot AST", async () => {
      const ast = await parseAST("::card\n#empty\n#content\nText here\n::");
      const card = ast.value[0];
      const empty = card[2];
      expect(empty[0]).toBe("template");
      expect(empty[1].name).toBe("empty");
      expect(empty.length).toBe(2); // no children
      const content = card[3];
      expect(content[0]).toBe("template");
      expect(content[1].name).toBe("content");
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
      const html = await renderToHtml(
        ':badge[T]{#myid .cls1 .cls2 key="val" flag}',
      );
      expect(html).toContain('id="myid"');
      expect(html).toContain('class="cls1 cls2"');
      expect(html).toContain('key="val"');
      expect(html).toContain("flag");
    });

    it("handles array/JSON prop value", async () => {
      const ast = await parseAST(':widget{:items=\'["a","b"]\'}');
      const comp = ast.value[0][2];
      expect(comp[0]).toBe("widget");
      expect(comp[1][":items"]).toEqual(["a", "b"]);
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
      const html = await renderToHtml(
        '[Link](https://example.com){target="_blank"}',
      );
      expect(html).toContain('target="_blank"');
      expect(html).toContain('<a href="https://example.com"');
    });

    it("renders image with class HTML", async () => {
      const html = await renderToHtml("![alt](img.png){.responsive}");
      expect(html).toContain(
        '<img src="img.png" alt="alt" class="responsive">',
      );
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
      const ast = await parseAST(
        '[Link](https://example.com){target="_blank"}',
      );
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

    it("renders strikethrough with attrs HTML", async () => {
      const html = await renderToHtml("~~del~~{.red}");
      expect(html).toContain('<del class="red">del</del>');
    });

    it("renders strong with boolean attr HTML", async () => {
      const html = await renderToHtml("**bold**{flag}");
      expect(html).toContain("<strong");
      expect(html).toContain("flag");
      expect(html).toContain(">bold</strong>");
    });

    it("renders strong with data attr HTML", async () => {
      const html = await renderToHtml('**bold**{data-value="custom"}');
      expect(html).toContain('data-value="custom"');
      expect(html).toContain(">bold</strong>");
    });

    it("parses strikethrough with attrs AST", async () => {
      const ast = await parseAST("~~del~~{.red}");
      const p = ast.value[0];
      const del = p[2];
      expect(del[0]).toBe("del");
      expect(del[1].class).toBe("red");
    });

    it("parses strong with boolean attr AST", async () => {
      const ast = await parseAST("**bold**{flag}");
      const p = ast.value[0];
      const strong = p[2];
      expect(strong[0]).toBe("strong");
      expect(strong[1].flag).toBe(true);
    });

    it("parses strong with data attr AST", async () => {
      const ast = await parseAST('**bold**{data-value="custom"}');
      const p = ast.value[0];
      const strong = p[2];
      expect(strong[0]).toBe("strong");
      expect(strong[1]["data-value"]).toBe("custom");
    });

    it.skip("renders heading with auto-id", async () => {
      const html = await renderToHtml("# Hello World");
      expect(html).toContain('id="hello-world"');
    });

    it.skip("renders emoji", async () => {
      const html = await renderToHtml("Hello :wave:");
      expect(html).toContain("\u{1F44B}");
    });
  });

  describe("real-world: nitro docs", () => {
    // Helper to recursively collect all element tag names from AST
    function collectTags(nodes, tags = []) {
      if (!Array.isArray(nodes)) return tags;
      for (const n of nodes) {
        if (Array.isArray(n)) {
          tags.push(n[0]);
          for (let i = 2; i < n.length; i++) {
            if (Array.isArray(n[i])) collectTags([n[i]], tags);
          }
        }
      }
      return tags;
    }

    // Helper to find elements by tag name (recursive)
    function findAll(nodes, tag) {
      const result = [];
      if (!Array.isArray(nodes)) return result;
      for (const n of nodes) {
        if (Array.isArray(n)) {
          if (n[0] === tag) result.push(n);
          for (let i = 2; i < n.length; i++) {
            if (Array.isArray(n[i])) result.push(...findAll([n[i]], tag));
          }
        }
      }
      return result;
    }

    it("parses without error", async () => {
      const ast = await parseAST(nitroIndex);
      expect(ast.type).toBe("comark");
      expect(ast.value.length).toBeGreaterThan(0);
    });

    it("parses frontmatter with nested seo object", async () => {
      const ast = await parseAST(nitroIndex);
      const fm = ast.value[0];
      expect(fm[0]).toBe("frontmatter");
      expect(fm[1].seo).toEqual({
        title: "Ship Full-Stack Vite Apps",
        description:
          "Nitro extends your Vite application with a production-ready server, compatible with any runtime. Add server routes to your application and deploy many hosting platform with a zero-config experience.",
      });
    });

    it("detects top-level block components", async () => {
      const ast = await parseAST(nitroIndex);
      // Skip frontmatter (index 0), collect top-level tags
      const topTags = ast.value
        .slice(1)
        .map((n) => (Array.isArray(n) ? n[0] : null));
      expect(topTags).toContain("u-page-hero");
      expect(topTags).toContain("div");
      expect(topTags).toContain("u-page-section");
    });

    it("detects nested block components", async () => {
      const ast = await parseAST(nitroIndex);
      const allTags = collectTags(ast.value);
      expect(allTags).toContain("code-group");
      expect(allTags).toContain("prose-pre");
      expect(allTags).toContain("u-button");
      expect(allTags).toContain("u-container");
      expect(allTags).toContain("tabs");
      // Deeply indented components (previously missed due to code block detection)
      expect(allTags).toContain("u-page-grid");
      expect(allTags).toContain("u-page-feature");
      expect(allTags).toContain("tabs-item");
      expect(allTags).toContain("code-tree");
    });

    it("detects inline components", async () => {
      const ast = await parseAST(nitroIndex);
      const allTags = collectTags(ast.value);
      expect(allTags).toContain("hero-background");
      expect(allTags).toContain("page-sponsors");
      expect(allTags).toContain("page-contributors");
    });

    it("detects named slots (templates)", async () => {
      const ast = await parseAST(nitroIndex);
      const templates = findAll(ast.value, "template");
      const slotNames = templates.map((t) => t[1].name);
      expect(slotNames).toContain("title");
      expect(slotNames).toContain("description");
      expect(slotNames).toContain("links");
      expect(slotNames).toContain("default");
    });

    it("detects component props", async () => {
      const ast = await parseAST(nitroIndex);
      const divs = findAll(ast.value, "div");
      const withClass = divs.find((d) => d[1].class?.includes("bg-neutral-50"));
      expect(withClass).toBeDefined();
    });

    it("detects u-button components inside slots", async () => {
      const ast = await parseAST(nitroIndex);
      const buttons = findAll(ast.value, "u-button");
      expect(buttons.length).toBe(2);
    });

    it("detects deeply nested tabs-item with props", async () => {
      const ast = await parseAST(nitroIndex);
      const tabsItems = findAll(ast.value, "tabs-item");
      expect(tabsItems.length).toBe(5);
      expect(tabsItems[0][1].label).toBe("FS Routing");
      expect(tabsItems[0][1].icon).toBe("i-lucide-folder");
    });

    it("detects u-page-feature components with slots", async () => {
      const ast = await parseAST(nitroIndex);
      const features = findAll(ast.value, "u-page-feature");
      expect(features.length).toBe(3);
      // Each feature has title and description slots
      for (const feature of features) {
        const slots = feature
          .slice(2)
          .filter((c) => Array.isArray(c) && c[0] === "template");
        expect(slots.length).toBe(2);
      }
    });

    it("renders HTML without error", async () => {
      const html = await renderToHtml(nitroIndex);
      expect(html).toContain("<u-page-hero>");
      expect(html).toContain("<u-page-section>");
      expect(html).toContain("</u-page-hero>");
    });

    it("renders ANSI without error", async () => {
      const ansi = await renderToAnsi(nitroIndex);
      expect(ansi.length).toBeGreaterThan(0);
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

  describe("renderToMeta", () => {
    it("returns a string", async () => {
      const json = await renderToMeta("# Hello");
      expect(typeof json).toBe("string");
    });

    it("returns valid JSON", async () => {
      const json = await renderToMeta("# Hello");
      const parsed = JSON.parse(json);
      expect(parsed.headings).toBeInstanceOf(Array);
    });

    it("returns empty headings for empty input", async () => {
      const json = await renderToMeta("");
      const parsed = JSON.parse(json);
      expect(parsed.headings).toHaveLength(0);
    });
  });

  describe("parseMeta", () => {
    it("extracts title from frontmatter", async () => {
      const meta = await parseMeta(
        "---\ntitle: Hello\ntags: [a, b]\n---\n\n# My Doc\n\n## Section 1",
      );
      expect(meta.title).toBe("Hello");
      expect(meta.tags).toEqual(["a", "b"]);
      expect(meta.headings).toEqual([
        { level: 1, text: "My Doc" },
        { level: 2, text: "Section 1" },
      ]);
    });

    it("falls back to first heading as title", async () => {
      const meta = await parseMeta("# My Doc\n\n## Section 1");
      expect(meta.title).toBe("My Doc");
      expect(meta.headings).toEqual([
        { level: 1, text: "My Doc" },
        { level: 2, text: "Section 1" },
      ]);
    });

    it("returns empty headings for empty input", async () => {
      const meta = await parseMeta("");
      expect(meta.headings).toHaveLength(0);
      expect(meta.title).toBeUndefined();
    });

    it("extracts multiple headings at different levels", async () => {
      const meta = await parseMeta("# H1\n\n## H2\n\n### H3\n\n#### H4");
      expect(meta.headings).toEqual([
        { level: 1, text: "H1" },
        { level: 2, text: "H2" },
        { level: 3, text: "H3" },
        { level: 4, text: "H4" },
      ]);
    });

    it("strips inline formatting from heading text", async () => {
      const meta = await parseMeta("# **Bold** and *italic* heading");
      expect(meta.headings[0].text).toBe("Bold and italic heading");
    });

    it("handles frontmatter with complex YAML", async () => {
      const meta = await parseMeta(
        "---\ntitle: Hello\nauthor:\n  name: John\ntags:\n  - js\n  - ts\ncount: 42\ndraft: true\n---",
      );
      expect(meta.title).toBe("Hello");
      expect(meta.author).toEqual({ name: "John" });
      expect(meta.tags).toEqual(["js", "ts"]);
      expect(meta.count).toBe(42);
      expect(meta.draft).toBe(true);
    });

    it("handles frontmatter without title and heading", async () => {
      const meta = await parseMeta("---\ndraft: true\n---\n\nJust a paragraph");
      expect(meta.draft).toBe(true);
      expect(meta.title).toBeUndefined();
      expect(meta.headings).toHaveLength(0);
    });

    it("handles heading with entity", async () => {
      const meta = await parseMeta("# Hello &amp; World");
      expect(meta.headings[0].text).toBe("Hello & World");
    });

    it("handles heading with inline code", async () => {
      const meta = await parseMeta("# Using `parseMeta` API");
      expect(meta.headings[0].text).toBe("Using parseMeta API");
    });
  });

  describe("renderToText", () => {
    it("renders a heading", async () => {
      expect(await renderToText("# Hello")).toBe("Hello\n");
    });

    it("renders a paragraph", async () => {
      expect(await renderToText("Hello world")).toBe("Hello world\n");
    });

    it("strips inline formatting", async () => {
      expect(await renderToText("**bold** and *italic*")).toBe(
        "bold and italic\n",
      );
    });

    it("renders empty input", async () => {
      expect(await renderToText("")).toBe("");
    });

    it("renders a link as text only", async () => {
      expect(await renderToText("[click](https://example.com)")).toBe(
        "click\n",
      );
    });

    it("renders a code block with indent", async () => {
      const text = await renderToText("```\ncode\n```");
      expect(text).toContain("  code");
    });

    it("renders unordered list", async () => {
      const text = await renderToText("- one\n- two");
      expect(text).toContain("- one");
      expect(text).toContain("- two");
    });

    it("renders ordered list", async () => {
      const text = await renderToText("1. first\n2. second");
      expect(text).toContain("1. first");
      expect(text).toContain("2. second");
    });

    it("renders blockquote with prefix", async () => {
      const text = await renderToText("> quoted");
      expect(text).toContain("> quoted");
    });

    it("renders horizontal rule", async () => {
      const text = await renderToText("text\n\n***");
      expect(text).toContain("---");
    });

    it("strips frontmatter", async () => {
      const text = await renderToText("---\ntitle: Test\n---\n\n# Content");
      expect(text).not.toContain("title: Test");
      expect(text).toContain("Content");
    });

    it("renders task list", async () => {
      const text = await renderToText("- [x] done\n- [ ] todo");
      expect(text).toContain("[x] done");
      expect(text).toContain("[ ] todo");
    });

    it("resolves entities", async () => {
      expect(await renderToText("&amp;")).toBe("&\n");
    });

    it("renders multiline content", async () => {
      const text = await renderToText("# Title\n\nParagraph\n\n- item");
      expect(text).toContain("Title");
      expect(text).toContain("Paragraph");
      expect(text).toContain("- item");
    });

    it("renders real-world content without error", async () => {
      const text = await renderToText(nitroIndex);
      expect(text.length).toBeGreaterThan(0);
    });
  });
}
