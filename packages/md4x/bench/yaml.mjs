import { bench, compact, run, summary } from "mitata";
import * as wasm from "../lib/wasm.mjs";
import * as napi from "../lib/wasm.mjs";
import jsYaml from "js-yaml";

await napi.init();
await wasm.init();

napi.parseYaml("test: true");

const small = `title: Hello World
description: A simple document
`;

const medium = `title: Project Config
description: A medium-sized YAML document
version: 3
enabled: true
tags:
  - markdown
  - parser
  - wasm
author:
  name: John Doe
  email: john@example.com
  url: https://example.com
options:
  debug: false
  verbose: true
  maxRetries: 3
dependencies:
  - name: libyaml
    version: "0.2.5"
  - name: zig
    version: "0.14.0"
`;

const large = `title: Large Config
description: A large YAML document with many fields
version: 42
published: true
license: MIT
homepage: https://example.com/project
repository: https://github.com/example/project
keywords:
  - markdown
  - parser
  - ast
  - html
  - wasm
  - napi
  - c
  - zig
author:
  name: Jane Smith
  email: jane@example.com
  url: https://example.com
  github: janesmith
contributors:
  - name: Alice
    email: alice@example.com
  - name: Bob
    email: bob@example.com
  - name: Charlie
    email: charlie@example.com
build:
  target: release
  optimize: true
  flags:
    - "-Wall"
    - "-Wextra"
    - "-O2"
  env:
    NODE_ENV: production
    DEBUG: "false"
features:
  tables: true
  strikethrough: true
  tasklists: true
  math: true
  wikilinks: true
  frontmatter: true
  components: true
  attributes: true
  alerts: true
scripts:
  build: zig build
  test: bun scripts/run-tests.ts
  bench: bun packages/md4x/bench/index.mjs
  format: bun fmt
`;

const inputs = { small, medium, large };

for (const [name, input] of Object.entries(inputs)) {
  compact(() => {
    summary(() => {
      bench(`md4x-napi parseYaml (${name})`, () => napi.parseYaml(input));
      bench(`md4x-wasm parseYaml (${name})`, () => wasm.parseYaml(input));
      bench(`js-yaml (${name})`, () => jsYaml.load(input));
    });
  });
}

await run();
