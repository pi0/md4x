<script setup lang="ts">
import { ref, onMounted } from "vue";
import { initWasm, renderToHtml } from "md4x/wasm";
import readmeRaw from "../../README.md?raw";

const html = ref("");

onMounted(async () => {
  await initWasm();
  html.value = renderToHtml(readmeRaw);
});
</script>

<template>
  <main class="flex-1 overflow-auto">
    <div
      class="prose mx-auto w-full max-w-none px-4 py-8 max-md:px-4 max-md:py-5 md:max-w-4xl md:px-10"
      v-html="html"
    />
  </main>
</template>
