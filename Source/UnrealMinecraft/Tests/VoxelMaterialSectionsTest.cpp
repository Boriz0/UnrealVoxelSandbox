#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ProceduralMeshComponent.h"
#include "Voxels/VoxelChunkActor.h"
#include "Voxels/VoxelChunkBuild.h"
#include "Voxels/VoxelCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVoxelMaterialSectionsTest, "UnrealMinecraft.Voxels.MaterialSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVoxelMaterialSectionsTest::RunTest(const FString& Parameters)
{
	UVoxelCatalog* Catalog = LoadObject<UVoxelCatalog>(nullptr,
		TEXT("/Game/Voxels/DA_VoxelCatalog.DA_VoxelCatalog"));
	if (!TestNotNull(TEXT("Terrain catalog"), Catalog))
	{
		return false;
	}
	FString Error;
	auto Snapshot = Catalog->BuildSnapshot(Error);
	if (!TestTrue(FString::Printf(TEXT("Valid catalog: %s"), *Error), Snapshot.IsValid()))
	{
		return false;
	}
	// A fresh physics scene snapshots the registered query materials on creation.
	for (const auto& Entry : Catalog->Blocks)
	{
		if (Entry.Value && Entry.Value.GetDefaultObject()->Material && Entry.Value.GetDefaultObject()->Material->GetPhysicalMaterial())
		{
			Entry.Value.GetDefaultObject()->Material->GetPhysicalMaterial()->GetPhysicsMaterial();
		}
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	if (!TestNotNull(TEXT("Preview world"), World))
	{
		return false;
	}
	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags = RF_Transient;
	const FVector Origin(0, 0, 500000);
	AVoxelChunkActor* Chunk = World->SpawnActor<AVoxelChunkActor>(Origin, FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("Test chunk"), Chunk))
	{
		World->DestroyWorld(false);
		return false;
	}

	FVoxelChunkData Data;
	Data.SetVoxel({0, 0, 0}, 1);
	auto Rebuild = [&](uint8 Neighbor)
	{
		Data.SetVoxel({1, 0, 0}, Neighbor);
		FVoxelChunkBuild Build;
		Build.Mesh = FVoxelMesher::BuildMesh(Data, {}, Snapshot->Tints);
		Chunk->ApplyBuild(Build);
	};
	auto Trace = [&](int32 X, FHitResult& Hit)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(VoxelMaterialTest), true);
		Query.bReturnPhysicalMaterial = true;
		const FVector Start = Origin + FVector(X, 45, 150);
		return World->LineTraceSingleByChannel(Hit, Start, Start - FVector(0, 0, 145), ECC_Visibility, Query);
	};

	Chunk->Catalog = nullptr;
	Chunk->VoxelMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Voxels/Materials/M_VoxelVertexColor.M_VoxelVertexColor"));
	Rebuild(3);
	TestNotNull(TEXT("Existing fallback"), Chunk->VoxelMaterial.Get());
	TestTrue(TEXT("Unassigned catalog uses fallback"), Chunk->ProceduralMesh->GetMaterial(2) == Chunk->VoxelMaterial);
	Chunk->Catalog = Catalog;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		Rebuild(3);
		FHitResult Hit;
		TestTrue(TEXT("Stone hit"), Trace(50, Hit));
		TestEqual(TEXT("Stone physical material"), GetNameSafe(Hit.PhysMaterial.Get()), FString(TEXT("PM_VoxelStone")));
		TestTrue(TEXT("Snow hit"), Trace(150, Hit));
		TestEqual(TEXT("Snow physical material"), GetNameSafe(Hit.PhysMaterial.Get()), FString(TEXT("PM_VoxelSnow")));
		TestTrue(TEXT("Snow render material"), Chunk->ProceduralMesh->GetMaterial(2) == Catalog->Find(3)->Material);
		Rebuild(VoxelAir);
		TestFalse(TEXT("Removed snow has no collision"), Trace(150, Hit));
		const FProcMeshSection* Snow = Chunk->ProceduralMesh->GetProcMeshSection(2);
		TestTrue(TEXT("Removed snow has no triangles"), !Snow || Snow->ProcIndexBuffer.IsEmpty());
	}
	World->DestroyWorld(false);
	return true;
}

#endif
