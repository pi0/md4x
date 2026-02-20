<script setup lang="ts">
import { ref, onMounted, watch } from "vue";
import { useRoute, useRouter } from "vue-router";
import {
  init as initMD4x,
  renderToHtml,
  parseAST,
  renderToAnsi,
  parseMeta,
} from "md4x/wasm";
import { createHighlighter, type Highlighter } from "shiki";
import TabSelect from "../components/TabSelect.vue";
import { ansiToHtml } from "../utils/ansi.ts";

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

const allExamplesMd = Object.values(examples).join("\n\n---\n\n");
const exampleOptions = [
  { value: "all", label: "All" },
  ...Object.keys(examples).map((key) => ({
    value: key,
    label: key
      .replace(/^\d+\./, "")
      .split("-")
      .map((w) => w[0].toUpperCase() + w.slice(1))
      .join(" "),
  })),
];
const modeOptions = [
  { value: "html", label: "HTML" },
  { value: "raw", label: "Source" },
  { value: "json", label: "AST" },
  { value: "ansi", label: "ANSI" },
  { value: "meta", label: "Meta" },
];
const mode = ref("html");
const example = ref("all");
const currentMd = ref(allExamplesMd);
const outputHtml = ref("");
const mobileTab = ref<"editor" | "output">("output");

const editorEl = ref<HTMLElement>();
const outputEl = ref<HTMLElement>();
let editorView: any = null;
let highlighter: Highlighter;

// Restore state from query
function restoreFromQuery() {
  const m = route.query.m as string;
  const e = route.query.e as string;
  const c = route.query.c as string;
  if (m) mode.value = m;
  if (e === "all") {
    example.value = "all";
    currentMd.value = allExamplesMd;
  } else if (e && examples[e]) {
    example.value = e;
    currentMd.value = examples[e];
  }
  if (c) currentMd.value = atob(c);
}

function updateQuery() {
  const query: Record<string, string> = {};
  if (mode.value !== "html") query.m = mode.value;
  if (example.value !== "all") query.e = example.value;
  const exampleMd =
    example.value === "all" ? allExamplesMd : examples[example.value];
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
    } else if (m === "meta") {
      outputHtml.value = highlighter.codeToHtml(
        JSON.stringify(parseMeta(md), null, 2),
        { lang: "json", theme: "github-light" },
      );
    }
  } catch (e) {
    outputHtml.value = `<pre>Error: ${(e as Error).message}</pre>`;
  }
  updateQuery();
}

// --- Scroll sync: editor → preview ---
let syncSource: "editor" | "preview" | null = null;
let syncLock = 0;

function stripMarkdown(line: string): string {
  return line
    .replace(/^#{1,6}\s+/, "")
    .replace(/\*{1,3}([^*]+)\*{1,3}/g, "$1")
    .replace(/_{1,3}([^_]+)_{1,3}/g, "$1")
    .replace(/`([^`]+)`/g, "$1")
    .replace(/!\[([^\]]*)\]\([^)]*\)/g, "$1")
    .replace(/\[([^\]]*)\]\([^)]*\)/g, "$1")
    .replace(/~~([^~]+)~~/g, "$1")
    .replace(/<[^>]+>/g, "")
    .trim();
}

function findInPreview(lineText: string): HTMLElement | null {
  const el = outputEl.value;
  if (!el || !lineText) return null;
  const plain = stripMarkdown(lineText);
  if (!plain || plain.length < 2) return null;
  const escaped = plain.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  const re = new RegExp(escaped.slice(0, 60));
  const walker = document.createTreeWalker(el, NodeFilter.SHOW_TEXT);
  let node: Text | null;
  while ((node = walker.nextNode() as Text | null)) {
    if (re.test(node.textContent || "")) {
      return node.parentElement;
    }
  }
  return null;
}

function scrollPreviewTo(target: HTMLElement) {
  if (syncSource === "preview") return;
  syncSource = "editor";
  clearTimeout(syncLock);
  target.scrollIntoView({ behavior: "smooth", block: "center" });
  syncLock = window.setTimeout(() => {
    syncSource = null;
  }, 300);
}

function getFirstVisibleLine(): string | null {
  if (!editorView) return null;
  const rect = editorView.dom.getBoundingClientRect();
  // Get the document position at the top of the visible area
  const topPos = editorView.posAtCoords({ x: rect.left + 1, y: rect.top + 1 });
  if (topPos == null) return null;
  const line = editorView.state.doc.lineAt(topPos);
  // Skip empty lines — scan forward to find first non-empty
  const doc = editorView.state.doc;
  for (let i = line.number; i <= doc.lines; i++) {
    const l = doc.line(i);
    const text = l.text.trim();
    if (text && text.length >= 2) return l.text;
  }
  return line.text;
}

// Debounced scroll handler for the editor's scroller
let editorScrollTimer: ReturnType<typeof setTimeout> | undefined;
function onEditorScroll() {
  if (syncSource === "preview") return;
  // Skip sync when near top of editor
  const scroller = editorView?.scrollDOM;
  if (!scroller || scroller.scrollTop < 50) return;
  clearTimeout(editorScrollTimer);
  editorScrollTimer = setTimeout(() => {
    const lineText = getFirstVisibleLine();
    if (lineText) {
      const target = findInPreview(lineText);
      if (target) scrollPreviewTo(target);
    }
  }, 100);
}

// Cursor/selection change handler
let cursorSyncTimer: ReturnType<typeof setTimeout> | undefined;
function onCursorActivity(update: any) {
  if (!update.selectionSet) return;
  clearTimeout(cursorSyncTimer);
  cursorSyncTimer = setTimeout(() => {
    const pos = update.state.selection.main.head;
    const line = update.state.doc.lineAt(pos);
    const target = findInPreview(line.text);
    if (target) scrollPreviewTo(target);
  }, 150);
}

watch(mode, () => {
  mobileTab.value = "output";
  render();
});
watch(example, () => {
  currentMd.value =
    example.value === "all" ? allExamplesMd : examples[example.value];
  if (editorView) {
    editorView.dispatch({
      changes: {
        from: 0,
        to: editorView.state.doc.length,
        insert: currentMd.value,
      },
    });
  }
  mobileTab.value = "editor";
  render();
});

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
    initMD4x(),
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
          onCursorActivity(update);
        }),
        EditorView.lineWrapping,
        EditorView.domEventHandlers({
          scroll: onEditorScroll,
        }),
      ],
    }),
    parent: editorEl.value!,
  });

  render();
});
</script>

<template>
  <div
    class="flex flex-wrap items-center justify-center gap-2 border-b border-gray-300 bg-gray-50 px-4 py-1.5 md:gap-3"
  >
    <TabSelect v-model="example" :options="exampleOptions" />
    <TabSelect v-model="mode" :options="modeOptions" />
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
      ref="outputEl"
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
