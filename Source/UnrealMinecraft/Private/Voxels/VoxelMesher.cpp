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
	const FFaceDefinition VoxelFaceDefinitions[] =
	{
		{ FIntVector(1, 0, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1) },
		{ FIntVector(-1, 0, 0), FVector(-1, 0, 0), FVector(0, -1, 0), FVector(0, 0, 1) },
		{ FIntVector(0, 1, 0), FVector(0, 1, 0), FVector(-1, 0, 0), FVector(0, 0, 1) },
		{ FIntVector(0, -1, 0), FVector(0, -1, 0), FVector(1, 0, 0), FVector(0, 0, 1) },
		{ FIntVector(0, 0, 1), FVector(0, 0, 1), FVector(1, 0, 0), FVector(0, 1, 0) },
		{ FIntVector(0, 0, -1), FVector(0, 0, -1), FVector(-1, 0, 0), FVector(0, 1, 0) }
	};

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

int32 FVoxelBorders::TouchingLayer(int32 Sign)
{
	return Sign > 0 ? 0 : FVoxelChunkData::ChunkSize - 1;
}

int32 FVoxelBorders::PlaneIndex(int32 Axis, FIntVector Local)
{
	return Local[(Axis + 1) % 3] + FVoxelChunkData::ChunkSize * Local[(Axis + 2) % 3];
}

TBitArray<>& FVoxelBorders::InitFace(int32 Axis, int32 Sign)
{
	constexpr int32 Size = FVoxelChunkData::ChunkSize;
	TBitArray<>& Face = Solid[2 * Axis + (Sign < 0)];
	Face.Init(false, Size * Size);
	return Face;
}

void FVoxelBorders::Capture(int32 Axis, int32 Sign, const FVoxelChunkData& Neighbor)
{
	constexpr int32 Size = FVoxelChunkData::ChunkSize;
	TBitArray<>& Face = InitFace(Axis, Sign);
	FIntVector Local = FIntVector::ZeroValue;
	Local[Axis] = TouchingLayer(Sign);
	for (int32 V = 0; V < Size; ++V)
	{
		for (int32 U = 0; U < Size; ++U)
		{
			Local[(Axis + 1) % 3] = U;
			Local[(Axis + 2) % 3] = V;
			Face[U + Size * V] = Neighbor.GetVoxel(Local) != EVoxelType::Air;
		}
	}
}

bool FVoxelBorders::IsSolid(FIntVector Outside, int32 Axis, int32 Sign) const
{
	const TBitArray<>& Face = Solid[2 * Axis + (Sign < 0)];
	return Face.Num() && Face[PlaneIndex(Axis, Outside)];
}

FVoxelMeshData FVoxelMesher::BuildMesh(const FVoxelChunkData& ChunkData, const FVoxelBorders& Borders)
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
				for (const FFaceDefinition& Face : VoxelFaceDefinitions)
				{
					const FIntVector Adjacent = Coordinates + Face.NeighborOffset;
					bool bSolid;
					if (FVoxelChunkData::IsValidCoordinate(Adjacent))
					{
						bSolid = ChunkData.GetVoxel(Adjacent) != EVoxelType::Air;
					}
					else
					{
						const int32 Axis = Face.NeighborOffset.X ? 0 : (Face.NeighborOffset.Y ? 1 : 2);
						bSolid = Borders.IsSolid(Adjacent, Axis, Face.NeighborOffset[Axis]);
					}
					if (bSolid)
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
