// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMinecraftGameMode.h"
#include "Voxels/VoxelHUD.h"

AUnrealMinecraftGameMode::AUnrealMinecraftGameMode()
{
	HUDClass = AVoxelHUD::StaticClass();
}
