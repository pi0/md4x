<script lang="ts">
import {
  Comment,
  defineComponent,
  h,
  type Component,
  type PropType,
} from "vue";
import type {
  ComarkElementAttributes,
  ComarkNode,
  ComarkTree,
} from "md4x/wasm";

function parseDynamicPropValue(value: unknown): unknown {
  if (typeof value !== "string") return value;
  if (value === "true") return true;
  if (value === "false") return false;
  if (value === "null") return null;
  try {
    return JSON.parse(value);
  } catch {
    return value;
  }
}

function cssStringToObject(css: string): Record<string, string> {
  return css
    .split(";")
    .filter(Boolean)
    .reduce(
      (acc, rule) => {
        const [prop, rawValue] = rule.split(":");
        if (!prop || !rawValue) return acc;
        const key = prop.trim();
        const value = rawValue.trim();
        if (!key || !value) return acc;
        const normalizedKey = key.startsWith("--")
          ? key
          : key.replace(/-([a-z])/g, (_, c: string) => c.toUpperCase());
        acc[normalizedKey] = value;
        return acc;
      },
      {} as Record<string, string>,
    );
}

function normalizeComarkProps(
  nodeProps: ComarkElementAttributes,
): Record<string, unknown> {
  const normalizedProps: Record<string, unknown> = {};
  for (const [key, value] of Object.entries(nodeProps || {})) {
    if (key === "className") {
      normalizedProps.class = value;
    } else if (key === "style" && typeof value === "string") {
      normalizedProps.style = cssStringToObject(value);
    } else if (key.startsWith(":")) {
      normalizedProps[key.slice(1)] = parseDynamicPropValue(value);
    } else {
      normalizedProps[key] = value;
    }
  }
  return normalizedProps;
}

function renderComarkNode(
  node: ComarkNode,
  components: Record<string, Component>,
  key?: number,
): unknown {
  if (typeof node === "string") return node;
  if (!Array.isArray(node)) return null;

  const [tag, nodeProps, ...children] = node;
  if (tag == null) {
    return h(
      Comment,
      children
        .filter((child): child is string => typeof child === "string")
        .join(""),
    );
  }

  const normalizedProps = normalizeComarkProps(nodeProps || {});
  if (key !== undefined) normalizedProps.key = key;

  const renderedChildren = children
    .map((child, index) => renderComarkNode(child, components, index))
    .filter((child) => child !== null);

  const component = components[tag];
  return h(component || tag, normalizedProps, renderedChildren);
}

export default defineComponent({
  name: "ComarkVueRenderer",
  props: {
    tree: {
      type: Object as PropType<ComarkTree | null>,
      default: null,
    },
    components: {
      type: Object as PropType<Record<string, Component>>,
      default: () => ({}),
    },
  },
  render() {
    if (!this.tree) return null;
    return h(
      "div",
      { class: "comark-content" },
      this.tree.nodes
        .map((node, index) => renderComarkNode(node, this.components, index))
        .filter((child) => child !== null),
    );
  },
});
</script>
