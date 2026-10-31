#include "Voxels/VoxelChunkData.h"

FVoxelChunkData::FVoxelChunkData()
{
	Voxels.Init(static_cast<uint8>(EVoxelType::Air), VoxelCount);
}

EVoxelType FVoxelChunkData::GetVoxel(const FIntVector& Coordinates) const
{
	if (!IsValidCoordinate(Coordinates))
	{
		return EVoxelType::Air;
	}

	return static_cast<EVoxelType>(Voxels[GetIndex(Coordinates)]);
}

bool FVoxelChunkData::SetVoxel(const FIntVector& Coordinates, EVoxelType VoxelType)
{
	if (!IsValidCoordinate(Coordinates))
	{
		return false;
	}

	Voxels[GetIndex(Coordinates)] = static_cast<uint8>(VoxelType);
	return true;
}

bool FVoxelChunkData::IsValidCoordinate(const FIntVector& Coordinates)
{
	return Coordinates.X >= 0 && Coordinates.X < ChunkSize
		&& Coordinates.Y >= 0 && Coordinates.Y < ChunkSize
		&& Coordinates.Z >= 0 && Coordinates.Z < ChunkSize;
}

int32 FVoxelChunkData::GetIndex(const FIntVector& Coordinates)
{
	return Coordinates.X + ChunkSize * (Coordinates.Y + ChunkSize * Coordinates.Z);
}
