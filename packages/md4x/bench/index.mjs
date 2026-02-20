import { bench, compact, run, summary } from "mitata";
import * as napi from "../lib/napi.mjs";
import * as wasm from "../lib/wasm.mjs";
import * as md4w from "md4w";
import MarkdownIt from "markdown-it";
import * as fixtures from "./_fixtures.mjs";

const markdownIt = new MarkdownIt();

// Initialize WASM instances
await wasm.init();
await napi.init();
await md4w.init();

const inputs = {
  // small: fixtures.small,
  medium: fixtures.medium,
  // large: fixtures.large,
};

for (const [name, input] of Object.entries(inputs)) {
  compact(() => {
    summary(() => {
      bench(`md4x-napi`, () => napi.renderToHtml(input));
      bench(`md4x-wasm`, () => wasm.renderToHtml(input));
      bench(`md4w`, () => md4w.mdToHtml(input));
      bench(`markdown-it`, () => markdownIt.render(input));

      // const bunToHTML = global.Bun.markdown.html;
      // if (bunToHTML) {
      //   bench(`Bun.markdown.html`, () => bunToHTML(input));
      // }
    });

    summary(() => {
      bench(`md4x (napi) ast (${name})`, () => napi.renderToAST(input));
      bench(`md4x (wasm) ast (${name})`, () => wasm.renderToAST(input));
    });

    summary(() => {
      bench(`md4x (napi) parseAST (${name})`, () => napi.parseAST(input));
      bench(`md4x (wasm) parseAST (${name})`, () => wasm.parseAST(input));
      bench(`md4w parseAST (${name})`, () => md4w.mdToJSON(input));
      bench(`markdown-it parseAST (${name})`, () =>
        JSON.stringify(markdownIt.parse(input, {})),
      );
    });
  });
}

await run();
