#include "Voxels/VoxelCatalog.h"
#include "Misc/DataValidation.h"
#include "UObject/ConstructorHelpers.h"

UVoxelCatalog* UVoxelCatalog::ConstructorDefault()
{
	static ConstructorHelpers::FObjectFinder<UVoxelCatalog> Default(TEXT("/Game/Voxels/DA_VoxelCatalog"));
	return Default.Object;
}

uint8 FVoxelCatalogSnapshot::SolidAtHeight(int64 Z) const
{
	for (const auto& Band : HeightBands)
	{
		if (Z >= Band.Key)
		{
			return Band.Value;
		}
	}
	return HeightBands.Last().Value;
}

const UVoxelDefinition* UVoxelCatalog::Find(uint8 Id) const
{
	const auto* Class = Blocks.Find(Id);
	return Class ? Class->GetDefaultObject() : nullptr;
}

TSharedPtr<const FVoxelCatalogSnapshot> UVoxelCatalog::BuildSnapshot(FString& Error) const
{
	check(IsInGameThread());
	auto Data = MakeShared<FVoxelCatalogSnapshot>();
	Data->Tints.SetNumZeroed(256);
	TMap<UClass*, uint8> Ids;
	for (const auto& Entry : Blocks)
	{
		if (Entry.Key < 1 || Entry.Key > MAX_uint8 || !Entry.Value || Entry.Value->HasAnyClassFlags(CLASS_Abstract)
			|| Ids.Contains(Entry.Value.Get()))
		{
			Error = FString::Printf(TEXT("Block %d needs a unique concrete definition and an ID from 1 to 255."), Entry.Key);
			return nullptr;
		}
		Ids.Add(Entry.Value.Get(), Entry.Key);
		Data->Tints[Entry.Key] = Entry.Value.GetDefaultObject()->Tint;
	}
	for (const auto& Band : HeightBands)
	{
		const uint8* Id = Ids.Find(Band.Value.Get());
		if (!Id)
		{
			Error = FString::Printf(TEXT("Terrain band at Z=%d references an unregistered block."), Band.Key);
			return nullptr;
		}
		Data->HeightBands.Emplace(Band.Key, *Id);
	}
	if (Data->HeightBands.IsEmpty())
	{
		Error = TEXT("Define at least one terrain height band.");
		return nullptr;
	}
	Data->HeightBands.Sort([](const auto& A, const auto& B) { return A.Key > B.Key; });
	return Data;
}

#if WITH_EDITOR
EDataValidationResult UVoxelCatalog::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	FString Error;
	if (!BuildSnapshot(Error))
	{
		Context.AddError(FText::FromString(Error));
		return EDataValidationResult::Invalid;
	}
	return Parent == EDataValidationResult::Invalid ? Parent : EDataValidationResult::Valid;
}
#endif
