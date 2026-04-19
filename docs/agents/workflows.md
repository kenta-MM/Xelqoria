# Workflows For Agents

## Placement Decision

1. Rendering concept -> Graphics
2. Platform-specific API -> Backends
3. GPU abstraction -> RHI
4. Engine base system -> Core
5. Gameplay logic or persistent gameplay data -> Game

If unsure, re-check `architecture.md` and `project-map.md`.

## Implementation Flow

1. Identify the responsibility of the change
2. Choose the correct layer
3. Keep the change minimal
4. Ensure no platform-specific leakage outside Backends
5. Verify build and behavior

## Validation Checklist

- No layer boundary violation
- No Direct3D types outside Backends
- No rendering in data objects
- No Game-to-Backends reference
- No rendering concepts added to RHI