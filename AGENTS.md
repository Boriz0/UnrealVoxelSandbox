# Repository Guidelines

## Project Structure & Module Organization
This is an Unreal Engine 5.8 C++ first-person prototype. `UnrealMinecraft.uproject` declares one runtime module, `UnrealMinecraft`.

- `Source/UnrealMinecraft/`: shared character, camera, controller, and game-mode code. `Variant_Horror/` and `Variant_Shooter/` contain variant gameplay; shooter subfolders separate AI, UI, and weapons.
- `Source/*.Target.cs` and `Source/UnrealMinecraft/UnrealMinecraft.Build.cs`: build targets and module dependencies.
- `Content/`: maps, Blueprints, input assets, characters, and weapons. The default map is `FirstPerson/Lvl_FirstPerson`.
- `Config/`: versioned Unreal configuration.
- `Binaries/`, `Intermediate/`, `Saved/`, and `DerivedDataCache/`: generated local data; do not commit these directories.

## Build, Test, and Development Commands
Install UE 5.8 and Visual Studio C++/game-development components (see `.vsconfig`). Run these PowerShell commands from the repository root, setting `$UE` to your engine installation:

```powershell
$UE = 'C:\Program Files\Epic Games\UE_5.8'
git lfs install
git lfs pull
& "$UE\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "-project=$PWD\UnrealMinecraft.uproject" -game -engine
& "$UE\Engine\Build\BatchFiles\Build.bat" UnrealMinecraftEditor Win64 Development "-Project=$PWD\UnrealMinecraft.uproject" -WaitMutex
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\UnrealMinecraft.uproject"
```

These commands initialize/download LFS assets, generate IDE files, build the editor target, and open the project. Use Play in Editor for local gameplay checks.

## Coding Style & Naming Conventions
Match existing C++: tab indentation, braces on separate lines, and PascalCase functions and members. Use Unreal type prefixes (`A`, `U`, `F`) and `b` for booleans, such as `bForceTouchControls`. Keep paired `.h`/`.cpp` filenames aligned with their classes and `.generated.h` last among header includes. Use reflection macros for Blueprint-exposed properties/functions. No repository formatter or lint configuration is present.

## Testing Guidelines
No project automation tests or coverage threshold currently exist. Build the editor target and smoke-test movement, looking, and jumping in `Lvl_FirstPerson`; test affected mechanics in `Lvl_Shooter` or `Lvl_Horror`. Check the Output Log for errors. For new automated tests, use Unreal Automation Framework, place them under `Source/UnrealMinecraft/Tests/`, name them `UnrealMinecraft.<Feature>.<Behavior>`, and run them through Session Frontend's Automation tab.

## Commit & Pull Request Guidelines
Recent commits use short, capitalized descriptions such as `Adjusted player start position`; follow that style and keep changes focused. PRs should describe behavior changes, list build/manual-test results, link relevant issues, and include screenshots or clips for visual changes. Track `.uasset` and `.umap` files through the configured Git LFS rules; coordinate binary-asset edits to avoid merge conflicts.
