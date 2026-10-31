#include "Voxels/VoxelChunkActor.h"

#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Voxels/VoxelMesher.h"
#include "Voxels/VoxelChunkBuild.h"
#include "Voxels/VoxelWorldManager.h"

AVoxelChunkActor::AVoxelChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	SetRootComponent(ProceduralMesh);
	ProceduralMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ProceduralMesh->bUseComplexAsSimpleCollision = true;
	// The component uses synchronous cooking in editor preview worlds.
	ProceduralMesh->bUseAsyncCooking = true;
}

void AVoxelChunkActor::BeginPlay()
{
	Super::BeginPlay();
	if (!Cast<AVoxelWorldManager>(GetOwner()))
	{
		GenerateChunk();
	}
}

void AVoxelChunkActor::GenerateChunk()
{
	if (AVoxelWorldManager* Manager = Cast<AVoxelWorldManager>(GetOwner()))
	{
		Manager->MarkChunkDirty(ChunkCoordinate);
		return;
	}
	const double ChunkWorldSize = FVoxelChunkData::ChunkSize * FVoxelMesher::VoxelSize;
	const FVector ChunkOrigin(
		static_cast<double>(ChunkCoordinate.X) * ChunkWorldSize,
		static_cast<double>(ChunkCoordinate.Y) * ChunkWorldSize,
		static_cast<double>(ChunkCoordinate.Z) * ChunkWorldSize);
	SetActorTransform(FTransform(FQuat::Identity, ChunkOrigin, FVector::OneVector),
		false, nullptr, ETeleportType::TeleportPhysics);

	const uint64 RequestedRevision = ++Revision;
	auto Task = FVoxelChunkBuild::Launch(WorldSeed, ChunkCoordinate);
	UE::Tasks::Launch(UE_SOURCE_LOCATION, [WeakThis = TWeakObjectPtr<AVoxelChunkActor>(this), RequestedRevision, Task]() mutable
	{
		AVoxelChunkActor* Actor = WeakThis.Get();
		if (Actor && Actor->Revision == RequestedRevision)
		{
			Actor->ApplyBuild(Task.GetResult());
		}
	}, UE::Tasks::Prerequisites(Task), UE::Tasks::ETaskPriority::Default, UE::Tasks::EExtendedTaskPriority::GameThreadNormalPri);
}

void AVoxelChunkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	++Revision;
	Super::EndPlay(EndPlayReason);
}

void AVoxelChunkActor::ApplyBuild(FVoxelChunkBuild& Build)
{
	check(IsInGameThread());
	if (Build.GeneratedData.IsSet())
	{
		ChunkData = MoveTemp(Build.GeneratedData);
	}
	const FVoxelMeshData& Mesh = Build.Mesh;
	if (Mesh.Vertices.IsEmpty())
	{
		ProceduralMesh->ClearMeshSection(0);
	}
	else
	{
		// Replace in one submission so the previous collision body survives until the new cook completes.
		const TArray<FProcMeshTangent> Tangents;
		ProceduralMesh->CreateMeshSection_LinearColor(0, Mesh.Vertices, Mesh.Triangles,
			Mesh.Normals, Mesh.UVs, Mesh.VertexColors, Tangents, true, false);
	}
	ProceduralMesh->SetMaterial(0, VoxelMaterial);
}
