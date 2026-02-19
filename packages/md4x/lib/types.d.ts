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
