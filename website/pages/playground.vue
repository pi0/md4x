<script setup lang="ts">
import { ref, onMounted, watch } from "vue";
import { useRoute, useRouter } from "vue-router";
import { initWasm, renderToHtml, parseAST, renderToAnsi } from "md4x/wasm";
import { createHighlighter, type Highlighter } from "shiki";

const examples = Object.fromEntries(
  Object.entries(
    import.meta.glob("../samples/*.md", {
      eager: true,
      query: "?raw",
      import: "default",
    }),
  ).map(([path, content]) => [
    path.replace("../samples/", "").replace(".md", ""),
    content,
  ]),
) as Record<string, string>;

const route = useRoute();
const router = useRouter();

const mode = ref("html");
const example = ref("basics");
const currentMd = ref(examples.basics);
const outputHtml = ref("");
const mobileTab = ref<"editor" | "output">("output");

const editorEl = ref<HTMLElement>();
let editorView: any = null;
let highlighter: Highlighter;

// Restore state from query
function restoreFromQuery() {
  const m = route.query.m as string;
  const e = route.query.e as string;
  const c = route.query.c as string;
  if (m) mode.value = m;
  if (e && examples[e]) {
    example.value = e;
    currentMd.value = examples[e];
  }
  if (c) currentMd.value = atob(c);
}

function updateQuery() {
  const query: Record<string, string> = {};
  if (mode.value !== "html") query.m = mode.value;
  if (example.value !== "basics") query.e = example.value;
  const exampleMd = examples[example.value];
  if (currentMd.value !== exampleMd) query.c = btoa(currentMd.value);
  router.replace({ query });
}

function render() {
  const md = currentMd.value;
  const m = mode.value;
  try {
    if (m === "html") {
      outputHtml.value = renderToHtml(md);
    } else if (m === "raw") {
      outputHtml.value = highlighter.codeToHtml(renderToHtml(md), {
        lang: "html",
        theme: "github-light",
      });
    } else if (m === "json") {
      outputHtml.value = highlighter.codeToHtml(
        JSON.stringify(parseAST(md), null, 2),
        { lang: "json", theme: "github-light" },
      );
    } else if (m === "ansi") {
      const ansi = renderToAnsi(md).replace(/\x1b\]8;[^\x1b]*\x1b\\/g, "");
      outputHtml.value = ansiToHtml(ansi);
    }
  } catch (e) {
    outputHtml.value = `<pre>Error: ${(e as Error).message}</pre>`;
  }
  updateQuery();
}

function onExampleChange() {
  currentMd.value = examples[example.value];
  if (editorView) {
    editorView.dispatch({
      changes: {
        from: 0,
        to: editorView.state.doc.length,
        insert: currentMd.value,
      },
    });
  }
  render();
}

watch(mode, render);

onMounted(async () => {
  restoreFromQuery();

  const [
    ,
    hl,
    { EditorView, basicSetup },
    { markdown },
    { languages },
    { EditorState },
  ] = await Promise.all([
    initWasm(),
    createHighlighter({
      themes: ["github-light"],
      langs: ["html", "json"],
    }),
    import("codemirror"),
    import("@codemirror/lang-markdown"),
    import("@codemirror/language-data"),
    import("@codemirror/state"),
  ]);

  highlighter = hl;

  const editorTheme = EditorView.theme({
    "&": { height: "100%" },
    ".cm-scroller": { overflow: "auto" },
  });

  editorView = new EditorView({
    state: EditorState.create({
      doc: currentMd.value,
      extensions: [
        basicSetup,
        markdown({ codeLanguages: languages }),
        editorTheme,
        EditorView.updateListener.of((update: any) => {
          if (update.docChanged) {
            currentMd.value = update.state.doc.toString();
            render();
          }
        }),
      ],
    }),
    parent: editorEl.value!,
  });

  render();
});

// --- ANSI helpers ---
const ansiColors: Record<number, string | null> = {
  30: "#545464",
  31: "#ff6b6b",
  32: "#69db7c",
  33: "#ffd43b",
  34: "#74c0fc",
  35: "#da77f2",
  36: "#66d9e8",
  37: "#c8cad8",
  39: null,
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
</script>

<template>
  <div
    class="flex flex-wrap items-center justify-center gap-2 border-b border-gray-300 bg-gray-50 px-4 py-1.5 md:gap-3"
  >
    <select
      v-model="example"
      class="min-w-0 flex-1 basis-0 rounded border border-gray-300 bg-white px-2 py-1 text-[13px] text-gray-700"
      @change="onExampleChange"
    >
      <option v-for="(_, key) in examples" :key="key" :value="key">
        {{
          key
            .split("-")
            .map((w) => w[0].toUpperCase() + w.slice(1))
            .join(" ")
        }}
      </option>
    </select>
    <select
      v-model="mode"
      class="min-w-0 flex-1 basis-0 rounded border border-gray-300 bg-white px-2 py-1 text-[13px] text-gray-700"
    >
      <option value="html">HTML (rendered)</option>
      <option value="raw">HTML (source)</option>
      <option value="json">JSON AST (comark)</option>
      <option value="ansi">ANSI (CLI)</option>
    </select>
    <div class="mobile-tabs w-full border-b border-gray-300 -mx-4 -mb-1.5 mt-1">
      <button
        class="flex-1 cursor-pointer border-b-2 bg-transparent py-1.5 font-inherit text-[13px] transition-colors"
        :class="
          mobileTab === 'editor'
            ? 'border-gray-800 text-gray-800 font-medium'
            : 'border-transparent text-gray-400'
        "
        @click="mobileTab = 'editor'"
      >
        Editor
      </button>
      <button
        class="flex-1 cursor-pointer border-b-2 bg-transparent py-1.5 font-inherit text-[13px] transition-colors"
        :class="
          mobileTab === 'output'
            ? 'border-gray-800 text-gray-800 font-medium'
            : 'border-transparent text-gray-400'
        "
        @click="mobileTab = 'output'"
      >
        Output
      </button>
    </div>
  </div>
  <main class="flex min-h-0 flex-1 flex-col md:flex-row" :data-tab="mobileTab">
    <div
      ref="editorEl"
      class="editor w-full min-w-0 min-h-0 flex-1 basis-0 overflow-auto md:border-r md:border-gray-300"
      :data-hidden="mobileTab === 'output' ? '' : undefined"
    />
    <div
      class="output min-w-0 min-h-0 flex-1 basis-0 overflow-auto break-words text-sm leading-relaxed"
      :class="[
        `mode-${mode}`,
        mode === 'html' && 'prose max-w-none p-5 px-6',
        mode === 'ansi' &&
          'whitespace-pre-wrap break-words bg-linear-to-br from-[#0f0f1a] to-[#161625] p-5 px-6 font-mono text-[13.5px] leading-[1.65] text-[#c8cad8]',
      ]"
      :data-hidden="mobileTab === 'editor' ? '' : undefined"
      v-html="outputHtml"
    />
  </main>
</template>

<style scoped>
.mobile-tabs {
  display: flex;
}
@media (min-width: 768px) {
  .mobile-tabs {
    display: none;
  }
}
</style>
