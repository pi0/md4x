import { createRequire } from "node:module";
import { arch, platform } from "node:process";

const require = createRequire(import.meta.url);

function loadBinding() {
  return require(`../build/md4x.${platform}-${arch}.node`);
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
