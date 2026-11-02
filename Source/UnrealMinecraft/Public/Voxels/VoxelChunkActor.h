#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Voxels/VoxelChunkData.h"
#include "VoxelChunkActor.generated.h"

class UMaterialInterface;
class UVoxelMaterialPalette;
class UProceduralMeshComponent;

/** A single world-aligned chunk. ChunkCoordinate determines its world transform. */
UCLASS()
class UNREALMINECRAFT_API AVoxelChunkActor : public AActor
{
	GENERATED_BODY()

public:
	AVoxelChunkActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	int32 WorldSeed = 12345;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	FIntVector ChunkCoordinate = FIntVector::ZeroValue;

	/** Assign an opaque surface material with Vertex Color RGB connected to Base Color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voxel")
	TObjectPtr<UMaterialInterface> VoxelMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	TObjectPtr<UVoxelMaterialPalette> MaterialPalette;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

	/** Also available in Details for previewing terrain before Play. Run on the game thread. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel")
	void GenerateChunk();

	/** Game-thread view, valid until the next generation or unload. */
	const FVoxelChunkData* GetChunkData() const { return ChunkData.GetPtrOrNull(); }

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class AVoxelWorldManager;
	friend class FVoxelMaterialSectionsTest;
	void ApplyBuild(struct FVoxelChunkBuild& Build);
	TArray<TArray<uint32>> AppliedFaces;
	uint64 Revision = 0;
	TOptional<FVoxelChunkData> ChunkData;
};
