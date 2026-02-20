import { parseHtmlWithHighlighting } from "./_shared.mjs";

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

export function renderToHtml(input, opts) {
  if (!opts?.highlighter) {
    const flags = opts?.full ? 0x0008 : 0;
    return getBinding().renderToHtml(input, flags);
  }

  const buf = getBinding().renderToHtmlMeta(input);
  return parseHtmlWithHighlighting(
    new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength),
    opts.highlighter,
  );
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

export function renderToMeta(input) {
  return getBinding().renderToMeta(input);
}

export function renderToText(input) {
  return getBinding().renderToText(input);
}

export function parseMeta(input) {
  const meta = JSON.parse(renderToMeta(input));
  if (!meta.title && meta.headings?.[0]) {
    meta.title = meta.headings[0].text;
  }
  return meta;
}
