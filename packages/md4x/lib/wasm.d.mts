import type { ComarkTree, ComarkMeta, HtmlOptions } from "./types.mjs";

export type {
  ComarkTree,
  ComarkNode,
  ComarkElement,
  ComarkText,
  ComarkElementAttributes,
  ComarkHeading,
  ComarkMeta,
  HtmlOptions,
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
export declare function renderToHtml(input: string, opts?: HtmlOptions): string;
export declare function renderToAST(input: string): string;
export declare function parseAST(input: string): ComarkTree;
export declare function renderToAnsi(input: string): string;
export declare function renderToMeta(input: string): string;
export declare function parseMeta(input: string): ComarkMeta;
export declare function renderToText(input: string): string;
export declare function parseYaml(input: string): unknown;
