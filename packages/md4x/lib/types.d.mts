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
