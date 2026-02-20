// --- internal ---

let binding;

function getBinding(opts) {
  if (binding) return binding;
  if (opts?.binding) {
    return (binding = opts.binding);
  }
  const { arch, platform } = globalThis.process || {};
  const { createRequire } = globalThis.process?.getBuiltinModule?.("module");
  const isMusl =
    platform === "linux" &&
    !process.report?.getReport?.()?.header?.glibcVersionRuntime;
  return (binding = createRequire(import.meta.url)(
    `../build/md4x.${platform}-${arch}${isMusl ? "-musl" : ""}.node`,
  ));
}

// --- exports ----

export function init(opts) {
  try {
    getBinding(opts);
    return Promise.resolve(binding);
  } catch (err) {
    return Promise.reject(err);
  }
}

export function renderToHtml(input) {
  return getBinding().renderToHtml(input);
}

export function renderToAST(input) {
  return getBinding().renderToAST(input);
}

export function parseAST(input) {
  return JSON.parse(renderToAST(input));
}

export function renderToAnsi(input) {
  return getBinding().renderToAnsi(input);
}
