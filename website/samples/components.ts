import { defineComponent, h, ref, type VNode } from "vue";
import { createElement, useState, type ReactNode } from "react";

/** Map color prop names to hex values. */
const colorMap: Record<string, string> = {
  blue: "%233b82f6",
  green: "%2322c55e",
  purple: "%238b5cf6",
  red: "%23ef4444",
  yellow: "%23eab308",
  gray: "%239ca3af",
};

/**
 * Convert an icon value to an Iconify API URL.
 * Supports "prefix:name" format (e.g. "mdi:folder").
 * Returns undefined for plain emoji/text icons.
 */
function iconifyUrl(icon: string, color?: string): string | undefined {
  const match = icon.match(/^([a-z][a-z0-9-]*):([a-z][a-z0-9-]*)$/i);
  if (!match) return undefined;
  const hex = color ? colorMap[color] : undefined;
  return `https://api.iconify.design/${match[1]}/${match[2]}.svg${hex ? `?color=${hex}` : ""}`;
}

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

// --- Vue card-group component ---
const VueCardGroup = defineComponent({
  name: "CardGroup",
  setup(_, { slots }) {
    return () => h("div", { class: "card-group" }, slots.default?.() || []);
  },
});

// --- Vue card component ---
const VueCard = defineComponent({
  name: "Card",
  props: {
    icon: { type: String, default: "" },
    title: { type: String, default: "" },
    to: { type: String, default: "" },
    color: { type: String, default: "" },
  },
  setup(props, { slots }) {
    return () => {
      const children: VNode[] = [];

      if (props.icon || props.title) {
        const url = props.icon
          ? iconifyUrl(props.icon, props.color)
          : undefined;
        children.push(
          h("div", { class: "card-header" }, [
            url
              ? h("img", {
                  class: "card-icon",
                  src: url,
                  alt: "",
                  width: 24,
                  height: 24,
                })
              : props.icon
                ? h("span", { class: "card-icon" }, props.icon)
                : null,
            props.title
              ? h("span", { class: "card-title" }, props.title)
              : null,
          ]),
        );
      }

      children.push(h("div", { class: "card-body" }, slots.default?.() || []));

      const card = h(
        "div",
        {
          class: ["card-component", props.color ? `card-${props.color}` : ""],
        },
        children,
      );

      if (props.to) {
        return h("a", { href: props.to, class: "card-link" }, [card]);
      }
      return card;
    };
  },
});

export const vueComponents: Record<string, any> = {
  "click-counter": VueCounter,
  "card-group": VueCardGroup,
  card: VueCard,
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

// --- React card-group component ---
function ReactCardGroup({ children }: { children?: ReactNode }) {
  return createElement("div", { className: "card-group" }, children);
}

// --- React card component ---
function ReactCard({
  icon,
  title,
  to,
  color,
  children,
}: {
  icon?: string;
  title?: string;
  to?: string;
  color?: string;
  children?: ReactNode;
}) {
  const url = icon ? iconifyUrl(icon, color) : undefined;
  const header =
    icon || title
      ? createElement("div", { className: "card-header" }, [
          url
            ? createElement("img", {
                className: "card-icon",
                src: url,
                alt: "",
                width: 24,
                height: 24,
                key: "icon",
              })
            : icon
              ? createElement(
                  "span",
                  { className: "card-icon", key: "icon" },
                  icon,
                )
              : null,
          title
            ? createElement(
                "span",
                { className: "card-title", key: "title" },
                title,
              )
            : null,
        ])
      : null;

  const card = createElement(
    "div",
    {
      className: `card-component${color ? ` card-${color}` : ""}`,
    },
    [
      header,
      createElement("div", { className: "card-body", key: "body" }, children),
    ],
  );

  if (to) {
    return createElement("a", { href: to, className: "card-link" }, card);
  }
  return card;
}

export const reactComponents: Record<string, any> = {
  "click-counter": ReactCounter,
  "card-group": ReactCardGroup,
  card: ReactCard,
};
