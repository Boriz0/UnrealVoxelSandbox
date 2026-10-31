#pragma once

#include "Voxels/VoxelChunkData.h"

using FVoxelOverrides = TMap<FIntVector, EVoxelType>;

/** Game-thread session overrides, indexed first by chunk, then by local voxel. */
struct FVoxelEditStore
{
	void Record(FIntVector Chunk, FIntVector Local, EVoxelType Type)
	{
		Chunks.FindOrAdd(Chunk).Add(Local, Type);
	}

	FVoxelOverrides Snapshot(FIntVector Chunk) const { return Chunks.FindRef(Chunk); }

private:
	TMap<FIntVector, FVoxelOverrides> Chunks;
};
