#pragma once

#include "Voxels/VoxelChunkData.h"

using FVoxelOverrides = TMap<FIntVector, EVoxelType>;

/** Game-thread session overrides, indexed first by chunk, then by local voxel. */
struct FVoxelEditStore
{
	void Record(FIntVector Chunk, FIntVector Local, EVoxelType Type, EVoxelType Generated)
	{
		if (Type != Generated)
		{
			Chunks.FindOrAdd(Chunk).Add(Local, Type);
		}
		else if (FVoxelOverrides* Overrides = Chunks.Find(Chunk))
		{
			Overrides->Remove(Local);
			if (Overrides->IsEmpty())
			{
				Chunks.Remove(Chunk);
			}
		}
	}

	FVoxelOverrides Snapshot(FIntVector Chunk) const { return Chunks.FindRef(Chunk); }

	void Export(TMap<FIntVector, uint8>& WorldEdits) const
	{
		WorldEdits.Reset();
		for (const auto& Chunk : Chunks)
		{
			for (const auto& Edit : Chunk.Value)
			{
				WorldEdits.Add(Chunk.Key * FVoxelChunkData::ChunkSize + Edit.Key, static_cast<uint8>(Edit.Value));
			}
		}
	}

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
