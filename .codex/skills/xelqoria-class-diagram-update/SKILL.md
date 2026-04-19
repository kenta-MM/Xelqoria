---
name: xelqoria-class-diagram-update
description: Use AFTER a pull request that changes public headers or dependencies. Update only the affected project diagram.
---

# Class Diagram Update (PR-based)

## Use

- Public header (.h) changes
- Class relationship changes
- ProjectReference changes

## Steps

1. Inspect PR changed files
2. Identify affected project
3. Update ONLY that project's diagram:
   docs/class_diagram/<project>-class-diagram.md
4. Reflect only code-visible relationships
5. Keep diagram minimal
6. Explain what changed and why

## Include

- Public classes in the project
- Direct relationships (inheritance, composition, usage)
- Lower-layer interfaces ONLY if directly referenced

## Do NOT Include

- Unrelated projects
- Full-system diagrams
- Speculative relationships
- Future design

## Layer Rules

- Core → Core only
- RHI → RHI only
- Backends → Backends + RHI interfaces
- Graphics → Graphics + RHI interfaces
- Game → Game + Graphics/Core if needed
- App/Editor → direct dependencies only

## Mermaid Rules

- Keep diagrams small
- Prefer simple relations:
  -->  ..>  *--  o--
- Avoid complex generics
- Avoid unsupported syntax
- Do NOT over-detail

## Update Policy

- Update ONE diagram only
- Do NOT update multiple diagrams unless required
- Prefer editing existing file
- Create new diagram ONLY if missing

## Fail-safe

- If project cannot be identified → do nothing