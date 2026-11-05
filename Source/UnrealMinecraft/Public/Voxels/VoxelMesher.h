#pragma once

#include "CoreMinimal.h"

class FVoxelChunkData;

/** Six one-voxel occupancy planes. Unset planes represent the exterior of the world. */
struct FVoxelBorders
{
	static int32 TouchingLayer(int32 Sign);
	static int32 PlaneIndex(int32 Axis, FIntVector Local);
	TBitArray<>& InitFace(int32 Axis, int32 Sign);
	void Capture(int32 Axis, int32 Sign, const FVoxelChunkData& Neighbor);
	bool IsSolid(FIntVector Outside, int32 Axis, int32 Sign) const;

private:
	TBitArray<> Solid[6];
};

/** Chunk-local geometry with no dependency on Actors or mesh components. */
struct UNREALMINECRAFT_API FVoxelMeshSection
{
	// Deterministic rectangle IDs: origin/direction (18 bits), width/height minus one (5 bits each).
	TArray<uint32> FaceIds;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
};

struct UNREALMINECRAFT_API FVoxelMeshData
{
	// Solid types use their persistent voxel ID minus one as their material slot.
	TArray<FVoxelMeshSection> Sections;
};

class UNREALMINECRAFT_API FVoxelMesher
{
public:
	static constexpr double VoxelSize = 100.0;

	/** Greedily merges exposed coplanar faces of the same type within this chunk.
	 * Read-only input; the caller must not modify ChunkData during meshing. */
	static FVoxelMeshData BuildMesh(const FVoxelChunkData& ChunkData, const FVoxelBorders& Borders, const TArray<FLinearColor>& Tints);
};
