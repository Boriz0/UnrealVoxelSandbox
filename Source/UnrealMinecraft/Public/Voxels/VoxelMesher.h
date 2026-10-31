#pragma once

#include "CoreMinimal.h"

class FVoxelChunkData;

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
	static FVoxelMeshData BuildMesh(const FVoxelChunkData& ChunkData);
};
