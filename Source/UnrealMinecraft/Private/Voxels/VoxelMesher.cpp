#include "Voxels/VoxelMesher.h"

#include "Voxels/VoxelChunkData.h"

namespace
{
	struct FFaceDefinition
	{
		FIntVector NeighborOffset;
		FVector Normal;
		FVector U;
		FVector V;
	};

	// U cross V points outward. Triangle winding is reversed for Unreal's front faces.
	const FFaceDefinition Faces[] =
	{
		{ FIntVector(1, 0, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1) },
		{ FIntVector(-1, 0, 0), FVector(-1, 0, 0), FVector(0, -1, 0), FVector(0, 0, 1) },
		{ FIntVector(0, 1, 0), FVector(0, 1, 0), FVector(-1, 0, 0), FVector(0, 0, 1) },
		{ FIntVector(0, -1, 0), FVector(0, -1, 0), FVector(1, 0, 0), FVector(0, 0, 1) },
		{ FIntVector(0, 0, 1), FVector(0, 0, 1), FVector(1, 0, 0), FVector(0, 1, 0) },
		{ FIntVector(0, 0, -1), FVector(0, 0, -1), FVector(-1, 0, 0), FVector(0, 1, 0) }
	};

	EVoxelType GetNeighborVoxel(const FVoxelChunkData& ChunkData, const FIntVector& Coordinates)
	{
		if (!FVoxelChunkData::IsValidCoordinate(Coordinates))
		{
			// Future chunk-neighbor sampling belongs here.
			return EVoxelType::Air;
		}
		return ChunkData.GetVoxel(Coordinates);
	}

	FLinearColor GetVoxelColor(EVoxelType VoxelType)
	{
		switch (VoxelType)
		{
		case EVoxelType::Stone: return FLinearColor(0.4f, 0.4f, 0.4f);
		case EVoxelType::Grass: return FLinearColor(0.1f, 0.6f, 0.1f);
		case EVoxelType::Snow: return FLinearColor::White;
		default: return FLinearColor::White;
		}
	}
}

FVoxelMeshData FVoxelMesher::BuildMesh(const FVoxelChunkData& ChunkData)
{
	FVoxelMeshData Mesh;
	for (int32 Z = 0; Z < FVoxelChunkData::ChunkSize; ++Z)
	{
		for (int32 Y = 0; Y < FVoxelChunkData::ChunkSize; ++Y)
		{
			for (int32 X = 0; X < FVoxelChunkData::ChunkSize; ++X)
			{
				const FIntVector Coordinates(X, Y, Z);
				const EVoxelType VoxelType = ChunkData.GetVoxel(Coordinates);
				if (VoxelType == EVoxelType::Air)
				{
					continue;
				}

				const FLinearColor Color = GetVoxelColor(VoxelType);
				const FVector Center = (FVector(X, Y, Z) + FVector(0.5)) * VoxelSize;
				for (const FFaceDefinition& Face : Faces)
				{
					if (GetNeighborVoxel(ChunkData, Coordinates + Face.NeighborOffset) != EVoxelType::Air)
					{
						continue;
					}

					const int32 FirstVertex = Mesh.Vertices.Num();
					const FVector FaceCenter = Center + Face.Normal * (VoxelSize * 0.5);
					const FVector HalfU = Face.U * (VoxelSize * 0.5);
					const FVector HalfV = Face.V * (VoxelSize * 0.5);
					Mesh.Vertices.Append({ FaceCenter - HalfU - HalfV, FaceCenter + HalfU - HalfV,
						FaceCenter + HalfU + HalfV, FaceCenter - HalfU + HalfV });
					Mesh.Triangles.Append({ FirstVertex, FirstVertex + 2, FirstVertex + 1,
						FirstVertex, FirstVertex + 3, FirstVertex + 2 });
					Mesh.UVs.Append({ FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1) });
					for (int32 Corner = 0; Corner < 4; ++Corner)
					{
						Mesh.Normals.Add(Face.Normal);
						Mesh.VertexColors.Add(Color);
					}
				}
			}
		}
	}
	return Mesh;
}
