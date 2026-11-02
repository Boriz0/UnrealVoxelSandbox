#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Voxels/VoxelChunkData.h"
#include "VoxelMaterialPalette.generated.h"

class UMaterialInterface;

UCLASS(BlueprintType)
class UNREALMINECRAFT_API UVoxelMaterialPalette : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Solid voxel surface materials. Assign each surface's physical material on its material or instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voxel")
	TMap<EVoxelType, TObjectPtr<UMaterialInterface>> Materials;
};
