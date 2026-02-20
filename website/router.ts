import { createRouter, createWebHashHistory } from "vue-router";

export const router = createRouter({
  history: createWebHashHistory("/md4x/"),
  routes: [
    {
      path: "/",
      component: () => import("./pages/readme.vue"),
    },
    {
      path: "/playground",
      component: () => import("./pages/playground.vue"),
    },
  ],
});
