#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Voxels/VoxelChunkBuild.h"
#include "VoxelWorldManager.generated.h"

class AVoxelChunkActor;
class UMaterialInterface;
class UVoxelCatalog;

/** Streams a bounded grid around player zero, independent of this Actor's transform. */
UCLASS()
class UNREALMINECRAFT_API AVoxelWorldManager : public AActor
{
	GENERATED_BODY()

public:
	AVoxelWorldManager();

	void SetMenuPreview(AActor* Camera);
	void StartWorld(int32 Seed, const TMap<FIntVector, uint8>& WorldEdits);
	void ExportEdits(TMap<FIntVector, uint8>& OutEdits) const { EditStore.Export(OutEdits); }
	bool IsRestoreAreaReady(FVector Location, double CapsuleRadius) const;
	bool IsInitialGridReady() const;
	uint8 SolidAtHeight(int64 Z) const { return CatalogData->SolidAtHeight(Z); }
	bool HasSurfaceCollision(const FVector& Surface) const;
	/** Game-thread counters. Pending includes queued, generating and awaiting-upload chunks, not remeshes. */
	void GetWorldStatistics(int32& Loaded, int64& Pending, int64& Modified) const;

	/** Game-thread access in voxel units. False means the requested chunk is unavailable. */
	bool GetWorldVoxel(FIntVector WorldVoxel, uint8& OutType) const;
	/** The lowest voxel row is an unbreakable floor, including for negative layer ranges. */
	bool IsBottomVoxel(FIntVector WorldVoxel) const;
	bool SetWorldVoxel(FIntVector WorldVoxel, uint8 Type);

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

	/** Running jobs and completed meshes waiting for upload share this limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "1"))
	int32 MaxConcurrentTasks = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel", meta = (ClampMin = "1"))
	int32 MaxMeshUploadsPerFrame = 1;

	/** Schedule a mesh from the loaded chunk's current voxels. Game thread only. */
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool MarkChunkDirty(FIntVector Coordinate);

	/** Inclusive vertical layer range. Negative layers are supported. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 MinVerticalLayer = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	int32 MaxVerticalLayer = 1;

	/** Shared vertex-color material assigned before each chunk generates its mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	TObjectPtr<UMaterialInterface> VoxelMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	TObjectPtr<UVoxelCatalog> Catalog;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TSharedPtr<const FVoxelCatalogSnapshot> CatalogData;
	void ClearStreamedWorld();
	TWeakObjectPtr<AActor> PreviewCamera;
	bool bPreview = false;
	FVoxelEditStore EditStore;
	void InvalidateNeighbors(FIntVector Chunk, FIntVector EditedLocal);
	void UpdateStreaming(const FIntVector& PlayerChunk);
	AVoxelChunkActor* SpawnChunk(const FIntVector& Coordinate);
	void UploadCompletedBuilds();
	// PendingLoads -> Builds (running or ready) -> SpawnedChunks. A remesh keeps its Actor.
	struct FBuildRequest
	{
		FVoxelChunkBuild::FTask Task;
		TWeakObjectPtr<AVoxelChunkActor> Source;
		uint64 Revision = 0;
		bool bRemesh = false;
		bool bObsolete = false;
	};
	TMap<FIntVector, FBuildRequest> Builds;
	TSet<FIntVector> DirtyChunks;
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
