import type { ComarkTree } from "./types.js";

export type { ComarkTree, ComarkNode, ComarkElement, ComarkText, ComarkElementAttributes } from "./types.js";

export declare function renderToHtml(input: string): string;
export declare function renderToJson(input: string): string;
export declare function parseAST(input: string): ComarkTree;
export declare function renderToAnsi(input: string): string;
