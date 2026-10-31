#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelWorldManager.generated.h"

class AVoxelChunkActor;
class UMaterialInterface;

/** Owns a fixed grid of chunks around world origin, independent of this Actor's transform. */
UCLASS()
class UNREALMINECRAFT_API AVoxelWorldManager : public AActor
{
	GENERATED_BODY()

public:
	AVoxelWorldManager();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 WorldSeed = 12345;

	/** Inclusive X/Y range: [-HorizontalRadius, HorizontalRadius]. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "0", UIMin = "0"))
	int32 HorizontalRadius = 2;

	/** Inclusive vertical layer range. Negative layers are supported. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 MinVerticalLayer = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 MaxVerticalLayer = 1;

	/** Shared vertex-color material assigned before each chunk generates its mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	TObjectPtr<UMaterialInterface> VoxelMaterial;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SpawnChunk(const FIntVector& Coordinate);

	UFUNCTION()
	void HandleChunkDestroyed(AActor* DestroyedActor);

	/** Only chunks owned by this manager; never serialized into the level. */
	UPROPERTY(Transient)
	TMap<FIntVector, AVoxelChunkActor*> SpawnedChunks;
};
