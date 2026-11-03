#pragma once

#include "CoreMinimal.h"

inline constexpr uint8 VoxelAir = 0;

/** Owns a fixed-size chunk of voxel types in X-major order. */
class UNREALMINECRAFT_API FVoxelChunkData
{
public:
	static constexpr int32 ChunkSize = 32;
	static constexpr int32 VoxelCount = ChunkSize * ChunkSize * ChunkSize;

	/** Initializes every voxel to Air. */
	FVoxelChunkData();

	/** Returns Air when Coordinates are outside the chunk. */
	uint8 GetVoxel(const FIntVector& Coordinates) const;

	/** Returns false without modifying data when Coordinates are outside the chunk. */
	bool SetVoxel(const FIntVector& Coordinates, uint8 VoxelType);

	/** Local coordinates must be in [0, ChunkSize) on every axis. */
	static bool IsValidCoordinate(const FIntVector& Coordinates);

private:
	/** Call only after validating Coordinates. */
	static int32 GetIndex(const FIntVector& Coordinates);

	TArray<uint8> Voxels;
};
