#pragma once

#include "Engine/DataAsset.h"
#include "Voxels/VoxelDefinition.h"
#include "VoxelCatalog.generated.h"

struct FVoxelCatalogSnapshot
{
	TArray<FLinearColor> Tints;
	TArray<TPair<int32, uint8>> HeightBands;
	uint8 SolidAtHeight(int64 Z) const;
};

UCLASS(BlueprintType)
class UNREALMINECRAFT_API UVoxelCatalog : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Permanent save IDs, 1 through 255. Zero is Air. Never reuse retired IDs. */
	UPROPERTY(EditAnywhere, Category="Blocks")
	TMap<int32, TSubclassOf<UVoxelDefinition>> Blocks;

	/** Inclusive minimum voxel Z mapped to a registered definition. The lowest band extends downward. */
	UPROPERTY(EditAnywhere, Category="Terrain")
	TMap<int32, TSubclassOf<UVoxelDefinition>> HeightBands;

	/** Shared default for newly placed chunk and manager actors. Constructor use only. */
	static UVoxelCatalog* ConstructorDefault();
	const UVoxelDefinition* Find(uint8 Id) const;
	TSharedPtr<const FVoxelCatalogSnapshot> BuildSnapshot(FString& Error) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
