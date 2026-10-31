#pragma once

#include "Tasks/Task.h"
#include "Voxels/VoxelChunkData.h"
#include "Voxels/VoxelMesher.h"
#include "Voxels/VoxelEditStore.h"

/** Worker input for the touching face of an unloaded, streamable chunk. */
struct FVoxelProceduralBorder
{
	FIntVector Neighbor;
	int32 Axis;
	int32 Sign;
	FVoxelOverrides Edits;
};

/** Worker-owned output. GeneratedData is populated only for a newly generated chunk. */
struct FVoxelChunkBuild
{
	TOptional<FVoxelChunkData> GeneratedData;
	FVoxelMeshData Mesh;

	using FTask = UE::Tasks::TTask<FVoxelChunkBuild>;
	static FTask Launch(int32 Seed, FIntVector Coordinate, TOptional<FVoxelChunkData> Snapshot = {},
		FVoxelOverrides Edits = {}, FVoxelBorders Borders = {}, TArray<FVoxelProceduralBorder> MissingBorders = {});
};
