# Unreal Voxel Sandbox

A Minecraft-inspired voxel sandbox built by Boris Vacula with **Unreal Engine 5.8 and C++**.

The primary focus is on clean architecture, procedural world generation, performance, and maintainable code rather than complex gameplay.

## Screenshots

*Screenshots coming soon.*

## Features

- **Procedural terrain** — Seed-based world generation using noise.
- **Chunk-based world** — Voxel terrain organized into chunks with dynamically generated meshes.
- **First-person exploration** — Character movement and collision with voxel terrain.
- **Multiple block types** — Terrain materials determined by elevation.
- **World streaming** — Dynamic chunk management around the player.
- **Main menu** — New world creation and gameplay menu.

### Additional Systems

The project also targets the following functionality:

- Block mining with different hardness levels.
- Grid-aligned block placement and placement preview.
- Save/load using the world seed and persistent voxel modifications.
- Inventory and additional visual polish.

## Technical Details

- **Engine:** Unreal Engine 5.8
- **Language:** C++ with Blueprints for presentation and configuration
- **World representation:** Chunk-based voxel data and procedural mesh generation
- **World generation:** Deterministic, seed-driven terrain

## Getting Started

1. Clone this repository.
2. Open `UnrealMinecraft.uproject` in Unreal Engine 5.8.
3. Build the C++ project when prompted.
4. Launch the project in the Unreal Editor.

## About

This is a technical demonstration. It is not intended to be a full Minecraft clone.

**Author:** [Boriz0](https://github.com/Boriz0)
