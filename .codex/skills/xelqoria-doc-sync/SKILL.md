---
name: xelqoria-doc-sync
description: Use AFTER a pull request is created. Update only the necessary docs based on PR changes.
---

# Doc Sync (PR-based)

## Use

- After a pull request is prepared
- When code changes may affect documentation

## Steps

1. Inspect PR:
   - title
   - description
   - changed files

2. Identify ONE affected doc

3. Update only that doc

4. Keep changes minimal

5. Explain what changed and why


## Mapping (select primary doc)

Choose the SINGLE most relevant document to update.

Only update multiple docs if the change clearly affects multiple areas.

- Dependency / architecture change
  → docs/agents/architecture.md

- Layer responsibility / placement change
  → docs/agents/project-map.md

- Implementation flow change
  → docs/agents/workflows.md

- Coding rule change
  → docs/agents/coding-rules.md

- Runtime / Editor boundary change
  → docs/agents/runtime-vs-editor-boundary.md

- Asset resolution change
  → docs/agents/asset-flow.md

- CI / validation change
  → docs/quality/*.md

## Rules

- Use ONLY PR-visible changes as source
- Do NOT scan entire repository
- Do NOT update multiple docs unless required
- Do NOT create new docs
- Do NOT rewrite large sections
- Do NOT speculate

## Fail-safe

- If affected doc is unclear → update nothing