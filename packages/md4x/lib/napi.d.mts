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

export interface NAPIBinding {
  renderToHtml(input: string, flags?: number): string;
  renderToAST(input: string): string;
  renderToAnsi(input: string): string;
  renderToMeta(input: string): string;
  renderToText(input: string): string;
}

export interface InitOptions {
  binding?: NAPIBinding;
}

export declare function init(opts?: InitOptions): Promise<void>;
export declare function renderToHtml(input: string, opts?: HtmlOptions): string;
export declare function renderToAST(input: string): string;
export declare function parseAST(input: string): ComarkTree;
export declare function renderToAnsi(input: string): string;
export declare function renderToMeta(input: string): string;
export declare function parseMeta(input: string): ComarkMeta;
export declare function renderToText(input: string): string;
