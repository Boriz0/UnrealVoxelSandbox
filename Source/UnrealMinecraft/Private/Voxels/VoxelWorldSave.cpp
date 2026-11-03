#include "Voxels/VoxelWorldSave.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/Paths.h"
#include "UnrealMinecraft.h"
#include "Voxels/VoxelWorldGenerator.h"

namespace
{
	const TCHAR* Slots[] = {TEXT("VoxelWorld_A"), TEXT("VoxelWorld_B")};
	constexpr int32 User = 0;
	constexpr int32 CurrentFormat = 3;

	FString SlotPath(const TCHAR* Slot)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SaveGames") / (FString(Slot) + TEXT(".sav")));
	}
}

bool UVoxelWorldSave::Exists()
{
	return UGameplayStatics::DoesSaveGameExist(Slots[0], User) || UGameplayStatics::DoesSaveGameExist(Slots[1], User);
}

bool UVoxelWorldSave::IsCompatible() const
{
	return FormatVersion >= 1 && FormatVersion <= CurrentFormat && GeneratorVersion == FVoxelWorldGenerator::Version
		&& Generation > 0 && PlayerTransform.IsValid() && !ViewRotation.ContainsNaN() && Inventory.IsValid();
}

UVoxelWorldSave* UVoxelWorldSave::LoadLatest(int32* OutSlot)
{
	UVoxelWorldSave* Latest = nullptr;
	int32 LatestSlot = INDEX_NONE;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
	{
		const TCHAR* Slot = Slots[Index];
		UE_LOG(LogUnrealMinecraft, Log, TEXT("Checking save slot %s user %d at %s"), Slot, User, *SlotPath(Slot));
		if (!UGameplayStatics::DoesSaveGameExist(Slot, User))
		{
			continue;
		}
		TArray<uint8> Bytes;
		if (!UGameplayStatics::LoadDataFromSlot(Bytes, Slot, User) || Bytes.Num() <= sizeof(uint32))
		{
			UE_LOG(LogUnrealMinecraft, Error, TEXT("Cannot read save data from %s"), Slot);
			continue;
		}
		uint32 StoredCRC;
		const int32 PayloadSize = Bytes.Num() - sizeof(StoredCRC);
		FMemory::Memcpy(&StoredCRC, Bytes.GetData() + PayloadSize, sizeof(StoredCRC));
		if (StoredCRC != FCrc::MemCrc32(Bytes.GetData(), PayloadSize))
		{
			UE_LOG(LogUnrealMinecraft, Error, TEXT("Incomplete or corrupt save in %s (checksum mismatch)"), Slot);
			continue;
		}
		Bytes.SetNum(PayloadSize, EAllowShrinking::No);
		auto* Save = Cast<UVoxelWorldSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
		if (!Save || !Save->IsCompatible())
		{
			UE_LOG(LogUnrealMinecraft, Error, TEXT("Invalid save class, version, transform or edits in %s"), Slot);
			continue;
		}
		UE_LOG(LogUnrealMinecraft, Log, TEXT("Loaded %s checkpoint %lld seed %d generator %d modified voxels %d"),
			Slot, Save->Generation, Save->Seed, Save->GeneratorVersion, Save->Edits.Num());
		if (!Latest || Save->Generation > Latest->Generation)
		{
			Latest = Save;
			LatestSlot = Index;
		}
	}
	if (OutSlot)
	{
		*OutSlot = LatestSlot;
	}
	return Latest;
}

bool UVoxelWorldSave::Commit()
{
	FormatVersion = CurrentFormat;
	GeneratorVersion = FVoxelWorldGenerator::Version;
	int32 LatestSlot;
	const UVoxelWorldSave* Previous = LoadLatest(&LatestSlot);
	const TCHAR* Slot = Slots[LatestSlot == 0 ? 1 : 0];
	UE_LOG(LogUnrealMinecraft, Log, TEXT("Saving %s user %d at %s seed %d generator %d modified voxels %d"),
		Slot, User, *SlotPath(Slot), Seed, GeneratorVersion, Edits.Num());
	if (Previous && Previous->Generation == MAX_int64)
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("Checkpoint sequence exhausted in %s"), Slot);
		return false;
	}
	Generation = Previous ? Previous->Generation + 1 : 1;
	TArray<uint8> Bytes;
	if (!IsCompatible() || !UGameplayStatics::SaveGameToMemory(this, Bytes) || Bytes.IsEmpty())
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("Validation or serialization failed for %s"), Slot);
		return false;
	}
	// Write only the inactive slot. A truncated write cannot supersede the last valid checkpoint.
	const uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	Bytes.Append(reinterpret_cast<const uint8*>(&CRC), sizeof(CRC));
	TArray<uint8> ReadBack;
	if (!UGameplayStatics::SaveDataToSlot(Bytes, Slot, User)
		|| !UGameplayStatics::LoadDataFromSlot(ReadBack, Slot, User) || ReadBack != Bytes)
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("Save write or read-back verification failed for %s"), *SlotPath(Slot));
		return false;
	}
	UE_LOG(LogUnrealMinecraft, Log, TEXT("Committed and verified %s checkpoint %lld (%d bytes)"), Slot, Generation, Bytes.Num());
	return true;
}
