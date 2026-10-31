#pragma once

#include "CoreMinimal.h"

class FVoxelChunkData;
enum class EVoxelType : uint8;

/** Deterministic height-based terrain generation, independent of world objects. */
class UNREALMINECRAFT_API FVoxelWorldGenerator
{
public:
	static EVoxelType GetSolidTypeAtHeight(int64 WorldZ);
	// Heights are measured in voxels; NoiseScale is noise units per voxel.
	static constexpr int32 BaseTerrainHeight = 32;
	static constexpr double NoiseScale = 0.01;
	static constexpr double HeightAmplitude = 24.0;
	static constexpr int32 GrassMinHeight = 16;
	static constexpr int32 SnowMinHeight = 48;

	/**
	 * Overwrites every voxel using absolute voxel coordinates.
	 * ChunkCoordinates are chunk indices, including signed Z layer indices.
	 * The surface voxel is solid; higher voxels are Air. Solid voxels below
	 * GrassMinHeight are Stone, below SnowMinHeight are Grass, otherwise Snow.
	 */
	static void GenerateChunk(FVoxelChunkData& ChunkData, int32 WorldSeed, const FIntVector& ChunkCoordinates);
};
