#pragma once

#include "GameFramework/SaveGame.h"
#include "VoxelWorldSave.generated.h"

/** A checkpoint of the procedural world. Positions in Edits are absolute voxel coordinates. */
UCLASS()
class UVoxelWorldSave : public USaveGame
{
	GENERATED_BODY()

public:
	// Zero defaults ensure Unreal serializes the actual versions even with delta serialization.
	UPROPERTY() int32 FormatVersion = 0;
	UPROPERTY() int32 GeneratorVersion = 0;
	UPROPERTY() int64 Generation = 0;
	UPROPERTY() int32 Seed = 0;
	UPROPERTY() bool bNoclip = false;
	UPROPERTY() FTransform PlayerTransform;
	UPROPERTY() FRotator ViewRotation = FRotator::ZeroRotator;
	UPROPERTY() TMap<FIntVector, uint8> Edits;

	static bool Exists();
	static UVoxelWorldSave* LoadLatest(int32* OutSlot = nullptr);
	bool Commit();
private:
	bool IsCompatible() const;
};
