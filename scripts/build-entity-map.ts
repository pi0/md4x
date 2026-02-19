const url = "https://html.spec.whatwg.org/entities.json";

const res = await fetch(url);
const entities: Record<string, { codepoints: number[] }> = await res.json();

const records: string[] = [];

for (const [name, entity] of Object.entries(entities)) {
  if (!name.endsWith(";")) continue;

  const codepoints = [...entity.codepoints];

  if (codepoints.length > 2) {
    console.error(
      `Entity ${name} needs ${codepoints.length} codepoints; may need to update the .c code accordingly.`,
    );
    process.exit(1);
  }

  while (codepoints.length < 2) codepoints.push(0);

  records.push(`    { "${name}", { ${codepoints.join(", ")} } }`);
}

records.sort();

process.stdout.write("static const ENTITY ENTITY_MAP[] = {\n");
process.stdout.write(records.join(",\n"));
process.stdout.write("\n};\n\n");
