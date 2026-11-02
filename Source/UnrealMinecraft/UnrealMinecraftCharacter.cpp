// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealMinecraftCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UnrealMinecraft.h"
#include "UnrealMinecraftPlayerController.h"

AUnrealMinecraftCharacter::AUnrealMinecraftCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void AUnrealMinecraftCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		BindVoxelInput(EnhancedInputComponent);
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AUnrealMinecraftCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::FlyUp);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AUnrealMinecraftCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUnrealMinecraftCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUnrealMinecraftCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AUnrealMinecraftCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AUnrealMinecraftCharacter::IncreaseRunningSpeed()
{
	ScaleRunningSpeed(1.2f);
}

void AUnrealMinecraftCharacter::DecreaseRunningSpeed()
{
	ScaleRunningSpeed(0.8f);
}

void AUnrealMinecraftCharacter::ScaleRunningSpeed(float Multiplier)
{
	const auto* PlayerController = Cast<AUnrealMinecraftPlayerController>(GetController());
	if (!PlayerController || PlayerController->SessionState != EVoxelSessionState::Playing)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float NewSpeed = Movement->MaxWalkSpeed * Multiplier;
	const float NewAcceleration = Movement->MaxAcceleration * Multiplier;
	if (FMath::IsFinite(NewSpeed) && NewSpeed > 0.f
		&& FMath::IsFinite(NewAcceleration) && NewAcceleration >= 0.f)
	{
		Movement->MaxWalkSpeed = NewSpeed;
		Movement->MaxFlySpeed = NewSpeed;
		Movement->MaxAcceleration = NewAcceleration;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 3.f, FColor::Yellow,
				FString::Printf(TEXT("Max speed: %.1f km/h | Acceleration: %.2f g"),
					Movement->MaxWalkSpeed * 0.036f, Movement->MaxAcceleration / 980.665f));
		}
	}
}

void AUnrealMinecraftCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AUnrealMinecraftCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AUnrealMinecraftCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AUnrealMinecraftCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		const FVector ForwardDirection = GetCharacterMovement()->IsFlying()
			? GetControlRotation().Vector() : GetActorForwardVector();
		AddMovementInput(ForwardDirection, Forward);
	}
}

void AUnrealMinecraftCharacter::DoJumpStart()
{
	// pass Jump to the character
	if (!GetCharacterMovement()->IsFlying())
	{
		Jump();
	}
}

void AUnrealMinecraftCharacter::ToggleFlight()
{
	const auto* PC = Cast<AUnrealMinecraftPlayerController>(GetController());
	if (PC && PC->SessionState == EVoxelSessionState::Playing)
	{
		auto* Movement = GetCharacterMovement();
		StopJumping();
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(Movement->IsFlying() ? MOVE_Falling : MOVE_Flying);
		if (GEngine)
		{
			static const uint64 MessageKey = GetTypeHash(FName(TEXT("VoxelFlightStatus")));
			GEngine->AddOnScreenDebugMessage(MessageKey, 3.f, FColor::Yellow,
				Movement->IsFlying() ? TEXT("Flight ON (noclip)") : TEXT("Flight OFF"));
		}
	}
}

void AUnrealMinecraftCharacter::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	auto* Movement = GetCharacterMovement();
	if (Movement->IsFlying() && PreviousMovementMode != MOVE_Flying)
	{
		Movement->MaxFlySpeed = Movement->MaxWalkSpeed;
		// PhysFlying clears velocity on release when this flag is set.
		Movement->bCheatFlying = true;
		bCollisionBeforeFlight = GetActorEnableCollision();
		SetActorEnableCollision(false);
	}
	else if (PreviousMovementMode == MOVE_Flying && !Movement->IsFlying())
	{
		Movement->bCheatFlying = false;
		SetActorEnableCollision(bCollisionBeforeFlight);
	}
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void AUnrealMinecraftCharacter::FlyUp()
{
	if (GetCharacterMovement()->IsFlying())
	{
		AddMovementInput(FVector::UpVector);
	}
}

void AUnrealMinecraftCharacter::FlyDown()
{
	if (GetCharacterMovement()->IsFlying())
	{
		AddMovementInput(-FVector::UpVector);
	}
}

void AUnrealMinecraftCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
