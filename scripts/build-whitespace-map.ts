import { buildUnicodeMap } from "./_unicode-map.ts";

buildUnicodeMap({
  filter: (cat) => cat === "Zs",
  arrayName: "WHITESPACE_MAP",
});
