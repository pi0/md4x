// Block node types
export type BlockType =
  | "document"
  | "block_quote"
  | "list"
  | "item"
  | "thematic_break"
  | "heading"
  | "code_block"
  | "html_block"
  | "paragraph"
  | "table"
  | "table_head"
  | "table_body"
  | "table_row"
  | "table_header_cell"
  | "table_cell"
  | "frontmatter";

// Span node types
export type SpanType =
  | "emph"
  | "strong"
  | "link"
  | "image"
  | "code"
  | "delete"
  | "latex_math"
  | "latex_math_display"
  | "wikilink"
  | "underline";

// Text node types
export type TextType = "text" | "linebreak" | "softbreak" | "html_inline";

export type NodeType = BlockType | SpanType | TextType;

// Leaf nodes (have `literal`, no `children`)
export interface LeafNode {
  type: "code_block" | "html_block" | "code" | "text" | "html_inline";
  literal: string;
  // code_block properties
  info?: string;
  fence?: string;
  // link/image properties (on code only if applicable)
  [key: string]: unknown;
}

// Container nodes (have `children`, no `literal`)
export interface ContainerNode {
  type: Exclude<NodeType, LeafNode["type"]>;
  children: MDNode[];
  // heading
  level?: number;
  // list
  listType?: string;
  listTight?: boolean;
  listStart?: number;
  listDelimiter?: string;
  // item (task list)
  task?: boolean;
  checked?: boolean;
  // table
  columns?: number;
  header_rows?: number;
  body_rows?: number;
  // table_header_cell / table_cell
  align?: string;
  // link / image
  destination?: string;
  title?: string;
  autolink?: boolean;
  // wikilink
  target?: string;
  [key: string]: unknown;
}

export type MDNode = LeafNode | ContainerNode;
