import { parseHtmlWithHighlighting } from "../_shared.mjs";

// --- internal ---

let _instance;

export function _hasInstance() {
  return !!_instance;
}

export function _setInstance(instance) {
  _instance = instance;
}

export function _getExports() {
  if (!_instance?.exports) {
    throw new Error("md4x: WASM not initialized. Call `await init()` first.", {
      cause: { instance: _instance },
    });
  }
  return _instance.exports;
}

export const _imports = {
  wasi_snapshot_preview1: {
    fd_close: () => 0,
    fd_seek: () => 0,
    fd_write: () => 0,
    proc_exit: () => {},
  },
};

function str(input) {
  if (input == null) return "";
  if (typeof input !== "string")
    throw new TypeError("md4x: input must be a string");
  return input;
}

function render(exports, fn, input, ...extra) {
  const { memory, md4x_alloc, md4x_free, md4x_result_ptr, md4x_result_size } =
    exports;
  const encoded = new TextEncoder().encode(str(input));
  const ptr = md4x_alloc(encoded.length);
  new Uint8Array(memory.buffer).set(encoded, ptr);
  const ret = fn(ptr, encoded.length, ...extra);
  md4x_free(ptr);
  if (ret !== 0) {
    throw new Error("md4x: render failed");
  }
  const outPtr = md4x_result_ptr();
  const outSize = md4x_result_size();
  const result = new TextDecoder().decode(
    new Uint8Array(memory.buffer, outPtr, outSize),
  );
  md4x_free(outPtr);
  return result;
}

const HEAL_FLAG = 0x0100;

export function renderToHtml(input, opts) {
  let flags = opts?.full ? 0x0008 : 0;
  if (opts?.heal) flags |= HEAL_FLAG;
  const exports = _getExports();
  if (!opts?.highlighter) {
    return render(exports, exports.md4x_to_html, input, flags);
  }
  const {
    memory,
    md4x_alloc,
    md4x_free,
    md4x_to_html_meta,
    md4x_result_ptr,
    md4x_result_size,
  } = exports;
  const encoded = new TextEncoder().encode(str(input));
  const ptr = md4x_alloc(encoded.length);
  new Uint8Array(memory.buffer).set(encoded, ptr);
  const ret = md4x_to_html_meta(ptr, encoded.length);
  md4x_free(ptr);
  if (ret !== 0) {
    throw new Error("md4x: render failed");
  }
  const outPtr = md4x_result_ptr();
  const outSize = md4x_result_size();
  const bytes = new Uint8Array(memory.buffer, outPtr, outSize);
  const result = parseHtmlWithHighlighting(bytes, opts.highlighter);
  md4x_free(outPtr);
  return result;
}

export function renderToAST(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  const exports = _getExports();
  return render(exports, exports.md4x_to_ast, input, flags);
}

export function parseAST(input, opts) {
  return JSON.parse(renderToAST(input, opts));
}

export function renderToAnsi(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  const exports = _getExports();
  return render(exports, exports.md4x_to_ansi, input, flags);
}

export function renderToMeta(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  const exports = _getExports();
  return render(exports, exports.md4x_to_meta, input, flags);
}

export function renderToText(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  const exports = _getExports();
  return render(exports, exports.md4x_to_text, input, flags);
}

export function parseMeta(input, opts) {
  const meta = JSON.parse(renderToMeta(input, opts));
  if (!meta.title && meta.headings?.[0]) {
    meta.title = meta.headings[0].text;
  }
  return meta;
}

export function heal(input) {
  const exports = _getExports();
  return render(exports, exports.md4x_heal, input);
}
