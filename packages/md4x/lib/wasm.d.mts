import type { ContainerNode } from "./types.js";

export type { BlockType, SpanType, TextType, NodeType, LeafNode, ContainerNode, MDNode } from "./types.js";

export declare function initWasm(input?: ArrayBuffer | Uint8Array | WebAssembly.Module | Response | Promise<Response>): Promise<void>;
export declare function renderToHtml(input: string): string;
export declare function renderToJson(input: string): ContainerNode;
export declare function renderToAnsi(input: string): string;
