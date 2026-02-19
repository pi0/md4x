import { createRequire } from "node:module";

const require = createRequire(import.meta.url);
const binding = require("../build/md4x.node");

export function renderToHtml(input) {
  return binding.renderToHtml(input);
}

export function renderToJson(input) {
  return JSON.parse(binding.renderToJson(input));
}

export function renderToAnsi(input) {
  return binding.renderToAnsi(input);
}
