#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelWorldManager.generated.h"

class AVoxelChunkActor;
class UMaterialInterface;

/** Streams a bounded grid around player zero, independent of this Actor's transform. */
UCLASS()
class UNREALMINECRAFT_API AVoxelWorldManager : public AActor
{
	GENERATED_BODY()

public:
	AVoxelWorldManager();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 WorldSeed = 12345;

	/** Inclusive square load radius in X/Y around the player's chunk. Configure before Play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "0", UIMin = "0"))
	int32 HorizontalRadius = 2;

	/** Retain chunks within this square radius; must exceed HorizontalRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "1", UIMin = "1"))
	int32 UnloadRadius = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxChunksPerFrame = 2;

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
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UpdateStreaming(const FIntVector& PlayerChunk);
	void SpawnChunk(const FIntVector& Coordinate);
	bool IsWithinHorizontalRadius(const FIntVector& Coordinate, int32 Radius) const;

	TArray<FIntVector> PendingLoads;
	int32 NextPendingLoad = 0;
	FIntVector StreamingCenter = FIntVector::ZeroValue;
	bool bHasStreamingCenter = false;
	bool bNeedsStreamingUpdate = false;

	UFUNCTION()
	void HandleChunkDestroyed(AActor* DestroyedActor);

	/** Only chunks owned by this manager; never serialized into the level. */
	UPROPERTY(Transient)
	TMap<FIntVector, AVoxelChunkActor*> SpawnedChunks;
};
