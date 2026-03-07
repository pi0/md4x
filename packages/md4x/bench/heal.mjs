import { bench, compact, run, summary } from "mitata";
import * as napi from "../lib/napi.mjs";
import * as wasm from "../lib/wasm/defult.mjs";
import remend from "remend";
import * as fixtures from "./_fixtures.mjs";

await wasm.init();
await napi.init();

// Strip closing delimiters to create broken markdown that triggers heal paths
function breakMarkdown(input) {
  return input
    .replace(/\*\*([^*]+)\*\*/g, "**$1") // unclosed bold
    .replace(/\*([^*]+)\*/g, "*$1") // unclosed italic
    .replace(/~~([^~]+)~~/g, "~~$1") // unclosed strikethrough
    .replace(/`([^`]+)`/g, "`$1") // unclosed inline code
    .replace(/```\n/g, "```js\n") // ensure fenced code blocks
    .replace(/\n```$/gm, ""); // remove closing code fences
}

const inputs = {
  small: breakMarkdown(fixtures.small),
  medium: breakMarkdown(fixtures.medium),
  large: breakMarkdown(fixtures.large),
};

for (const [name, input] of Object.entries(inputs)) {
  compact(() => {
    summary(() => {
      bench(`md4x-napi heal (${name})`, () => napi.heal(input));
      bench(`md4x-wasm heal (${name})`, () => wasm.heal(input));
      bench(`remend heal (${name})`, () => remend(input));
    });
  });
}

await run();
