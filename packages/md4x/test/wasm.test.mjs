import { beforeAll } from "vitest";
import { initWasm, renderToHtml, renderToJson, renderToAnsi } from "../lib/wasm.mjs";
import { defineSuite } from "./_suite.mjs";

beforeAll(async () => {
  await initWasm();
});

defineSuite({ renderToHtml, renderToJson, renderToAnsi });
