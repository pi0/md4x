import type {
  ComarkTree,
  ComarkMeta,
  HtmlOptions,
  RenderOptions,
} from "./types.mjs";

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
} from "./types.mjs";

export interface NAPIBinding {
  renderToHtml(input: string, flags?: number): string;
  renderToAST(input: string, flags?: number): string;
  renderToAnsi(input: string, flags?: number): string;
  renderToMeta(input: string, flags?: number): string;
  renderToText(input: string, flags?: number): string;
  heal(input: string): string;
}

export interface InitOptions {
  binding?: NAPIBinding;
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
