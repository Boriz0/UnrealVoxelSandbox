# Unreal Voxel Sandbox

A Minecraft-inspired voxel sandbox built by Boris Vacula with **Unreal Engine 5.8 and C++**.

The primary focus is on clean architecture, procedural world generation, performance, and maintainable code rather than complex gameplay.

## Download & Play

**[Download the release Windows build](https://github.com/Boriz0/UnrealVoxelSandbox/releases/download/v1.0.0/UnrealVoxelSandbox.zip)**

1. Download `UnrealVoxelSandbox-Windows-v1.0.zip` from the release assets.
2. Extract the ZIP file.
3. Run `UnrealMinecraft.exe`.

No Unreal Engine installation is required.

## Screenshots

<img width="2560" height="1440" alt="ScreenShot00016" src="https://github.com/user-attachments/assets/43ebca0a-d52d-49eb-9b7f-0698f17b4a7a" />

<img width="2560" height="1440" alt="ScreenShot00010" src="https://github.com/user-attachments/assets/d911d94e-3eee-4e5f-b086-7290c5a9b380" />

<img width="2560" height="1440" alt="ScreenShot00015" src="https://github.com/user-attachments/assets/b0bc611b-856b-4639-9c55-48a63fa28864" />

<img width="2560" height="1440" alt="ScreenShot00017" src="https://github.com/user-attachments/assets/b5c1485b-f55c-4c2b-bb44-870dff8923d7" />

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
