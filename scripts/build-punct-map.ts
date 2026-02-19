import { buildUnicodeMap } from "./_unicode-map.ts";

buildUnicodeMap({
  filter: (cat) => cat[0] === "P" || cat[0] === "S",
  arrayName: "PUNCT_MAP",
});
