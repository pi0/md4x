import { init as _init } from "./wasm.mjs";

export {
  renderToHtml,
  renderToAST,
  parseAST,
  renderToAnsi,
  parseMeta,
  renderToMeta,
  renderToText,
} from "./wasm.mjs";

export async function init(opts) {
  if (opts?.wasm) {
    return _init(opts);
  }
  const mod = await import("md4x/build/md4x.wasm?module");
  return _init({ wasm: mod.default });
}
