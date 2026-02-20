let _instance;

function getExports() {
  if (!_instance) {
    throw new Error(
      "md4x: WASM not initialized. Call `await initWasm()` first.",
    );
  }
  return _instance.exports;
}

export async function initWasm(input) {
  if (_instance) return;
  let bytes;
  if (input instanceof ArrayBuffer || input instanceof Uint8Array) {
    bytes = input;
  } else if (input instanceof WebAssembly.Module) {
    const { instance } = await WebAssembly.instantiate(input, {
      wasi_snapshot_preview1: wasiStub,
    });
    _instance = instance;
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
      const wasmPath = new URL("../build/md4x.wasm", import.meta.url);
      bytes = await fsp.readFile(wasmPath);
    } else {
      bytes = await fetch(
        await import("../build/md4x.wasm?url").then((m) => m.default),
      ).then((r) => r.arrayBuffer());
    }
  }
  const { instance } = await WebAssembly.instantiate(bytes, {
    wasi_snapshot_preview1: wasiStub,
  });
  _instance = instance;
}

export function renderToHtml(input) {
  return render(getExports(), getExports().md4x_to_html, input);
}

export function renderToAST(input) {
  return render(getExports(), getExports().md4x_to_ast, input);
}

export function parseAST(input) {
  return JSON.parse(renderToAST(input));
}

export function renderToAnsi(input) {
  return render(getExports(), getExports().md4x_to_ansi, input);
}

// --- internals ---

const wasiStub = {
  fd_close: () => 0,
  fd_seek: () => 0,
  fd_write: () => 0,
  proc_exit: () => {},
};

function render(exports, fn, input) {
  const { memory, md4x_alloc, md4x_free, md4x_result_ptr, md4x_result_size } =
    exports;
  const encoded = new TextEncoder().encode(input);
  const ptr = md4x_alloc(encoded.length);
  new Uint8Array(memory.buffer).set(encoded, ptr);
  const ret = fn(ptr, encoded.length);
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
