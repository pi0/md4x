#!/usr/bin/env node

import { parseArgs } from "node:util";
import { readFileSync, writeFileSync } from "node:fs";
import {
  renderToHtml,
  renderToAnsi,
  renderToText,
  parseAST,
  parseMeta,
} from "./napi.mjs";

const { values, positionals } = parseArgs({
  allowPositionals: true,
  options: {
    output: { type: "string", short: "o" },
    format: {
      type: "string",
      short: "t",
      default: process.stdout.isTTY ? "ansi" : "text",
    },
    "full-html": { type: "boolean", short: "f", default: false },
    "html-title": { type: "string" },
    "html-css": { type: "string" },
    stat: { type: "boolean", short: "s", default: false },
    help: { type: "boolean", short: "h", default: false },
    version: { type: "boolean", short: "v", default: false },
  },
});

const _tty = process.stderr.isTTY;
const c = (code) => (s) => (_tty ? `\x1b[${code}m${s}\x1b[0m` : s);
const _b = c(1);
const _d = c(2);
const _c = c(36);
const _g = c(32);

function usage() {
  process.stderr.write(
    `${_b("md4x")} — Markdown renderer

${_g("Usage:")} ${_b("md4x")} ${_d("[OPTION]... [FILE]")}

${_g("General options:")}
  ${_c("-o")}, ${_c("--output")}=${_d("FILE")}     Output file ${_d("(default: stdout)")}
  ${_c("-t")}, ${_c("--format")}=${_d("FORMAT")}   Output format: ${_c("html")}, ${_c("text")}, ${_c("ast")}, ${_c("ansi")}, ${_c("meta")} ${_d("(default: ansi for TTY, text otherwise)")}
  ${_c("-s")}, ${_c("--stat")}            Measure parsing time
  ${_c("-h")}, ${_c("--help")}            Display this help and exit
  ${_c("-v")}, ${_c("--version")}         Display version and exit

${_g("Input:")}
  File path, ${_c("-")} for stdin, HTTP/HTTPS URL, or shorthand:
  ${_c("gh:")}${_d("<owner>/<repo>[/path]")}          GitHub raw content
  ${_c("npm:")}${_d("<package>[@version][/path]")}    npm package via unpkg

${_g("HTML options:")}
      ${_c("-f")}, ${_c("--full-html")}     Full HTML document with header
      ${_c("--html-title")}=${_d("TITLE")}  Document title
      ${_c("--html-css")}=${_d("URL")}      CSS link

${_g("Examples:")}
  ${_d("$")} ${_b("md4x")} README.md                          ${_d("# ANSI output")}
  ${_d("$")} ${_b("md4x")} README.md ${_c("-t html")}                  ${_d("# HTML output")}
  ${_d("$")} ${_b("md4x")} README.md ${_c("-t text")}                  ${_d("# Plain text output (strip markdown)")}
  ${_d("$")} ${_b("md4x")} README.md ${_c("-t ast")}                   ${_d("# JSON AST output (comark)")}
  ${_d("$")} ${_b("md4x")} README.md ${_c("-t meta")}                  ${_d("# Metadata JSON output")}
  ${_d("$")} ${_b("md4x")} ${_c("https://nitro.build/guide")}          ${_d("# Fetch and render any URL")}
  ${_d("$")} ${_b("md4x")} ${_c("gh:")}nitrojs/nitro                   ${_d("# GitHub repo → README.md")}
  ${_d("$")} ${_b("md4x")} ${_c("npm:")}vue@3                          ${_d("# npm package at specific version")}
  ${_d("$")} echo "# Hello" | ${_b("md4x")} ${_c("-t text")}
  ${_d("$")} cat README.md | ${_b("md4x")} ${_c("-t html")}
  ${_d("$")} ${_b("md4x")} README.md ${_c("-t meta -o")} README.json   ${_d("# Write to file")}
  ${_d("$")} ${_b("md4x")} README.md ${_c("-t html -f --html-title")}="My Docs"  ${_d("# Full HTML with <head>")}
  ${_d("$")} ${_b("md4x")} README.md ${_c("-t html -f --html-css")}=style.css    ${_d("# Add CSS link")}
`,
  );
}

if (values.help) {
  usage();
  process.exit(0);
}

if (values.version) {
  const { version } = JSON.parse(
    readFileSync(new URL("../package.json", import.meta.url), "utf8"),
  );
  process.stdout.write(`${version}\n`);
  process.exit(0);
}

const supportedFormats = ["html", "ast", "ansi", "text", "meta"];
const format = values.format;
if (!supportedFormats.includes(format)) {
  process.stderr.write(`Unknown format: ${format}\n`);
  process.stderr.write(`Supported formats: ${supportedFormats.join(", ")}\n`);
  process.exit(1);
}

let inputPath = positionals[0];
// gh:owner/repo[/path] → https://github.com/owner/repo[/path]
if (inputPath && /^gh:/i.test(inputPath)) {
  inputPath = `https://github.com/${inputPath.slice(3)}`;
}
// npm:package[@version][/path] → https://unpkg.com/package[@version][/path]
if (inputPath && /^npm:/i.test(inputPath)) {
  const spec = inputPath.slice(4);
  // Check if spec includes a file path (after the package name + optional version)
  // Scoped: @scope/pkg[@ver][/path] — path starts after 2nd slash
  // Unscoped: pkg[@ver][/path] — path starts after 1st slash
  const slashIdx = spec.startsWith("@")
    ? spec.indexOf("/", spec.indexOf("/") + 1)
    : spec.indexOf("/");
  const hasPath = slashIdx > 0;
  inputPath = `https://unpkg.com/${spec}${hasPath ? "" : "/README.md"}`;
}
let input;
if (!inputPath || inputPath === "-") {
  if (process.stdin.isTTY) {
    usage();
    process.exit(1);
  }
  try {
    input = readFileSync(0, "utf8");
  } catch {
    usage();
    process.exit(1);
  }
} else if (/^https?:\/\//i.test(inputPath)) {
  const fetchUrl = toRawUrl(inputPath);
  try {
    const res = await fetch(fetchUrl, {
      headers: { Accept: "text/markdown, text/plain;q=0.9, */*;q=0.1" },
    });
    if (!res.ok) {
      process.stderr.write(
        `Failed to fetch ${inputPath}: ${res.status} ${res.statusText}\n`,
      );
      process.exit(1);
    }
    input = await res.text();
  } catch (err) {
    process.stderr.write(`Failed to fetch ${inputPath}: ${err.message}\n`);
    process.exit(1);
  }
} else {
  try {
    input = readFileSync(inputPath, "utf8");
  } catch {
    process.stderr.write(`Cannot open ${inputPath}.\n`);
    process.exit(1);
  }
}

const t0 = values.stat ? performance.now() : 0;

let output;
switch (format) {
  case "html":
    output = renderToHtml(input);
    break;
  case "ansi":
    output = renderToAnsi(input);
    break;
  case "text":
    output = renderToText(input);
    break;
  case "ast":
    output = JSON.stringify(parseAST(input), null, 2);
    break;
  case "meta":
    output = JSON.stringify(parseMeta(input), null, 2);
    break;
}

if (values.stat) {
  const elapsed = performance.now() - t0;
  if (elapsed < 1000) {
    process.stderr.write(`Time spent on parsing: ${elapsed.toFixed(2)} ms.\n`);
  } else {
    process.stderr.write(
      `Time spent on parsing: ${(elapsed / 1000).toFixed(3)} s.\n`,
    );
  }
}

let result = "";
if (values["full-html"] && format === "html") {
  result += "<!DOCTYPE html>\n<html>\n<head>\n";
  result += `<title>${values["html-title"] || ""}</title>\n`;
  result += '<meta name="generator" content="md4x">\n';
  result += '<meta charset="UTF-8">\n';
  if (values["html-css"]) {
    result += `<link rel="stylesheet" href="${values["html-css"]}">\n`;
  }
  result += "</head>\n<body>\n";
  result += output;
  result += "</body>\n</html>\n";
} else {
  result = output;
}

if (values.output && values.output !== "-") {
  try {
    writeFileSync(values.output, result);
  } catch {
    process.stderr.write(`Cannot open ${values.output}.\n`);
    process.exit(1);
  }
} else {
  process.stdout.write(result);
}

// --- internal helpers ---

/** Convert GitHub web URLs to raw content URLs */
function toRawUrl(url) {
  const u = new URL(url);
  if (u.hostname === "github.com" || u.hostname === "www.github.com") {
    // github.com/:owner/:repo/blob/:ref/path → raw.githubusercontent.com/:owner/:repo/:ref/path
    const blob = u.pathname.match(/^\/([^/]+\/[^/]+)\/blob\/(.+)/);
    if (blob) {
      return `https://raw.githubusercontent.com/${blob[1]}/${blob[2]}`;
    }
    // github.com/:owner/:repo → raw.githubusercontent.com/:owner/:repo/HEAD/README.md
    const repo = u.pathname.match(/^\/([^/]+\/[^/]+)\/?$/);
    if (repo) {
      return `https://raw.githubusercontent.com/${repo[1]}/HEAD/README.md`;
    }
  }
  // gist.github.com/:user/:id → gist.githubusercontent.com/:user/:id/raw
  if (u.hostname === "gist.github.com") {
    return `https://gist.githubusercontent.com${u.pathname}/raw`;
  }
  return url;
}
