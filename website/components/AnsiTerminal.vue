<script setup lang="ts">
import { ref, watch, onMounted, onBeforeUnmount, nextTick } from "vue";
import { Terminal } from "@xterm/xterm";
import { FitAddon } from "@xterm/addon-fit";
import "@xterm/xterm/css/xterm.css";

const props = defineProps<{
  content: string;
}>();

const containerEl = ref<HTMLElement>();
let terminal: Terminal | null = null;
let fitAddon: FitAddon | null = null;
let resizeObserver: ResizeObserver | null = null;

function writeContent(content: string) {
  if (!terminal) return;
  terminal.reset();
  // xterm.js needs \r\n for newlines (convertEol handles this)
  terminal.write(content);
}

onMounted(() => {
  terminal = new Terminal({
    disableStdin: true,
    convertEol: true,
    cursorStyle: "bar",
    cursorWidth: 0,
    cursorInactiveStyle: "none",
    scrollback: 10000,
    fontFamily: "'JetBrains Mono', 'Fira Code', 'Cascadia Code', monospace",
    fontSize: 13,
    lineHeight: 1.5,
    letterSpacing: 0,
    theme: {
      foreground: "#c8cad8",
      background: "#0f0f1a",
      cursor: "transparent",
      selectionBackground: "#3a3a5c",
      black: "#545464",
      red: "#ff6b6b",
      green: "#69db7c",
      yellow: "#ffd43b",
      blue: "#74c0fc",
      magenta: "#da77f2",
      cyan: "#66d9e8",
      white: "#c8cad8",
      brightBlack: "#686878",
      brightRed: "#ff8787",
      brightGreen: "#8ce99a",
      brightYellow: "#ffe066",
      brightBlue: "#91d5ff",
      brightMagenta: "#e599f7",
      brightCyan: "#99e9f2",
      brightWhite: "#e4e5eb",
    },
    linkHandler: {
      activate(_event: MouseEvent, uri: string) {
        window.open(uri, "_blank", "noopener");
      },
    },
  });

  fitAddon = new FitAddon();
  terminal.loadAddon(fitAddon);
  terminal.open(containerEl.value!);

  nextTick(() => {
    fitAddon!.fit();
  });

  resizeObserver = new ResizeObserver(() => {
    fitAddon?.fit();
  });
  resizeObserver.observe(containerEl.value!);

  writeContent(props.content);
});

watch(
  () => props.content,
  (content) => writeContent(content),
);

onBeforeUnmount(() => {
  resizeObserver?.disconnect();
  terminal?.dispose();
  terminal = null;
  fitAddon = null;
  resizeObserver = null;
});
</script>

<template>
  <div ref="containerEl" class="ansi-terminal" />
</template>

<style scoped>
.ansi-terminal {
  width: 100%;
  height: 100%;
  background: #0f0f1a;
}
.ansi-terminal :deep(.xterm) {
  padding: 12px;
  height: 100%;
}
.ansi-terminal :deep(.xterm-viewport) {
  background: #0f0f1a !important;
}
</style>
