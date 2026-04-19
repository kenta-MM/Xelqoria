# Coding Rules For Agents

## Scope

Follow `architecture.md` for dependency direction and layer responsibility.

## Placement Rules

- Put rendering concepts in Graphics
- Put GPU abstraction in RHI
- Put platform-specific API code in Backends
- Put gameplay logic and persistent gameplay data in Game

## Rendering Rules

- Sprite must NOT render itself
- Rendering must be handled by Renderer classes
- Graphics must use RHI for GPU operations

## Prohibitions

- Do NOT use Direct3D types in Graphics
- Do NOT add rendering concepts to RHI
- Do NOT reference Backends from Game
- Do NOT create includes that violate layer boundaries

## Change Policy

- Keep changes minimal
- Preserve existing layer boundaries
- Avoid leaking platform-specific details upward