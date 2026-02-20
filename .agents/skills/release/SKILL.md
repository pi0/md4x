---
name: release
description: Run the md4x release workflow. Use when the user says "release", "publish", "cut a release", "bump version", or wants to publish a new version. Generates changelog from commits since last tag, updates CHANGELOG.md, and runs the release script.
---

# Release

## Workflow

### Step 1: Gather commits since last tag

```bash
git describe --tags --abbrev=0   # get latest tag
git log <latest-tag>..HEAD --oneline --no-merges
```

If there are no new commits, abort and inform the user.

### Step 2: Determine bump type

Analyze commit messages to suggest a bump type:

- `feat:` or new features → **minor**
- `fix:`, `perf:`, `refactor:` → **patch**
- `BREAKING CHANGE` or `!:` → **major**

Present the commits and suggested bump type to the user. Ask for confirmation or override.

**Pre-1.0 version collapsing:** The release script automatically collapses bump levels for pre-1.0 versions:

- `0.0.x` — All bumps (major/minor/patch) increment the last digit only. The single digit covers everything.
- `0.x.y` — Major bumps the second digit (resets third to 0). Minor and patch both increment the last digit.
- `x.y.z` (x≥1) — Standard semver: major/minor/patch bump their respective digits.

### Step 3: Update CHANGELOG.md

Read `CHANGELOG.md`. Insert a `## Next` section (if not already present) right after the `# MD4X Change Log` heading with a summary of changes grouped by type:

- **Features** — from `feat:` commits
- **Fixes** — from `fix:` commits
- **Performance** — from `perf:` commits
- **Other** — remaining commits (chore, refactor, build, ci, docs, style, test)

Only include groups that have entries. Use concise descriptions derived from commit messages. Strip the conventional commit prefix and scope, capitalize the first letter.

Example:

```markdown
## Next

### Features

- Add linux-musl and linux-arm NAPI targets

### Fixes

- Fix release script tag handling
```

### Step 4: Let the user review

After updating `CHANGELOG.md`, show the diff and ask the user to review before proceeding.

### Step 5: Run the release script

```bash
bun scripts/release.ts <bump-type>
```

The script will:

1. Bump version in `packages/md4x/package.json` and `build.zig.zon`
2. Replace `## Next` with `## v{version}` in `CHANGELOG.md`
3. Prompt to tag and push

Pass through to the interactive prompt — let the user confirm the tag+push step.
