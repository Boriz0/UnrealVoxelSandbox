#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ProceduralMeshComponent.h"
#include "Voxels/VoxelChunkActor.h"
#include "Voxels/VoxelChunkBuild.h"
#include "Voxels/VoxelMaterialPalette.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVoxelMaterialSectionsTest, "UnrealMinecraft.Voxels.MaterialSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVoxelMaterialSectionsTest::RunTest(const FString& Parameters)
{
	UVoxelMaterialPalette* Palette = LoadObject<UVoxelMaterialPalette>(nullptr,
		TEXT("/Game/Voxels/Materials/DA_VoxelMaterialPalette.DA_VoxelMaterialPalette"));
	if (!TestNotNull(TEXT("Terrain palette"), Palette))
	{
		return false;
	}
	// A fresh physics scene snapshots the registered query materials on creation.
	for (const auto& Entry : Palette->Materials)
	{
		if (Entry.Value && Entry.Value->GetPhysicalMaterial())
		{
			Entry.Value->GetPhysicalMaterial()->GetPhysicsMaterial();
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
	Data.SetVoxel({0, 0, 0}, EVoxelType::Stone);
	auto Rebuild = [&](EVoxelType Neighbor)
	{
		Data.SetVoxel({1, 0, 0}, Neighbor);
		FVoxelChunkBuild Build;
		Build.Mesh = FVoxelMesher::BuildMesh(Data);
		Chunk->ApplyBuild(Build);
	};
	auto Trace = [&](int32 X, FHitResult& Hit)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(VoxelMaterialTest), true);
		Query.bReturnPhysicalMaterial = true;
		const FVector Start = Origin + FVector(X, 45, 150);
		return World->LineTraceSingleByChannel(Hit, Start, Start - FVector(0, 0, 145), ECC_Visibility, Query);
	};

	Chunk->VoxelMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/Voxels/Materials/M_VoxelVertexColor.M_VoxelVertexColor"));
	Rebuild(EVoxelType::Snow);
	TestNotNull(TEXT("Existing fallback"), Chunk->VoxelMaterial.Get());
	TestTrue(TEXT("Unassigned palette uses fallback"), Chunk->ProceduralMesh->GetMaterial(2) == Chunk->VoxelMaterial);
	Chunk->MaterialPalette = Palette;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		Rebuild(EVoxelType::Snow);
		FHitResult Hit;
		TestTrue(TEXT("Stone hit"), Trace(50, Hit));
		TestEqual(TEXT("Stone physical material"), GetNameSafe(Hit.PhysMaterial.Get()), FString(TEXT("PM_VoxelStone")));
		TestTrue(TEXT("Snow hit"), Trace(150, Hit));
		TestEqual(TEXT("Snow physical material"), GetNameSafe(Hit.PhysMaterial.Get()), FString(TEXT("PM_VoxelSnow")));
		TestTrue(TEXT("Snow render material"), Chunk->ProceduralMesh->GetMaterial(2) == Palette->Materials.FindRef(EVoxelType::Snow));
		Rebuild(EVoxelType::Air);
		TestFalse(TEXT("Removed snow has no collision"), Trace(150, Hit));
		const FProcMeshSection* Snow = Chunk->ProceduralMesh->GetProcMeshSection(2);
		TestTrue(TEXT("Removed snow has no triangles"), !Snow || Snow->ProcIndexBuffer.IsEmpty());
	}
	World->DestroyWorld(false);
	return true;
}

#endif
