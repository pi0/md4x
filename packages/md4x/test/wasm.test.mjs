import { beforeAll } from "vitest";
import * as api from "../lib/wasm.mjs";
import { defineSuite } from "./_suite.mjs";

beforeAll(async () => {
  await api.init();
});

defineSuite(api);
