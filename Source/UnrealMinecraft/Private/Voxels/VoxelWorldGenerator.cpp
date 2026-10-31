#include "Voxels/VoxelWorldGenerator.h"

#include "Math/RandomStream.h"
#include "Voxels/VoxelChunkData.h"

EVoxelType FVoxelWorldGenerator::GetSolidTypeAtHeight(int64 WorldZ)
{
	return WorldZ < GrassMinHeight ? EVoxelType::Stone
		: (WorldZ < SnowMinHeight ? EVoxelType::Grass : EVoxelType::Snow);
}

void FVoxelWorldGenerator::GenerateChunk(FVoxelChunkData& ChunkData, int32 WorldSeed, const FIntVector& ChunkCoordinates)
{
	// Recreate a local stream per chunk so offsets depend only on the world seed.
	FRandomStream RandomStream(WorldSeed);
	constexpr float MaxNoiseOffset = 10000.0f;
	const double NoiseOffsetX = RandomStream.FRandRange(-MaxNoiseOffset, MaxNoiseOffset);
	const double NoiseOffsetY = RandomStream.FRandRange(-MaxNoiseOffset, MaxNoiseOffset);

	constexpr int32 ChunkSize = FVoxelChunkData::ChunkSize;
	// Widen before multiplying to support negative and large chunk indices.
	const int64 OriginX = static_cast<int64>(ChunkCoordinates.X) * ChunkSize;
	const int64 OriginY = static_cast<int64>(ChunkCoordinates.Y) * ChunkSize;
	const int64 OriginZ = static_cast<int64>(ChunkCoordinates.Z) * ChunkSize;

	// Sample once per world-space column, independent of the vertical chunk layer.
	int32 SurfaceHeights[ChunkSize * ChunkSize];
	for (int32 Y = 0; Y < ChunkSize; ++Y)
	{
		for (int32 X = 0; X < ChunkSize; ++X)
		{
			const FVector2D NoisePosition(
				static_cast<double>(OriginX + X) * NoiseScale + NoiseOffsetX,
				static_cast<double>(OriginY + Y) * NoiseScale + NoiseOffsetY);
			const double Noise = FMath::PerlinNoise2D(NoisePosition);
			SurfaceHeights[X + ChunkSize * Y] = FMath::FloorToInt(BaseTerrainHeight + Noise * HeightAmplitude);
		}
	}

	// Write in storage order, including Air, so reusing a chunk leaves no stale data.
	for (int32 Z = 0; Z < ChunkSize; ++Z)
	{
		const int64 WorldZ = OriginZ + Z;
		const EVoxelType SolidType = GetSolidTypeAtHeight(WorldZ);

		for (int32 Y = 0; Y < ChunkSize; ++Y)
		{
			for (int32 X = 0; X < ChunkSize; ++X)
			{
				const EVoxelType VoxelType = WorldZ > SurfaceHeights[X + ChunkSize * Y]
					? EVoxelType::Air : SolidType;
				ChunkData.SetVoxel(FIntVector(X, Y, Z), VoxelType);
			}
		}
	}
}
