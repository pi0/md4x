import { beforeAll } from "vitest";
import {
  init,
  renderToHtml,
  renderToAST,
  renderToAnsi,
  parseAST,
} from "../lib/wasm.mjs";
import { defineSuite } from "./_suite.mjs";

beforeAll(async () => {
  await init();
});

defineSuite({ renderToHtml, renderToAST, renderToAnsi, parseAST });
