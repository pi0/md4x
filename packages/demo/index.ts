import { EditorView, basicSetup } from "codemirror";
import { markdown } from "@codemirror/lang-markdown";
import { languages } from "@codemirror/language-data";
import { EditorState } from "@codemirror/state";
import { initWasm, renderToHtml, parseAST, renderToAnsi } from "md4x/wasm";
import { createHighlighter } from "shiki";
const examples = Object.fromEntries(
  Object.entries(
    import.meta.glob("./samples/*.md", {
      eager: true,
      query: "?raw",
      import: "default",
    }),
  ).map(([path, content]) => [
    path.replace("./samples/", "").replace(".md", ""),
    content,
  ]),
) as Record<string, string>;

const ansiColors: Record<number, string | null> = {
  30: "#545464",
  31: "#ff6b6b",
  32: "#69db7c",
  33: "#ffd43b",
  34: "#74c0fc",
  35: "#da77f2",
  36: "#66d9e8",
  37: "#c8cad8",
  39: null, // default
  90: "#686878",
  91: "#ff8787",
  92: "#8ce99a",
  93: "#ffe066",
  94: "#91d5ff",
  95: "#e599f7",
  96: "#99e9f2",
  97: "#e4e5eb",
};

function ansiToHtml(str: string): string {
  const esc = (s: string) =>
    s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  let out = "",
    styles = {
      bold: false,
      dim: false,
      italic: false,
      underline: false,
      strike: false,
      color: null as string | null,
    };
  const parts = str.split(/\x1b\[([\d;]*)m/);
  for (let i = 0; i < parts.length; i++) {
    if (i % 2 === 0) {
      const text = parts[i];
      if (!text) continue;
      const css: string[] = [];
      if (styles.bold) css.push("font-weight:bold");
      if (styles.dim) css.push("opacity:0.6");
      if (styles.italic) css.push("font-style:italic");
      if (styles.underline) css.push("text-decoration:underline");
      if (styles.strike) css.push("text-decoration:line-through");
      if (styles.color) css.push(`color:${styles.color}`);
      out += css.length
        ? `<span style="${css.join(";")}">${esc(text)}</span>`
        : esc(text);
    } else {
      for (const c of (parts[i] || "0").split(";")) {
        const n = Number(c);
        if (n === 0) {
          styles = {
            bold: false,
            dim: false,
            italic: false,
            underline: false,
            strike: false,
            color: null,
          };
        } else if (n === 1) styles.bold = true;
        else if (n === 2) styles.dim = true;
        else if (n === 3) styles.italic = true;
        else if (n === 4) styles.underline = true;
        else if (n === 9) styles.strike = true;
        else if (n === 22) {
          styles.bold = false;
          styles.dim = false;
        } else if (n === 23) styles.italic = false;
        else if (n === 24) styles.underline = false;
        else if (n === 29) styles.strike = false;
        else if (ansiColors[n] !== undefined) styles.color = ansiColors[n];
      }
    }
  }
  return out;
}

const output = document.getElementById("output")!;
const modeEl = document.getElementById("mode") as HTMLSelectElement;
const exampleEl = document.getElementById("example") as HTMLSelectElement;

for (const key of Object.keys(examples)) {
  const opt = document.createElement("option");
  opt.value = key;
  opt.textContent = key
    .split("-")
    .map((w) => w[0].toUpperCase() + w.slice(1))
    .join(" ");
  exampleEl.append(opt);
}

// Restore state from URL hash
let initialMd = examples.basics;
const hash = location.hash.slice(1);
if (hash) {
  const params = new URLSearchParams(hash);
  const m = params.get("m");
  const e = params.get("e");
  const c = params.get("c");
  if (m) modeEl.value = m;
  if (e && examples[e]) {
    exampleEl.value = e;
    initialMd = examples[e];
  }
  if (c) initialMd = atob(c);
}

// Init WASM and Shiki in parallel
const [, highlighter] = await Promise.all([
  initWasm(),
  createHighlighter({
    themes: ["github-light"],
    langs: ["html", "json"],
  }),
]);

let currentMd = initialMd;

// CodeMirror dark theme
const editorTheme = EditorView.theme(
  {
    "&": { height: "100%" },
    ".cm-scroller": { overflow: "auto" },
  },
  { dark: true },
);

const editor = new EditorView({
  state: EditorState.create({
    doc: initialMd,
    extensions: [
      basicSetup,
      markdown({ codeLanguages: languages }),
      editorTheme,
      EditorView.updateListener.of((update) => {
        if (update.docChanged) {
          currentMd = update.state.doc.toString();
          render();
        }
      }),
    ],
  }),
  parent: document.getElementById("editor")!,
});

function updateHash() {
  const params = new URLSearchParams();
  if (modeEl.value !== "html") params.set("m", modeEl.value);
  if (exampleEl.value !== "basics") params.set("e", exampleEl.value);
  const exampleMd = examples[exampleEl.value];
  if (currentMd !== exampleMd) params.set("c", btoa(currentMd));
  history.replaceState(
    null,
    "",
    params.size ? `#${params}` : location.pathname,
  );
}

function render() {
  const md = currentMd;
  const m = modeEl.value;
  output.className = `mode-${m}`;
  try {
    if (m === "html") {
      output.innerHTML = renderToHtml(md);
    } else if (m === "raw") {
      output.innerHTML = highlighter.codeToHtml(renderToHtml(md), {
        lang: "html",
        theme: "github-light",
      });
    } else if (m === "json") {
      output.innerHTML = highlighter.codeToHtml(
        JSON.stringify(parseAST(md), null, 2),
        { lang: "json", theme: "github-light" },
      );
    } else if (m === "ansi") {
      const ansi = renderToAnsi(md).replace(/\x1b\]8;[^\x1b]*\x1b\\/g, "");
      output.innerHTML = ansiToHtml(ansi);
    }
  } catch (e) {
    output.textContent = `Error: ${(e as Error).message}`;
  }
  updateHash();
}

document.getElementById("reset")!.addEventListener("click", (e) => {
  e.preventDefault();
  modeEl.value = "html";
  exampleEl.value = "basics";
  currentMd = examples.basics;
  editor.dispatch({
    changes: { from: 0, to: editor.state.doc.length, insert: currentMd },
  });
  history.replaceState(null, "", location.pathname);
  render();
});

modeEl.addEventListener("change", render);
exampleEl.addEventListener("change", () => {
  const newMd = examples[exampleEl.value];
  currentMd = newMd;
  editor.dispatch({
    changes: { from: 0, to: editor.state.doc.length, insert: newMd },
  });
  updateHash();
  render();
});

// Mobile tab switching
const mainEl = document.querySelector("main")!;
for (const btn of document.querySelectorAll<HTMLButtonElement>(
  ".mobile-tabs button",
)) {
  btn.addEventListener("click", () => {
    (mainEl as HTMLElement).dataset.tab = btn.dataset.for;
    for (const b of document.querySelectorAll(".mobile-tabs button"))
      b.classList.toggle("active", b === btn);
  });
}

render();
