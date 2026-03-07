import { beforeAll, describe, it, expect } from "vitest";
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
import { _setInstance, _getExports } from "../lib/wasm/common.mjs";
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

describe("wasm: error handling", () => {
  describe("not initialized", () => {
    it("_getExports throws 'md4x: WASM not initialized...' when instance is null", () => {
      const saved = _getExports();
      _setInstance(null);
      try {
        expect(() => _getExports()).toThrow(
          "md4x: WASM not initialized. Call `await init()` first.",
        );
      } finally {
        _setInstance({ exports: saved });
      }
    });

    it("_getExports throws 'md4x: WASM not initialized...' when instance has no exports", () => {
      const saved = _getExports();
      _setInstance({});
      try {
        expect(() => _getExports()).toThrow(
          "md4x: WASM not initialized. Call `await init()` first.",
        );
      } finally {
        _setInstance({ exports: saved });
      }
    });
  });
});
