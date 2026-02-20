import { parseHtmlWithHighlighting } from "./_shared.mjs";

// --- internal ---

let binding;

function str(input) {
  if (input == null) return "";
  if (typeof input !== "string")
    throw new TypeError("md4x: input must be a string");
  return input;
}

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

const HEAL_FLAG = 0x0100;

export function renderToHtml(input, opts) {
  let flags = opts?.full ? 0x0008 : 0;
  if (opts?.heal) flags |= HEAL_FLAG;
  if (!opts?.highlighter) {
    return getBinding().renderToHtml(str(input), flags);
  }
  const buf = getBinding().renderToHtmlMeta(str(input));
  return parseHtmlWithHighlighting(
    new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength),
    opts.highlighter,
  );
}

export function renderToAST(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  return getBinding().renderToAST(str(input), flags);
}

export function parseAST(input, opts) {
  return JSON.parse(renderToAST(input, opts));
}

export function renderToAnsi(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  return getBinding().renderToAnsi(str(input), flags);
}

export function renderToMeta(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  return getBinding().renderToMeta(str(input), flags);
}

export function renderToText(input, opts) {
  const flags = opts?.heal ? HEAL_FLAG : 0;
  return getBinding().renderToText(str(input), flags);
}

export function parseMeta(input, opts) {
  const meta = JSON.parse(renderToMeta(input, opts));
  if (!meta.title && meta.headings?.[0]) {
    meta.title = meta.headings[0].text;
  }
  return meta;
}

export function heal(input) {
  return getBinding().heal(str(input));
}
