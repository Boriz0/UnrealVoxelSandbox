#include "UnrealMinecraftCharacter.h"
#include "UnrealMinecraftPlayerController.h"

#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Voxels/VoxelChunkActor.h"
#include "Voxels/VoxelCatalog.h"
#include "Voxels/VoxelWorldManager.h"

void AUnrealMinecraftCharacter::BindVoxelInput(UEnhancedInputComponent* Input)
{
	RemoveVoxelInput();
	const APlayerController* PC = Cast<APlayerController>(GetController());
	auto* Subsystem = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	if (!Subsystem)
	{
		return;
	}

	VoxelContext = NewObject<UInputMappingContext>(this);
	UInputAction* Mine = NewObject<UInputAction>(VoxelContext);
	UInputAction* Place = NewObject<UInputAction>(VoxelContext);
	Mine->ValueType = Place->ValueType = EInputActionValueType::Boolean;
	Mine->bConsumeInput = Place->bConsumeInput = false;
	VoxelContext->MapKey(Mine, EKeys::LeftMouseButton);
	VoxelContext->MapKey(Place, EKeys::RightMouseButton);
	const FKey SlotKeys[] = {EKeys::One, EKeys::Two, EKeys::Three};
	for (int32 Slot = 0; Slot < FVoxelInventory::SlotCount; ++Slot)
	{
		UInputAction* Select = NewObject<UInputAction>(VoxelContext);
		VoxelContext->MapKey(Select, SlotKeys[Slot]);
		VoxelBindingHandles.Add(Input->BindAction(Select, ETriggerEvent::Started, this, &ThisClass::SelectVoxelSlot, Slot).GetHandle());
	}
	UInputAction* SpeedUp = NewObject<UInputAction>(VoxelContext);
	UInputAction* SpeedDown = NewObject<UInputAction>(VoxelContext);
	SpeedUp->ValueType = SpeedDown->ValueType = EInputActionValueType::Boolean;
	VoxelContext->MapKey(SpeedUp, EKeys::MouseScrollUp);
	VoxelContext->MapKey(SpeedDown, EKeys::MouseScrollDown);
	UInputAction* RadiusUp = NewObject<UInputAction>(VoxelContext);
	UInputAction* RadiusDown = NewObject<UInputAction>(VoxelContext);
	RadiusUp->ValueType = RadiusDown->ValueType = EInputActionValueType::Boolean;
	VoxelContext->MapKey(RadiusUp, EKeys::PageUp);
	VoxelContext->MapKey(RadiusDown, EKeys::PageDown);
	VoxelBindingHandles.Add(Input->BindAction(RadiusUp, ETriggerEvent::Started, this, &ThisClass::AdjustChunkRadii, true).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(RadiusDown, ETriggerEvent::Started, this, &ThisClass::AdjustChunkRadii, false).GetHandle());
	BoundVoxelInput = Input;
	UInputAction* FlightToggle = NewObject<UInputAction>(VoxelContext);
	UInputAction* LightingToggle = NewObject<UInputAction>(VoxelContext);
	VoxelContext->MapKey(LightingToggle, EKeys::L);
	VoxelBindingHandles.Add(Input->BindAction(LightingToggle, ETriggerEvent::Started, this, &ThisClass::ToggleLumenLighting).GetHandle());
	UInputAction* Descend = NewObject<UInputAction>(VoxelContext);
	VoxelContext->MapKey(FlightToggle, EKeys::LeftShift);
	VoxelContext->MapKey(Descend, EKeys::LeftControl);
	VoxelContext->MapKey(Descend, EKeys::RightControl);
	VoxelBindingHandles.Add(Input->BindAction(FlightToggle, ETriggerEvent::Started, this, &ThisClass::ToggleFlight).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Descend, ETriggerEvent::Triggered, this, &ThisClass::FlyDown).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(SpeedUp, ETriggerEvent::Triggered, this, &ThisClass::IncreaseRunningSpeed).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(SpeedDown, ETriggerEvent::Triggered, this, &ThisClass::DecreaseRunningSpeed).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Mine, ETriggerEvent::Triggered, this, &ThisClass::UpdateMining).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Mine, ETriggerEvent::Completed, this, &ThisClass::ResetMining).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Mine, ETriggerEvent::Canceled, this, &ThisClass::ResetMining).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Place, ETriggerEvent::Started, this, &ThisClass::PlaceVoxel).GetHandle());
	VoxelInputSubsystem = Subsystem;
	Subsystem->AddMappingContext(VoxelContext, -1);
}

void AUnrealMinecraftCharacter::AdjustChunkRadii(bool bIncrease)
{
	const auto* PC = Cast<AUnrealMinecraftPlayerController>(GetController());
	if (PC && PC->SessionState == EVoxelSessionState::Playing)
	{
		if (AVoxelWorldManager* Manager = PC->GetVoxelWorldManager())
		{
			Manager->AdjustStreamingRadii(bIncrease);
		}
	}
}

void AUnrealMinecraftCharacter::RemoveVoxelInput()
{
	if (VoxelInputSubsystem.IsValid() && VoxelContext)
	{
		VoxelInputSubsystem->RemoveMappingContext(VoxelContext);
	}
	if (BoundVoxelInput.IsValid())
	{
		for (uint32 Handle : VoxelBindingHandles)
		{
			BoundVoxelInput->RemoveBindingByHandle(Handle);
		}
	}
	VoxelBindingHandles.Reset();
	BoundVoxelInput.Reset();
	VoxelInputSubsystem.Reset();
	VoxelContext = nullptr;
	ResetMining();
}

void AUnrealMinecraftCharacter::UnPossessed()
{
	RemoveVoxelInput();
	Super::UnPossessed();
}

void AUnrealMinecraftCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveVoxelInput();
	Super::EndPlay(EndPlayReason);
}

AVoxelWorldManager* AUnrealMinecraftCharacter::TraceVoxel(FIntVector& Voxel, FIntVector& Face, uint8& Type) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || InteractionRange <= 0)
	{
		return nullptr;
	}
	FVector Start;
	FRotator Rotation;
	PC->GetPlayerViewPoint(Start, Rotation);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VoxelInteraction), true, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Rotation.Vector() * InteractionRange, ECC_Visibility, Params))
	{
		return nullptr;
	}
	const AVoxelChunkActor* Chunk = Cast<AVoxelChunkActor>(Hit.GetActor());
	AVoxelWorldManager* Manager = Chunk ? Cast<AVoxelWorldManager>(Chunk->GetOwner()) : nullptr;
	if (!Manager || Hit.bStartPenetrating)
	{
		return nullptr;
	}

	const int32 Axis = Hit.ImpactNormal.GetAbsMax() == FMath::Abs(Hit.ImpactNormal.Z) ? 2
		: (FMath::Abs(Hit.ImpactNormal.X) >= FMath::Abs(Hit.ImpactNormal.Y) ? 0 : 1);
	Face = FIntVector::ZeroValue;
	Face[Axis] = Hit.ImpactNormal[Axis] > 0 ? 1 : -1;
	// Sample just inside the hit face. Floor handles both sides of world zero.
	const FVector Inside = (Hit.ImpactPoint - FVector(Face) * 0.1) / FVoxelMesher::VoxelSize;
	for (int32 I = 0; I < 3; ++I)
	{
		const int64 Cell = FMath::FloorToInt64(Inside[I]);
		if (Cell < MIN_int32 || Cell > MAX_int32 || Cell + Face[I] < MIN_int32 || Cell + Face[I] > MAX_int32)
		{
			return nullptr;
		}
		Voxel[I] = static_cast<int32>(Cell);
	}
	return Manager->GetWorldVoxel(Voxel, Type) && Type != VoxelAir ? Manager : nullptr;
}

void AUnrealMinecraftCharacter::ResetMining()
{
	MiningManager.Reset();
}

void AUnrealMinecraftCharacter::UpdateMining()
{
	FIntVector Voxel, Face;
	uint8 Type;
	AVoxelWorldManager* Manager = TraceVoxel(Voxel, Face, Type);
	if (!Manager || Manager->IsBottomVoxel(Voxel))
	{
		ResetMining();
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (MiningManager.Get() != Manager || MiningVoxel != Voxel || MiningType != Type)
	{
		MiningManager = Manager;
		MiningVoxel = Voxel;
		MiningType = Type;
		MiningStartTime = Now;
	}
	if (Now - MiningStartTime >= Manager->Catalog->Find(Type)->MiningSeconds)
	{
		const int32 Slot = Inventory.SlotForType(Type);
		if (Slot != INDEX_NONE && Inventory.Counts[Slot] < MAX_int32 && Manager->SetWorldVoxel(Voxel, VoxelAir))
		{
			++Inventory.Counts[Slot];
		}
		ResetMining();
	}
}

bool AUnrealMinecraftCharacter::GetVoxelAimFeedback(FIntVector& Voxel, FIntVector& Face, float& Progress, bool& bCanPlace) const
{
	uint8 Type;
	AVoxelWorldManager* Manager = TraceVoxel(Voxel, Face, Type);
	if (!Manager)
	{
		return false;
	}
	bCanPlace = CanPlaceVoxel(*Manager, Voxel + Face);
	Progress = -1.f;
	if (!Manager->IsBottomVoxel(Voxel) && MiningManager.Get() == Manager && MiningVoxel == Voxel && MiningType == Type)
	{
		const float Duration = Manager->Catalog->Find(Type)->MiningSeconds;
		Progress = Duration > 0 ? FMath::Clamp((GetWorld()->GetTimeSeconds() - MiningStartTime) / Duration, 0., 1.) : 1.f;
	}
	return true;
}

bool AUnrealMinecraftCharacter::CanPlaceVoxel(AVoxelWorldManager& Manager, const FIntVector& Target) const
{
	uint8 Type;
	if (Inventory.Counts[Inventory.SelectedSlot] == 0 || !Manager.GetWorldVoxel(Target, Type) || Type != VoxelAir)
	{
		return false;
	}
	const FVector Center = (FVector(Target) + FVector(0.5)) * FVoxelMesher::VoxelSize;
	const FCollisionShape Box = FCollisionShape::MakeBox(FVector(FVoxelMesher::VoxelSize * 0.5));
	return !GetCapsuleComponent()->OverlapComponent(Center, FQuat::Identity, Box);
}

void AUnrealMinecraftCharacter::PlaceVoxel()
{
	FIntVector Voxel, Face;
	uint8 Type;
	AVoxelWorldManager* Manager = TraceVoxel(Voxel, Face, Type);
	if (Manager && CanPlaceVoxel(*Manager, Voxel + Face))
	{
		const FIntVector Target = Voxel + Face;
		if (Manager->SetWorldVoxel(Target, FVoxelInventory::Types[Inventory.SelectedSlot]))
		{
			--Inventory.Counts[Inventory.SelectedSlot];
		}
	}
}

void AUnrealMinecraftCharacter::SelectVoxelSlot(int32 Slot)
{
	Inventory.SelectedSlot = Slot;
}
