#include "Voxels/VoxelChunkBuild.h"

#include "Voxels/VoxelWorldGenerator.h"

FVoxelChunkBuild::FTask FVoxelChunkBuild::Launch(int32 Seed, FIntVector Coordinate, TOptional<FVoxelChunkData> Snapshot,
	FVoxelOverrides Edits, FVoxelBorders Borders)
{
	return UE::Tasks::Launch(UE_SOURCE_LOCATION, [Seed, Coordinate, Snapshot = MoveTemp(Snapshot),
		Edits = MoveTemp(Edits), Borders = MoveTemp(Borders)]() mutable
	{
		FVoxelChunkBuild Result;
		if (!Snapshot.IsSet())
		{
			Result.GeneratedData.Emplace();
			FVoxelWorldGenerator::GenerateChunk(Result.GeneratedData.GetValue(), Seed, Coordinate);
			for (const auto& Edit : Edits)
			{
				Result.GeneratedData->SetVoxel(Edit.Key, Edit.Value);
			}
		}
		const FVoxelChunkData& Data = Snapshot.IsSet() ? Snapshot.GetValue() : Result.GeneratedData.GetValue();
		Result.Mesh = FVoxelMesher::BuildMesh(Data, Borders);
		return Result;
	});
}
