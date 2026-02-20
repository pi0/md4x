import { beforeAll } from "vitest";
import {
  initWasm,
  renderToHtml,
  renderToAST,
  renderToAnsi,
  parseAST,
} from "../lib/wasm.mjs";
import { defineSuite } from "./_suite.mjs";

beforeAll(async () => {
  await initWasm();
});

defineSuite({ renderToHtml, renderToAST, renderToAnsi, parseAST });
