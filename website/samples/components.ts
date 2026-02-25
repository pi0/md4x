import { defineComponent, h, ref } from "vue";
import { createElement, useState } from "react";

// --- Vue counter component ---
const VueCounter = defineComponent({
  props: { start: { type: Number, default: 0 } },
  setup(props) {
    const count = ref(props.start);
    return () =>
      h(
        "button",
        {
          class: "counter-btn",
          onClick: () => count.value++,
        },
        `Count: ${count.value}`,
      );
  },
});

export const vueComponents: Record<string, any> = {
  "click-counter": VueCounter,
};

// --- React counter component ---
function ReactCounter({ start = 0 }: { start?: number }) {
  const [count, setCount] = useState(start);
  return createElement(
    "button",
    {
      className: "counter-btn",
      onClick: () => setCount((c: number) => c + 1),
    },
    `Count: ${count}`,
  );
}

export const reactComponents: Record<string, any> = {
  "click-counter": ReactCounter,
};
