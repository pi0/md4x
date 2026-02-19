import { writeFileSync } from "node:fs";
import { bench, compact, run, summary } from "mitata";
import * as napi from "../lib/napi.mjs";
import * as wasm from "../lib/wasm.mjs";
import MarkdownIt from "markdown-it";
import pluginMdc from "markdown-it-mdc";

await wasm.initWasm();

const markdownItMdc = new MarkdownIt().use(pluginMdc);

const input = `---
title: Documentation
description: Full Comark example
---

# Getting Started

:badge[v2.0]{color="green" .pill} :icon-new

Status: :badge[Active]{color="green"} and :icon-check

Click the :button[Submit]{type="primary" size="lg" disabled} to continue.

:tooltip{text="Hover text"} is useful for :badge[**bold** content]{#my-id .highlight}.

**bold**{.highlight} and *italic*{#myid} and \`code\`{.lang-ts}

~~deleted~~{.red} and _underline_{.accent}

[Link](https://example.com){target="_blank" rel="noopener"}

![hero](hero.png){.responsive .rounded width="800"}

[styled text]{.prose .dark:prose-invert} and [tag]{#label .badge .badge-sm}

::alert{type="warning" dismissible}
> Important: use :badge[NAPI]{color="blue"} for server and :badge[WASM]{color="purple"} for browser.

- Item one
- Item two
::

::card{#main-card .shadow .rounded}
- [x] :badge[Done]{color="green"} CommonMark support
- [x] :badge[Done]{color="green"} GFM extensions
- [ ] :badge[WIP]{color="yellow"} Plugin system

Main content with :badge[Tag]{color="blue"} and \`inline code\`.

[View docs](https://example.com){target="_blank"} · [GitHub](https://github.com/example){.link}
::

::code-group
\`\`\`js [app.js] {1,3}
const x = 1;
const y = 2;
const z = x + y;
\`\`\`

\`\`\`ts [app.ts] {2}
const x: number = 1;
const y: number = 2;
\`\`\`
::
`;

// --- Compare HTML output ---
const napiHtml = napi.renderToHtml(input);
const mdcHtml = markdownItMdc.render(input);

const wrap = (title, body) =>
  `<!doctype html><html><head><meta charset="utf-8"><title>${title}</title></head><body>\n${body}\n</body></html>`;

writeFileSync("/tmp/comark-napi.html", wrap("md4x (napi)", napiHtml));
writeFileSync("/tmp/comark-mdc.html", wrap("markdown-it-mdc", mdcHtml));

console.log("HTML output written:");
console.log("  /tmp/comark-napi.html");
console.log("  /tmp/comark-mdc.html");
console.log();

// --- Benchmark ---
compact(() => {
  summary(() => {
    bench("napi html", () => napi.renderToHtml(input));
    bench("wasm html", () => wasm.renderToHtml(input));
    bench("markdown-it-mdc html", () => markdownItMdc.render(input));
  });
});

await run();
