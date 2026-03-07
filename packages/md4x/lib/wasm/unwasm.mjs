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

import { _setInstance, _hasInstance, _imports } from "./common.mjs";

export async function init() {
  if (_hasInstance()) {
    return;
  }
  let { default: instance } = await import("md4x/build/md4x.wasm?module");
  if (instance instanceof WebAssembly.Module) {
    instance = await WebAssembly.instantiate(instance, _imports);
  }
  _setInstance(instance);
}
