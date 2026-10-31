#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Voxels/VoxelChunkData.h"
#include "VoxelChunkActor.generated.h"

class UMaterialInterface;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voxel")
	TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

	/** Also available in Details for previewing terrain before Play. Run on the game thread. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Voxel")
	void GenerateChunk();

protected:
	virtual void BeginPlay() override;

private:
	FVoxelChunkData ChunkData;
};
