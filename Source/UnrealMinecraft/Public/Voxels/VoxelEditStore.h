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

	FVoxelOverrides SnapshotFace(FIntVector Chunk, int32 Axis, int32 Layer) const
	{
		FVoxelOverrides Result;
		if (const FVoxelOverrides* Edits = Chunks.Find(Chunk))
		{
			for (const auto& Edit : *Edits)
			{
				if (Edit.Key[Axis] == Layer)
				{
					Result.Add(Edit.Key, Edit.Value);
				}
			}
		}
		return Result;
	}

private:
	TMap<FIntVector, FVoxelOverrides> Chunks;
};
