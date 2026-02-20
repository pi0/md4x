#!/usr/bin/env bun
import { execSync } from "node:child_process";
import { readFileSync, writeFileSync } from "node:fs";
import { dirname, resolve } from "node:path";
import { createInterface } from "node:readline";
import { fileURLToPath } from "node:url";

const selfPath = dirname(fileURLToPath(import.meta.url));
const projectDir = resolve(selfPath, "..");
const pkgJsonPath = resolve(projectDir, "packages/md4x/package.json");
const zigZonPath = resolve(projectDir, "build.zig.zon");
const changelogPath = resolve(projectDir, "CHANGELOG.md");

const args = process.argv.slice(2);
const noTag = args.includes("--no-tag");
const bump = (args.find((a) => !a.startsWith("--")) || "patch") as
  | "major"
  | "minor"
  | "patch";
if (!["major", "minor", "patch"].includes(bump)) {
  console.error(
    `Usage: bun scripts/release.ts [major|minor|patch] [--no-tag]`,
  );
  process.exit(1);
}

// Read current version from package.json
const pkgJson = JSON.parse(readFileSync(pkgJsonPath, "utf8"));
const current = pkgJson.version as string;
const [major, minor, patch] = current.split(".").map(Number);

const next =
  bump === "major"
    ? `${major + 1}.0.0`
    : bump === "minor"
      ? `${major}.${minor + 1}.0`
      : `${major}.${minor}.${patch + 1}`;

console.log(`${current} → ${next}`);

// Update package.json
pkgJson.version = next;
writeFileSync(pkgJsonPath, JSON.stringify(pkgJson, null, 2) + "\n");
console.log(`Updated packages/md4x/package.json`);

// Update build.zig.zon
const zigZon = readFileSync(zigZonPath, "utf8");
const updatedZigZon = zigZon.replace(
  /\.version\s*=\s*"[^"]+"/,
  `.version = "${next}"`,
);
if (updatedZigZon === zigZon) {
  console.error("Failed to update version in build.zig.zon");
  process.exit(1);
}
writeFileSync(zigZonPath, updatedZigZon);
console.log(`Updated build.zig.zon`);

// Update CHANGELOG.md — replace "## Next" with "## v{next}"
const changelog = readFileSync(changelogPath, "utf8");
const updatedChangelog = changelog.replace("## Next", `## v${next}`);
if (updatedChangelog !== changelog) {
  writeFileSync(changelogPath, updatedChangelog);
  console.log(`Updated CHANGELOG.md`);
}

if (!noTag) {
  const rl = createInterface({ input: process.stdin, output: process.stdout });
  const answer = await new Promise<string>((r) =>
    rl.question(`Tag and push v${next}? (y/N) `, r),
  );
  rl.close();
  if (answer.toLowerCase() !== "y") {
    console.log("Aborted.");
    process.exit(0);
  }

  // Git commit and tag
  execSync(`git add "${pkgJsonPath}" "${zigZonPath}" "${changelogPath}"`, {
    cwd: projectDir,
    stdio: "inherit",
  });
  execSync(`git commit -m "v${next}"`, {
    cwd: projectDir,
    stdio: "inherit",
  });
  execSync(`git tag v${next}`, { cwd: projectDir, stdio: "inherit" });
  execSync(`git push --follow-tags`, { cwd: projectDir, stdio: "inherit" });
  console.log(`\nPublished v${next}`);
}
