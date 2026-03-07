import { beforeAll } from "vitest";
import {
  init,
  renderToHtml,
  renderToAST,
  renderToAnsi,
  parseAST,
  renderToMeta,
  parseMeta,
  renderToText,
  heal,
} from "md4x/wasm";
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
  renderToText,
  heal,
});
