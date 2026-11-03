#include "Voxels/VoxelChunkBuild.h"

#include "Voxels/VoxelWorldGenerator.h"

static void GenerateBorder(const FVoxelWorldGenerator& Terrain, const FVoxelProceduralBorder& Input, FVoxelBorders& Borders)
{
	constexpr int32 Size = FVoxelChunkData::ChunkSize;
	const FInt64Vector Origin = FInt64Vector(Input.Neighbor) * static_cast<int64>(Size);
	FIntVector Min = FIntVector::ZeroValue;
	FIntVector Max(Size - 1);
	Min[Input.Axis] = Max[Input.Axis] = FVoxelBorders::TouchingLayer(Input.Sign);
	TBitArray<>& Face = Borders.InitFace(Input.Axis, Input.Sign);

	// Only visit the touching plane, sampling noise once per distinct X/Y column.
	for (int32 Y = Min.Y; Y <= Max.Y; ++Y)
	{
		for (int32 X = Min.X; X <= Max.X; ++X)
		{
			const int32 Height = Terrain.GetSurfaceHeight(Origin.X + X, Origin.Y + Y);
			for (int32 Z = Min.Z; Z <= Max.Z; ++Z)
			{
				Face[FVoxelBorders::PlaneIndex(Input.Axis, FIntVector(X, Y, Z))] = Origin.Z + Z <= Height;
			}
		}
	}
	for (const auto& Edit : Input.Edits)
	{
		Face[FVoxelBorders::PlaneIndex(Input.Axis, Edit.Key)] = Edit.Value != VoxelAir;
	}
}

FVoxelChunkBuild::FTask FVoxelChunkBuild::Launch(TSharedRef<const FVoxelCatalogSnapshot> Catalog, int32 Seed, FIntVector Coordinate, TOptional<FVoxelChunkData> Snapshot,
	FVoxelOverrides Edits, FVoxelBorders Borders, TArray<FVoxelProceduralBorder> MissingBorders)
{
	return UE::Tasks::Launch(UE_SOURCE_LOCATION, [Catalog, Seed, Coordinate, Snapshot = MoveTemp(Snapshot),
		Edits = MoveTemp(Edits), Borders = MoveTemp(Borders), MissingBorders = MoveTemp(MissingBorders)]() mutable
	{
		FVoxelChunkBuild Result;
		if (!Snapshot.IsSet())
		{
			Result.GeneratedData.Emplace();
			FVoxelWorldGenerator::GenerateChunk(Result.GeneratedData.GetValue(), Seed, Coordinate, *Catalog);
			for (const auto& Edit : Edits)
			{
				Result.GeneratedData->SetVoxel(Edit.Key, Edit.Value);
			}
		}
		const FVoxelChunkData& Data = Snapshot.IsSet() ? Snapshot.GetValue() : Result.GeneratedData.GetValue();
		const FVoxelWorldGenerator Terrain(Seed);
		for (const FVoxelProceduralBorder& Input : MissingBorders)
		{
			GenerateBorder(Terrain, Input, Borders);
		}
		Result.Mesh = FVoxelMesher::BuildMesh(Data, Borders, Catalog->Tints);
		return Result;
	});
}
