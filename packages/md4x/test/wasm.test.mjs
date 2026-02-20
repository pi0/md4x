import { beforeAll } from "vitest";
import {
  init,
  renderToHtml,
  renderToAST,
  renderToAnsi,
  parseAST,
  renderToMeta,
  parseMeta,
} from "../lib/wasm.mjs";
import { defineSuite } from "./_suite.mjs";

beforeAll(async () => {
  await init();
});

defineSuite({
  renderToHtml,
  renderToAST,
  renderToAnsi,
  parseAST,
  renderToMeta,
  parseMeta,
});
