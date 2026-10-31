#include "Voxels/VoxelChunkBuild.h"

#include "Voxels/VoxelWorldGenerator.h"

FVoxelChunkBuild::FTask FVoxelChunkBuild::Launch(int32 Seed, FIntVector Coordinate, TOptional<FVoxelChunkData> Snapshot)
{
	return UE::Tasks::Launch(UE_SOURCE_LOCATION, [Seed, Coordinate, Snapshot = MoveTemp(Snapshot)]() mutable
	{
		FVoxelChunkBuild Result;
		if (!Snapshot.IsSet())
		{
			Result.GeneratedData.Emplace();
			FVoxelWorldGenerator::GenerateChunk(Result.GeneratedData.GetValue(), Seed, Coordinate);
			// Future saved edits can be applied to this owned data before the first mesh is built.
		}
		const FVoxelChunkData& Data = Snapshot.IsSet() ? Snapshot.GetValue() : Result.GeneratedData.GetValue();
		Result.Mesh = FVoxelMesher::BuildMesh(Data);
		return Result;
	});
}
