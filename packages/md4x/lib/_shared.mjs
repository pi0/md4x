const decoder = new TextDecoder();

export function parseHtmlMeta(bytes) {
  const nullIdx = bytes.indexOf(0);
  if (nullIdx === -1) {
    return { html: decoder.decode(bytes), codeBlocks: [] };
  }
  const htmlBytes = bytes.subarray(0, nullIdx);
  const metaBytes = bytes.subarray(nullIdx + 1);
  const html = decoder.decode(htmlBytes);
  const meta = JSON.parse(decoder.decode(metaBytes));
  const codeBlocks = meta.map((m) => {
    const start = decoder.decode(htmlBytes.subarray(0, m.s)).length;
    const end = decoder.decode(htmlBytes.subarray(0, m.e)).length;
    const block = { start, end, lang: m.l || "" };
    if (m.f) block.filename = m.f;
    if (m.h) block.highlights = m.h;
    return block;
  });
  return { html, codeBlocks };
}

export function parseHtmlWithHighlighting(bytes, highlighter) {
  const { html, codeBlocks } = parseHtmlMeta(bytes);
  if (codeBlocks.length === 0) return html;
  let out = "";
  let pos = 0;
  for (const block of codeBlocks) {
    const code = unescapeHtml(html.slice(block.start, block.end));
    const highlighted = highlighter(code, block);
    if (highlighted === undefined) {
      out += html.slice(pos, block.end);
    } else {
      // Calculate <pre><code...> prefix length to replace the full wrapper
      const preLen = block.lang
        ? '<pre><code class="language-'.length + block.lang.length + '">'.length
        : "<pre><code>".length;
      out += html.slice(pos, block.start - preLen);
      out += highlighted;
      pos = block.end + "</code></pre>\n".length;
      continue;
    }
    pos = block.end;
  }
  out += html.slice(pos);
  return out;
}

function unescapeHtml(str) {
  if (!str.includes("&")) return str;
  return str
    .replaceAll("&amp;", "&")
    .replaceAll("&lt;", "<")
    .replaceAll("&gt;", ">")
    .replaceAll("&quot;", '"');
}
