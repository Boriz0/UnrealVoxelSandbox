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
struct UNREALMINECRAFT_API FVoxelMeshData
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
};

class UNREALMINECRAFT_API FVoxelMesher
{
public:
	static constexpr double VoxelSize = 100.0;

	/** Read-only input; the caller must not modify ChunkData during meshing. */
	static FVoxelMeshData BuildMesh(const FVoxelChunkData& ChunkData, const FVoxelBorders& Borders = {});
};
