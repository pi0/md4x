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

function render(exports, fn, input, ...extra) {
  const { memory, md4x_alloc, md4x_free, md4x_result_ptr, md4x_result_size } =
    exports;
  const encoded = new TextEncoder().encode(input);
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

export function renderToHtml(input, opts) {
  const flags = opts?.full ? 0x0008 : 0;
  const exports = _getExports();
  return render(exports, exports.md4x_to_html, input, flags);
}

export function renderToAST(input) {
  const exports = _getExports();
  return render(exports, exports.md4x_to_ast, input);
}

export function parseAST(input) {
  return JSON.parse(renderToAST(input));
}

export function renderToAnsi(input) {
  const exports = _getExports();
  return render(exports, exports.md4x_to_ansi, input);
}

export function renderToMeta(input) {
  const exports = _getExports();
  return render(exports, exports.md4x_to_meta, input);
}

export function renderToText(input) {
  const exports = _getExports();
  return render(exports, exports.md4x_to_text, input);
}

export function parseMeta(input) {
  const meta = JSON.parse(renderToMeta(input));
  if (!meta.title && meta.headings?.[0]) {
    meta.title = meta.headings[0].text;
  }
  return meta;
}

export function heal(input) {
  const exports = _getExports();
  return render(exports, exports.md4x_heal, input);
}
