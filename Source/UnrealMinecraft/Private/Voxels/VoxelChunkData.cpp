#include "Voxels/VoxelChunkData.h"

FVoxelChunkData::FVoxelChunkData()
{
	Voxels.Init(VoxelAir, VoxelCount);
}

uint8 FVoxelChunkData::GetVoxel(const FIntVector& Coordinates) const
{
	if (!IsValidCoordinate(Coordinates))
	{
		return VoxelAir;
	}

	return Voxels[GetIndex(Coordinates)];
}

bool FVoxelChunkData::SetVoxel(const FIntVector& Coordinates, uint8 VoxelType)
{
	if (!IsValidCoordinate(Coordinates))
	{
		return false;
	}

	Voxels[GetIndex(Coordinates)] = VoxelType;
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
