export type ComarkTree = {
  type: "comark";
  value: ComarkNode[];
};

export type ComarkNode = ComarkElement | ComarkText;

export type ComarkText = string;

export type ComarkElement = [
  tag: string,
  props: ComarkElementAttributes,
  ...children: ComarkNode[],
];

export type ComarkElementAttributes = {
  [key: string]: unknown;
};

export type ComarkHeading = {
  level: number;
  text: string;
};

export type ComarkMeta = {
  title?: string;
  headings: ComarkHeading[];
  [key: string]: unknown;
};

export interface HtmlOptions {
  /** Generate a full HTML document with `<!DOCTYPE html>`, `<head>`, and `<body>`. */
  full?: boolean;

  /**
   * Custom highlighter function for fenced code blocks. If provided, this will cause
   */
  highlighter: CodeBlockHighlighter;
}

export interface CodeBlock {
  /** Character offset in HTML string where code content starts (after `<code...>`) */
  start: number;
  /** Character offset in HTML string where code content ends (before `</code>`) */
  end: number;
  /** Language identifier (empty string if none) */
  lang: string;
  /** Filename from `[filename]` syntax */
  filename?: string;
  /** Highlighted line numbers from `{1-3,5}` syntax */
  highlights?: number[];
}

export type CodeBlockHighlighter = (
  /** Raw code content (HTML entities unescaped) */
  code: string,
  /** Code block metadata (lang, filename, highlights, offsets) */
  block: CodeBlock,
) => string | undefined;

export interface HtmlWithCodeBlocks {
  /** The HTML string */
  html: string;
  /** Metadata for each fenced code block in document order */
  codeBlocks: CodeBlock[];
}
