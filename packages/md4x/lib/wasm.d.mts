import type { ComarkTree } from "./types.js";

export type {
  ComarkTree,
  ComarkNode,
  ComarkElement,
  ComarkText,
  ComarkElementAttributes,
} from "./types.js";

export declare function initWasm(
  input?:
    | ArrayBuffer
    | Uint8Array
    | WebAssembly.Module
    | Response
    | Promise<Response>,
): Promise<void>;
export declare function renderToHtml(input: string): string;
export declare function renderToAST(input: string): string;
export declare function parseAST(input: string): ComarkTree;
export declare function renderToAnsi(input: string): string;
