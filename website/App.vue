<script setup lang="ts">
import { RouterLink, RouterView } from "vue-router";
</script>

<template>
  <header
    class="flex flex-wrap items-center gap-3 bg-header px-4 py-2 text-gray-200"
  >
    <h1 class="text-base font-semibold max-md:hidden">
      <RouterLink to="/" class="text-inherit no-underline">md4x</RouterLink>
    </h1>
    <nav class="flex flex-1 flex-wrap items-center justify-center gap-1">
      <RouterLink
        to="/"
        class="rounded px-3 py-1 text-[13px] text-gray-400 no-underline transition-colors hover:text-gray-200"
        exact-active-class="!bg-header-active !text-white"
        >DOCS</RouterLink
      >
      <RouterLink
        to="/playground"
        class="rounded px-3 py-1 text-[13px] text-gray-400 no-underline transition-colors hover:text-gray-200"
        active-class="!bg-header-active !text-white"
        >Playground</RouterLink
      >
    </nav>
    <a
      href="https://github.com/pi0/md4x"
      target="_blank"
      class="flex text-gray-400 transition-colors hover:text-white"
      title="GitHub"
    >
      <svg width="20" height="20" viewBox="0 0 16 16" fill="currentColor">
        <path
          d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27s1.36.09 2 .27c1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0016 8c0-4.42-3.58-8-8-8z"
        />
      </svg>
    </a>
  </header>
  <Suspense>
    <RouterView />
  </Suspense>
</template>
