import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const selfPath = dirname(fileURLToPath(import.meta.url));

export function buildUnicodeMap(opts: {
  filter: (category: string) => boolean;
  arrayName: string;
}) {
  const input = readFileSync(
    join(selfPath, "unicode/DerivedGeneralCategory.txt"),
    "utf8",
  );

  const codepointList: number[] = [];

  for (let line of input.split("\n")) {
    const commentOff = line.indexOf("#");
    if (commentOff >= 0) line = line.slice(0, commentOff);
    line = line.trim();
    if (!line) continue;

    const [charRange, category] = line.split(";").map((s) => s.trim());
    if (!opts.filter(category)) continue;

    const delimOff = charRange.indexOf("..");
    if (delimOff >= 0) {
      const cp0 = Number.parseInt(charRange.slice(0, delimOff), 16);
      const cp1 = Number.parseInt(charRange.slice(delimOff + 2), 16);
      for (let cp = cp0; cp <= cp1; cp++) codepointList.push(cp);
    } else {
      codepointList.push(Number.parseInt(charRange, 16));
    }
  }

  codepointList.sort((a, b) => a - b);

  const records: string[] = [];
  let index0 = 0;
  const count = codepointList.length;

  while (index0 < count) {
    let index1 = index0 + 1;
    while (
      index1 < count &&
      codepointList[index1] === codepointList[index1 - 1] + 1
    ) {
      index1++;
    }

    if (index1 - index0 > 1) {
      records.push(
        `R(${hex(codepointList[index0])},${hex(codepointList[index1 - 1])})`,
      );
    } else {
      records.push(`S(${hex(codepointList[index0])})`);
    }
    index0 = index1;
  }

  process.stdout.write(`static const unsigned ${opts.arrayName}[] = {\n`);
  process.stdout.write(wrapLines(records.join(", "), 110, "    "));
  process.stdout.write("\n};\n\n");
}

function hex(n: number): string {
  return `0x${n.toString(16).padStart(4, "0")}`;
}

export function wrapLines(
  text: string,
  width: number,
  indent: string,
): string {
  // Emulate Python's textwrap.wrap: break at spaces in "item1, item2, item3"
  // Each "word" is "item," (with comma attached), except the last item
  const words = text.split(" ");
  const lines: string[] = [];
  let current = indent;

  for (const word of words) {
    if (current === indent) {
      current += word;
    } else if (current.length + 1 + word.length <= width) {
      current += " " + word;
    } else {
      lines.push(current);
      current = indent + word;
    }
  }
  if (current !== indent) lines.push(current);
  return lines.join("\n");
}
