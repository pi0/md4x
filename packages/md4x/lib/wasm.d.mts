import type { ComarkTree } from "./types.mjs";

export type {
  ComarkTree,
  ComarkNode,
  ComarkElement,
  ComarkText,
  ComarkElementAttributes,
} from "./types.mjs";

export interface InitOptions {
  wasm?:
    | ArrayBuffer
    | Uint8Array
    | WebAssembly.Module
    | Response
    | Promise<Response>;
}

export declare function init(opts?: InitOptions): Promise<void>;
export declare function renderToHtml(input: string): string;
export declare function renderToAST(input: string): string;
export declare function parseAST(input: string): ComarkTree;
export declare function renderToAnsi(input: string): string;
