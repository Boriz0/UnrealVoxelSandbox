#include "Voxels/VoxelChunkActor.h"

#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Voxels/VoxelMesher.h"
#include "Voxels/VoxelWorldGenerator.h"

AVoxelChunkActor::AVoxelChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	SetRootComponent(ProceduralMesh);
	ProceduralMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->bUseComplexAsSimpleCollision = true;
	// Ensure collision exists as soon as generation finishes at BeginPlay.
	ProceduralMesh->bUseAsyncCooking = false;
}

void AVoxelChunkActor::BeginPlay()
{
	Super::BeginPlay();
	GenerateChunk();
}

void AVoxelChunkActor::GenerateChunk()
{
	const double ChunkWorldSize = FVoxelChunkData::ChunkSize * FVoxelMesher::VoxelSize;
	const FVector ChunkOrigin(
		static_cast<double>(ChunkCoordinate.X) * ChunkWorldSize,
		static_cast<double>(ChunkCoordinate.Y) * ChunkWorldSize,
		static_cast<double>(ChunkCoordinate.Z) * ChunkWorldSize);
	SetActorTransform(FTransform(FQuat::Identity, ChunkOrigin, FVector::OneVector),
		false, nullptr, ETeleportType::TeleportPhysics);

	FVoxelWorldGenerator::GenerateChunk(ChunkData, WorldSeed, ChunkCoordinate);
	const FVoxelMeshData Mesh = FVoxelMesher::BuildMesh(ChunkData);
	ProceduralMesh->ClearAllMeshSections();
	if (!Mesh.Vertices.IsEmpty())
	{
		const TArray<FProcMeshTangent> Tangents;
		ProceduralMesh->CreateMeshSection_LinearColor(0, Mesh.Vertices, Mesh.Triangles,
			Mesh.Normals, Mesh.UVs, Mesh.VertexColors, Tangents, true, false);
	}
	ProceduralMesh->SetMaterial(0, VoxelMaterial);
}
