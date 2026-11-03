#pragma once

#include "CoreMinimal.h"
#include "VoxelDefinition.generated.h"

class UMaterialInterface;

/** Create child Blueprints and edit Class Defaults to author block families. */
UCLASS(Blueprintable, BlueprintType, Abstract)
class UNREALMINECRAFT_API UVoxelDefinition : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voxel")
	FLinearColor Tint = FLinearColor::White;

	/** Assign a physical material on this material or material instance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voxel")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Voxel", meta=(ClampMin="0", Units="s"))
	float MiningSeconds = 1.f;
};
