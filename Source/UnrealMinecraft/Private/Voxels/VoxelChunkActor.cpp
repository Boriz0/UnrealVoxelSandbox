#include "Voxels/VoxelChunkActor.h"

#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Voxels/VoxelMesher.h"
#include "Voxels/VoxelCatalog.h"
#include "Voxels/VoxelChunkBuild.h"
#include "Voxels/VoxelWorldManager.h"

AVoxelChunkActor::AVoxelChunkActor()
{
	Catalog = UVoxelCatalog::ConstructorDefault();
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
	FString Error;
	auto Data = Catalog ? Catalog->BuildSnapshot(Error) : nullptr;
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("%s requires a valid voxel catalog: %s"), *GetName(), *Error);
		return;
	}
	// Standalone editor regeneration can change the definition tint without changing any faces.
	AppliedFaces.Reset();
	auto Task = FVoxelChunkBuild::Launch(Data.ToSharedRef(), WorldSeed, ChunkCoordinate);
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
	const int32 NumSlots = FMath::Max(ProceduralMesh->GetNumSections(), Build.Mesh.Sections.Num());
	AppliedFaces.SetNum(NumSlots);
	for (int32 Slot = 0; Slot < NumSlots; ++Slot)
	{
		FVoxelMeshSection* Section = Build.Mesh.Sections.IsValidIndex(Slot) ? &Build.Mesh.Sections[Slot] : nullptr;
		if (!Section || Section->Vertices.IsEmpty())
		{
			const FProcMeshSection* Previous = ProceduralMesh->GetProcMeshSection(Slot);
			if (Previous && !Previous->ProcIndexBuffer.IsEmpty())
			{
				ProceduralMesh->ClearMeshSection(Slot);
			}
			AppliedFaces[Slot].Reset();
			continue;
		}

		const uint8 Type = static_cast<uint8>(Slot + 1);
		const UVoxelDefinition* Definition = Catalog ? Catalog->Find(Type) : nullptr;
		UMaterialInterface* Material = Definition ? Definition->Material.Get() : nullptr;
		ProceduralMesh->SetMaterial(Slot, Material ? Material : VoxelMaterial.Get());
		// Face IDs determine every vertex attribute within a type. Skip redundant uploads and cooks.
		if (AppliedFaces[Slot] != Section->FaceIds)
		{
			const TArray<FProcMeshTangent> Tangents;
			ProceduralMesh->CreateMeshSection_LinearColor(Slot, Section->Vertices, Section->Triangles,
				Section->Normals, Section->UVs, Section->VertexColors, Tangents, true, false);
			AppliedFaces[Slot] = MoveTemp(Section->FaceIds);
		}
	}
}
