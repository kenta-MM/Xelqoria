# Architecture For Agents

## Dependency

Game -> Graphics -> RHI -> Backends

Never break this order.

## Layer Responsibility

- Game = gameplay logic and persistent gameplay data
- Graphics = API-independent rendering logic and rendering concepts
- RHI = low-level GPU abstraction
- Backends = platform-specific implementation of RHI

## Core Rules

- Platform-specific API is allowed ONLY in Backends projects
- Graphics must NOT use Direct3D types
- RHI must NOT contain rendering concepts such as Sprite, Camera, Material, or Renderer
- Game must NOT reference Backends or Direct3D

## Rendering Rule

- Rendering is performed by Renderer classes
- Data objects such as Sprite must NOT render themselves