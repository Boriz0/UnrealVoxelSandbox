#include "UnrealMinecraftCharacter.h"

#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Voxels/VoxelChunkActor.h"
#include "Voxels/VoxelWorldGenerator.h"
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
	BoundVoxelInput = Input;
	VoxelBindingHandles.Add(Input->BindAction(Mine, ETriggerEvent::Triggered, this, &ThisClass::UpdateMining).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Mine, ETriggerEvent::Completed, this, &ThisClass::ResetMining).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Mine, ETriggerEvent::Canceled, this, &ThisClass::ResetMining).GetHandle());
	VoxelBindingHandles.Add(Input->BindAction(Place, ETriggerEvent::Started, this, &ThisClass::PlaceVoxel).GetHandle());
	VoxelInputSubsystem = Subsystem;
	Subsystem->AddMappingContext(VoxelContext, -1);
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

AVoxelWorldManager* AUnrealMinecraftCharacter::TraceVoxel(FIntVector& Voxel, FIntVector& Face, EVoxelType& Type) const
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
	return Manager->GetWorldVoxel(Voxel, Type) && Type != EVoxelType::Air ? Manager : nullptr;
}

void AUnrealMinecraftCharacter::ResetMining()
{
	MiningManager.Reset();
}

void AUnrealMinecraftCharacter::UpdateMining()
{
	FIntVector Voxel, Face;
	EVoxelType Type;
	AVoxelWorldManager* Manager = TraceVoxel(Voxel, Face, Type);
	if (!Manager)
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
	const float Duration = Type == EVoxelType::Stone ? StoneMiningSeconds
		: (Type == EVoxelType::Snow ? SnowMiningSeconds : GrassMiningSeconds);
	if (Now - MiningStartTime >= Duration)
	{
		Manager->SetWorldVoxel(Voxel, EVoxelType::Air);
		ResetMining();
	}
}

void AUnrealMinecraftCharacter::PlaceVoxel()
{
	FIntVector Voxel, Face;
	EVoxelType Type;
	AVoxelWorldManager* Manager = TraceVoxel(Voxel, Face, Type);
	if (!Manager)
	{
		return;
	}
	const FIntVector Target = Voxel + Face;
	if (!Manager->GetWorldVoxel(Target, Type) || Type != EVoxelType::Air)
	{
		return;
	}
	const FVector Center = (FVector(Target) + FVector(0.5)) * FVoxelMesher::VoxelSize;
	const FCollisionShape Box = FCollisionShape::MakeBox(FVector(FVoxelMesher::VoxelSize * 0.5));
	if (!GetCapsuleComponent()->OverlapComponent(Center, FQuat::Identity, Box))
	{
		Manager->SetWorldVoxel(Target, FVoxelWorldGenerator::GetSolidTypeAtHeight(Target.Z));
	}
}
