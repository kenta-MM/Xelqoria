# Project Map For Agents

## Dependency

App / Editor
↓
Game
↓
Graphics
↓
RHI
↓
Backends

Core is a shared base layer.

## Core

- Responsibility: application base, window, asset identifiers
- Must NOT contain rendering logic

## RHI

- Responsibility: GPU abstraction
- Must NOT contain rendering concepts
- Must NOT expose Direct3D types in public abstractions

## Graphics

- Responsibility: rendering concepts and rendering systems
- May use: Core, RHI
- Must NOT use Direct3D types
- Must use RHI for GPU operations

## Game

- Responsibility: gameplay logic, scene, entity, component, persistent gameplay data
- May use: Core, Graphics
- Must NOT use: Backends, Direct3D

## Backends

- Responsibility: platform-specific implementation of RHI
- Only layer allowed to use Direct3D

## App

- Responsibility: runtime composition and startup
- May compose: Core, Game, Graphics, RHI, Backends
- Must NOT contain Editor-only logic

## Editor

- Responsibility: editor-only tools and editing support
- May compose: Core, Game, Graphics, RHI, Backends
- Must NOT push editor-only concepts into shared runtime layers