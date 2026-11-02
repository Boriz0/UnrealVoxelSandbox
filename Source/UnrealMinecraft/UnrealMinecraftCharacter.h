// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "UnrealMinecraftCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class AVoxelWorldManager;
enum class EVoxelType : uint8;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class AUnrealMinecraftCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:

	UPROPERTY(EditDefaultsOnly, Category="Voxel Interaction", meta=(ClampMin="0", Units="cm"))
	float InteractionRange = 500.f;

	UPROPERTY(EditDefaultsOnly, Category="Voxel Interaction", meta=(ClampMin="0", Units="s"))
	float StoneMiningSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category="Voxel Interaction", meta=(ClampMin="0", Units="s"))
	float GrassMiningSeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category="Voxel Interaction", meta=(ClampMin="0", Units="s"))
	float SnowMiningSeconds = 0.3f;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	AUnrealMinecraftCharacter();

	/** Progress is negative when the aimed block is not being mined. */
	bool GetVoxelAimFeedback(FIntVector& Voxel, FIntVector& Face, float& Progress, bool& bCanPlace) const;
	void ResetMining();

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool bCollisionBeforeFlight = true;
	void ToggleFlight();
	void FlyUp();
	void FlyDown();
	void IncreaseRunningSpeed();
	void DecreaseRunningSpeed();
	void ScaleRunningSpeed(float Multiplier);

	void BindVoxelInput(UEnhancedInputComponent* Input);
	void RemoveVoxelInput();
	void UpdateMining();
	float GetMiningDuration(EVoxelType Type) const;
	bool CanPlaceVoxel(AVoxelWorldManager& Manager, const FIntVector& Target) const;
	void PlaceVoxel();
	AVoxelWorldManager* TraceVoxel(FIntVector& Voxel, FIntVector& Face, EVoxelType& Type) const;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> VoxelContext;
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> VoxelInputSubsystem;
	TWeakObjectPtr<UEnhancedInputComponent> BoundVoxelInput;
	TArray<uint32> VoxelBindingHandles;
	TWeakObjectPtr<AVoxelWorldManager> MiningManager;
	FIntVector MiningVoxel = FIntVector::ZeroValue;
	EVoxelType MiningType{};
	double MiningStartTime = 0;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

