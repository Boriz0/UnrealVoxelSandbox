#pragma once

#include "CoreMinimal.h"
#include "VoxelInventory.generated.h"

USTRUCT()
struct FVoxelInventory
{
	GENERATED_BODY()

	// Hotbar order is independent of the catalog's permanent save IDs.
	inline static constexpr uint8 Types[] = {2, 1, 3}; // Grass, Stone, Snow
	static constexpr int32 SlotCount = UE_ARRAY_COUNT(Types);

	UPROPERTY() int32 Counts[3] = {};
	UPROPERTY() int32 SelectedSlot = 0;

	int32 SlotForType(uint8 Type) const
	{
		return MakeArrayView(Types).Find(Type);
	}

	bool IsValid() const
	{
		return SelectedSlot >= 0 && SelectedSlot < SlotCount
			&& Counts[0] >= 0 && Counts[1] >= 0 && Counts[2] >= 0;
	}
};
