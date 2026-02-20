const ansiColors: Record<number, string | null> = {
  30: "#545464",
  31: "#ff6b6b",
  32: "#69db7c",
  33: "#ffd43b",
  34: "#74c0fc",
  35: "#da77f2",
  36: "#66d9e8",
  37: "#c8cad8",
  39: null,
  90: "#686878",
  91: "#ff8787",
  92: "#8ce99a",
  93: "#ffe066",
  94: "#91d5ff",
  95: "#e599f7",
  96: "#99e9f2",
  97: "#e4e5eb",
};

export function ansiToHtml(str: string): string {
  const esc = (s: string) =>
    s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  let out = "",
    styles = {
      bold: false,
      dim: false,
      italic: false,
      underline: false,
      strike: false,
      color: null as string | null,
    };
  const parts = str.split(/\x1b\[([\d;]*)m/);
  for (let i = 0; i < parts.length; i++) {
    if (i % 2 === 0) {
      const text = parts[i];
      if (!text) continue;
      const css: string[] = [];
      if (styles.bold) css.push("font-weight:bold");
      if (styles.dim) css.push("opacity:0.6");
      if (styles.italic) css.push("font-style:italic");
      if (styles.underline) css.push("text-decoration:underline");
      if (styles.strike) css.push("text-decoration:line-through");
      if (styles.color) css.push(`color:${styles.color}`);
      out += css.length
        ? `<span style="${css.join(";")}">${esc(text)}</span>`
        : esc(text);
    } else {
      for (const c of (parts[i] || "0").split(";")) {
        const n = Number(c);
        if (n === 0) {
          styles = {
            bold: false,
            dim: false,
            italic: false,
            underline: false,
            strike: false,
            color: null,
          };
        } else if (n === 1) styles.bold = true;
        else if (n === 2) styles.dim = true;
        else if (n === 3) styles.italic = true;
        else if (n === 4) styles.underline = true;
        else if (n === 9) styles.strike = true;
        else if (n === 22) {
          styles.bold = false;
          styles.dim = false;
        } else if (n === 23) styles.italic = false;
        else if (n === 24) styles.underline = false;
        else if (n === 29) styles.strike = false;
        else if (ansiColors[n] !== undefined) styles.color = ansiColors[n];
      }
    }
  }
  return out;
}
