# AGENTS.md

## Rules

- Follow `architecture.md` as the source of truth for dependency direction and layer responsibility.
- Platform-specific API is allowed ONLY in Backends projects.
- Data objects must NOT render themselves.

## Never

- Use Direct3D types in Graphics
- Put rendering concepts in RHI
- Reference Backends from Game
- Break layer boundaries for convenience

## Decision

- If unsure about placement, check `project-map.md`.
- If unsure about implementation flow, check `workflows.md`.
- If unsure about architecture, follow `architecture.md`.

## Docs

- architecture.md
- coding-rules.md
- workflows.md
- project-map.md