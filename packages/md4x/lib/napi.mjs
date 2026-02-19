import { createRequire } from "node:module";
import { arch, platform } from "node:process";

const require = createRequire(import.meta.url);

function loadBinding() {
  const name = `md4x.${platform}-${arch}`;
  try {
    return require(`../build/${name}.node`);
  } catch {
    return require("../build/md4x.node");
  }
}

const binding = loadBinding();

export function renderToHtml(input) {
  return binding.renderToHtml(input);
}

export function renderToJson(input) {
  return JSON.parse(binding.renderToJson(input));
}

export function renderToAnsi(input) {
  return binding.renderToAnsi(input);
}
