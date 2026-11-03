#pragma once

#include "CoreMinimal.h"

class FVoxelChunkData;
struct FVoxelCatalogSnapshot;

/** Deterministic height-based terrain generation, independent of world objects. */
class UNREALMINECRAFT_API FVoxelWorldGenerator
{
public:
	static constexpr int32 Version = 1;
	explicit FVoxelWorldGenerator(int32 WorldSeed);
	int32 GetSurfaceHeight(int64 WorldX, int64 WorldY) const;
	// Heights are measured in voxels; NoiseScale is noise units per voxel.
	static constexpr int32 BaseTerrainHeight = 32;
	static constexpr double NoiseScale = 0.01;
	static constexpr double HeightAmplitude = 24.0;

	/**
	 * Overwrites every voxel using absolute voxel coordinates.
	 * ChunkCoordinates are chunk indices, including signed Z layer indices.
	 * The surface voxel is solid; higher voxels are Air.
	 */
	static void GenerateChunk(FVoxelChunkData& ChunkData, int32 WorldSeed, const FIntVector& ChunkCoordinates, const FVoxelCatalogSnapshot& Catalog);

private:
	FVector2D NoiseOffset;
};
