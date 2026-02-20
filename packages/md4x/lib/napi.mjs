import { createRequire } from "node:module";
import { arch, platform } from "node:process";

const require = createRequire(import.meta.url);

function isMusl() {
  if (platform !== "linux") return false;
  try {
    // glibc sets glibcVersionRuntime; musl does not
    return !process.report?.getReport?.()?.header?.glibcVersionRuntime;
  } catch {
    return false;
  }
}

function loadBinding() {
  const suffix = isMusl() ? "-musl" : "";
  return require(`../build/md4x.${platform}-${arch}${suffix}.node`);
}

const binding = loadBinding();

export function renderToHtml(input) {
  return binding.renderToHtml(input);
}

export function renderToAST(input) {
  return binding.renderToAST(input);
}

export function parseAST(input) {
  return JSON.parse(renderToAST(input));
}

export function renderToAnsi(input) {
  return binding.renderToAnsi(input);
}
