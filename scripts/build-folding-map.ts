import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { wrapLines } from "./_unicode-map.ts";

const selfPath = dirname(fileURLToPath(import.meta.url));
const input = readFileSync(join(selfPath, "unicode/CaseFolding.txt"), "utf8");

const statusList = new Set(["C", "F"]);
const foldingList: Map<number, number[]>[] = [new Map(), new Map(), new Map()];

// Filter the foldings for "full" folding.
for (let line of input.split("\n")) {
  const commentOff = line.indexOf("#");
  if (commentOff >= 0) line = line.slice(0, commentOff);
  line = line.trim();
  if (!line) continue;

  const [rawCodepoint, status, rawMapping] = line.split(";", 3);
  if (!statusList.has(status.trim())) continue;

  const codepoint = Number.parseInt(rawCodepoint.trim(), 16);
  const mapping = rawMapping
    .trim()
    .split(" ")
    .map((s) => Number.parseInt(s, 16));
  const mappingLen = mapping.length;

  if (mappingLen >= 1 && mappingLen <= 3) {
    foldingList[mappingLen - 1].set(codepoint, mapping);
  }
}

// Check if a new index is compatible with the range starting at index0.
//
// Type 1: consecutive codepoints mapping to consecutive targets
// Type 2: step-2 codepoints where each CP maps to CP+1
function isRangeCompatible(
  folding: Map<number, number[]>,
  codepointList: number[],
  index0: number,
  index: number,
): boolean {
  const N = index - index0;
  const codepoint0 = codepointList[index0];
  const codepoint1 = codepointList[index0 + 1];
  const codepointN = codepointList[index];
  const mapping0 = folding.get(codepoint0)!;
  const mapping1 = folding.get(codepoint1)!;
  const mappingN = folding.get(codepointN)!;

  // Type 1
  if (
    codepoint1 - codepoint0 === 1 &&
    codepointN - codepoint0 === N &&
    mapping1[0] - mapping0[0] === 1 &&
    arrEq(mapping1.slice(1), mapping0.slice(1)) &&
    mappingN[0] - mapping0[0] === N &&
    arrEq(mappingN.slice(1), mapping0.slice(1))
  ) {
    return true;
  }

  // Type 2
  if (
    codepoint1 - codepoint0 === 2 &&
    codepointN - codepoint0 === 2 * N &&
    mapping0[0] - codepoint0 === 1 &&
    mapping1[0] - codepoint1 === 1 &&
    arrEq(mapping1.slice(1), mapping0.slice(1)) &&
    mappingN[0] - codepointN === 1 &&
    arrEq(mappingN.slice(1), mapping0.slice(1))
  ) {
    return true;
  }

  return false;
}

function hex(n: number): string {
  return `0x${n.toString(16).padStart(4, "0")}`;
}

function mappingStr(mapping: number[]): string {
  return mapping.map((x) => hex(x)).join(",");
}

for (let mappingLen = 1; mappingLen <= 3; mappingLen++) {
  const folding = foldingList[mappingLen - 1];
  const codepointList = [...folding.keys()];
  const count = codepointList.length;

  const records: string[] = [];
  const dataRecords: string[] = [];

  let index0 = 0;
  while (index0 < count) {
    let index1 = index0 + 1;
    while (
      index1 < count &&
      isRangeCompatible(folding, codepointList, index0, index1)
    ) {
      index1++;
    }

    if (index1 - index0 > 2) {
      // Range of codepoints
      records.push(
        `R(${hex(codepointList[index0])},${hex(codepointList[index1 - 1])})`,
      );
      dataRecords.push(mappingStr(folding.get(codepointList[index0])!));
      dataRecords.push(mappingStr(folding.get(codepointList[index1 - 1])!));
      index0 = index1;
    } else {
      // Single codepoint
      records.push(`S(${hex(codepointList[index0])})`);
      dataRecords.push(mappingStr(folding.get(codepointList[index0])!));
      index0++;
    }
  }

  process.stdout.write(`static const unsigned FOLD_MAP_${mappingLen}[] = {\n`);
  process.stdout.write(wrapLines(records.join(", "), 110, "    "));
  process.stdout.write("\n};\n");

  process.stdout.write(
    `static const unsigned FOLD_MAP_${mappingLen}_DATA[] = {\n`,
  );
  process.stdout.write(wrapLines(dataRecords.join(", "), 110, "    "));
  process.stdout.write("\n};\n");
}

// --- helpers ---

function arrEq(a: number[], b: number[]): boolean {
  return a.length === b.length && a.every((v, i) => v === b[i]);
}
