export {
  renderToHtml,
  renderToAST,
  parseAST,
  renderToAnsi,
  parseMeta,
  renderToMeta,
  renderToText,
  heal,
} from "./common.mjs";

import { _setInstance, _imports, _hasInstance } from "./common.mjs";

export async function init(opts) {
  if (_hasInstance()) {
    return;
  }
  const input = opts?.wasm;
  let bytes;
  if (input instanceof ArrayBuffer || input instanceof Uint8Array) {
    bytes = input;
  } else if (input instanceof WebAssembly.Module) {
    const { instance } = await WebAssembly.instantiate(input, {
      wasi_snapshot_preview1: wasiStub,
    });
    _setInstance(instance);
    return;
  } else if (
    input instanceof Response ||
    (typeof input === "object" && typeof input.then === "function")
  ) {
    const response = await input;
    if (response instanceof Response) {
      bytes = await response.arrayBuffer();
    } else {
      bytes = response;
    }
  } else {
    const fsp = globalThis.process?.getBuiltinModule?.("fs/promises");
    if (fsp) {
      const wasmPath = new URL("../../build/md4x.wasm", import.meta.url);
      bytes = await fsp.readFile(wasmPath);
    } else {
      bytes = await fetch(
        await import("../../build/md4x.wasm?url").then((m) => m.default),
      ).then((r) => r.arrayBuffer());
    }
  }
  const { instance } = await WebAssembly.instantiate(bytes, _imports);
  _setInstance(instance);
}
