import { defineConfig } from "vite";
import tailwindcss from "@tailwindcss/vite";
import vue from "@vitejs/plugin-vue";

export default defineConfig({
  // base: "/md4x/",
  plugins: [tailwindcss(), vue()],
  build: {
    target: "esnext",
  },
});
