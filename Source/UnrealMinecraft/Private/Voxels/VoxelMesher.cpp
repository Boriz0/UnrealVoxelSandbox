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
			Face[U + Size * V] = Neighbor.GetVoxel(Local) != VoxelAir;
		}
	}
}

bool FVoxelBorders::IsSolid(FIntVector Outside, int32 Axis, int32 Sign) const
{
	const TBitArray<>& Face = Solid[2 * Axis + (Sign < 0)];
	return Face.Num() && Face[PlaneIndex(Axis, Outside)];
}

FVoxelMeshData FVoxelMesher::BuildMesh(const FVoxelChunkData& ChunkData, const FVoxelBorders& Borders, const TArray<FLinearColor>& Tints)
{
	constexpr int32 Size = FVoxelChunkData::ChunkSize;
	static_assert(Size == 32, "Packed rectangle IDs require 32-voxel chunks.");
	FVoxelMeshData Mesh;
	uint8 Mask[Size * Size];
	for (int32 FaceIndex = 0; FaceIndex < UE_ARRAY_COUNT(VoxelFaceDefinitions); ++FaceIndex)
	{
		const FFaceDefinition& Face = VoxelFaceDefinitions[FaceIndex];
		const int32 Axis = FaceIndex / 2;
		const int32 UAxis = Face.U.X ? 0 : (Face.U.Y ? 1 : 2);
		const int32 VAxis = Face.V.X ? 0 : (Face.V.Y ? 1 : 2);
		// Scan in the original face's UV directions, including its negative axes.
		const auto CoordinatesAt = [&](int32 Slice, int32 U, int32 V)
		{
			FIntVector Local = FIntVector::ZeroValue;
			Local[Axis] = Slice;
			Local[UAxis] = Face.U[UAxis] > 0 ? U : Size - 1 - U;
			Local[VAxis] = Face.V[VAxis] > 0 ? V : Size - 1 - V;
			return Local;
		};
		for (int32 Slice = 0; Slice < Size; ++Slice)
		{
			// Zero means hidden or consumed; material IDs prevent incompatible merges.
			for (int32 V = 0; V < Size; ++V)
			{
				for (int32 U = 0; U < Size; ++U)
				{
					const FIntVector Local = CoordinatesAt(Slice, U, V);
					const FIntVector Adjacent = Local + Face.NeighborOffset;
					const bool bSolid = FVoxelChunkData::IsValidCoordinate(Adjacent)
						? ChunkData.GetVoxel(Adjacent) != VoxelAir
						: Borders.IsSolid(Adjacent, Axis, Face.NeighborOffset[Axis]);
					Mask[U + Size * V] = bSolid ? VoxelAir : ChunkData.GetVoxel(Local);
				}
			}
			for (int32 V = 0; V < Size; ++V)
			{
				for (int32 U = 0; U < Size;)
				{
					const uint8 Type = Mask[U + Size * V];
					if (Type == VoxelAir)
					{
						++U;
						continue;
					}
					int32 Width = 1;
					while (U + Width < Size && Mask[U + Width + Size * V] == Type)
					{
						++Width;
					}
					int32 Height = 1;
					while (V + Height < Size)
					{
						int32 RowWidth = 0;
						while (RowWidth < Width && Mask[U + RowWidth + Size * (V + Height)] == Type)
						{
							++RowWidth;
						}
						if (RowWidth != Width)
						{
							break;
						}
						++Height;
					}
					const int32 Slot = static_cast<int32>(Type) - 1;
					if (Slot >= Mesh.Sections.Num())
					{
						Mesh.Sections.SetNum(Slot + 1);
					}
					FVoxelMeshSection& Section = Mesh.Sections[Slot];
					const FIntVector Local = CoordinatesAt(Slice, U, V);
					const uint32 Cell = Local.X + Size * (Local.Y + Size * Local.Z);
					// Include extents so upload deduplication detects resized rectangles.
					Section.FaceIds.Add((Cell * 6 + FaceIndex)
						| (static_cast<uint32>(Width - 1) << 18) | (static_cast<uint32>(Height - 1) << 23));
					const FVector Origin = (FVector(Local) + FVector(0.5)
						+ (Face.Normal - Face.U - Face.V) * 0.5) * VoxelSize;
					const FVector EdgeU = Face.U * (Width * VoxelSize);
					const FVector EdgeV = Face.V * (Height * VoxelSize);
					const int32 FirstVertex = Section.Vertices.Num();
					Section.Vertices.Append({ Origin, Origin + EdgeU, Origin + EdgeU + EdgeV, Origin + EdgeV });
					Section.Triangles.Append({ FirstVertex, FirstVertex + 2, FirstVertex + 1,
						FirstVertex, FirstVertex + 3, FirstVertex + 2 });
					// Preserve one texture tile per voxel.
					Section.UVs.Append({ FVector2D(0, 0), FVector2D(Width, 0),
						FVector2D(Width, Height), FVector2D(0, Height) });
					for (int32 Corner = 0; Corner < 4; ++Corner)
					{
						Section.Normals.Add(Face.Normal);
						Section.VertexColors.Add(Tints[Type]);
					}
					for (int32 Row = 0; Row < Height; ++Row)
					{
						for (int32 Column = 0; Column < Width; ++Column)
						{
							Mask[U + Column + Size * (V + Row)] = VoxelAir;
						}
					}
					U += Width;
				}
			}
		}
	}
	return Mesh;
}
