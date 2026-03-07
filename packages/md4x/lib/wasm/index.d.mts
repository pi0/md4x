import type {
  ComarkTree,
  ComarkMeta,
  HtmlOptions,
  RenderOptions,
} from "../types.mjs";

export type {
  ComarkTree,
  ComarkNode,
  ComarkElement,
  ComarkText,
  ComarkElementAttributes,
  ComarkHeading,
  ComarkMeta,
  HtmlOptions,
  RenderOptions,
} from "../types.mjs";

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
export declare function renderToAST(
  input: string,
  opts?: RenderOptions,
): string;
export declare function parseAST(
  input: string,
  opts?: RenderOptions,
): ComarkTree;
export declare function renderToAnsi(
  input: string,
  opts?: RenderOptions,
): string;
export declare function renderToMeta(
  input: string,
  opts?: RenderOptions,
): string;
export declare function parseMeta(
  input: string,
  opts?: RenderOptions,
): ComarkMeta;
export declare function renderToText(
  input: string,
  opts?: RenderOptions,
): string;
export declare function heal(input: string): string;
