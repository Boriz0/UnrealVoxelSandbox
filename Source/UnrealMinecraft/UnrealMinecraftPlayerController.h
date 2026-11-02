// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UnrealMinecraftPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class USessionMenuWidget;
class AMenuCamera;
class AVoxelWorldManager;
class ACharacter;

UENUM(BlueprintType)
enum class EVoxelSessionState : uint8
{
	MainMenu, Loading, Playing, Paused
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoxelSessionStateChanged, EVoxelSessionState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoxelSessionMessage, FText, Message);

/**
 *  Simple first person Player Controller
 *  Manages the input mapping context.
 *  Overrides the Player Camera Manager class.
 */
UCLASS(abstract, config="Game")
class UNREALMINECRAFT_API AUnrealMinecraftPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AUnrealMinecraftPlayerController();

	UPROPERTY(BlueprintReadOnly, Category="Session")
	EVoxelSessionState SessionState = EVoxelSessionState::MainMenu;

	UPROPERTY(BlueprintAssignable, Category="Session")
	FVoxelSessionStateChanged OnSessionStateChanged;
	UPROPERTY(BlueprintAssignable, Category="Session")
	FVoxelSessionMessage OnLoadingStatusChanged;
	UPROPERTY(BlueprintAssignable, Category="Session")
	FVoxelSessionMessage OnSessionOperationFailed;
	UPROPERTY(BlueprintReadOnly, Category="Session")
	FText LoadingStatus;
	UPROPERTY(BlueprintReadOnly, Category="Session")
	FText LastSessionError;

	UFUNCTION(BlueprintPure, Category="Session")
	bool HasCompatibleSave() const;
	bool HasExistingSave() const;
	UFUNCTION(BlueprintPure, Category="Session")
	bool IsPersistenceAvailable() const;
	UFUNCTION(BlueprintCallable, Category="Session")
	bool ContinueSession();
	UFUNCTION(BlueprintCallable, Category="Session")
	bool StartNewGame(int32 Seed);
	UFUNCTION(BlueprintCallable, Category="Session")
	bool TogglePauseMenu();
	/** Save active gameplay before quitting unless the caller explicitly requests discard. */
	UFUNCTION(BlueprintCallable, Category="Session")
	bool ExitGame(bool bDiscardUnsavedSession = false);
	UFUNCTION(BlueprintPure, Category="Session")
	static bool TryParseWorldSeed(const FString& Text, int32& Seed);
	UFUNCTION(BlueprintCallable, Category="Session")
	static int32 GenerateWorldSeed();

protected:
	UPROPERTY(EditDefaultsOnly, Category="Session")
	TSubclassOf<USessionMenuWidget> SessionMenuClass;

	UPROPERTY(Transient)
	TObjectPtr<USessionMenuWidget> SessionMenu;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void SetPawn(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	void InitializeMenu();
	void ApplyState(EVoxelSessionState State);
	void ApplyPawnControls();
	void ReleasePawnControls();
	void HandleEscape();
	void PollLoading();
	void BeginLoading(bool bRestore);
	bool SaveCheckpoint();
	bool bRestoringCheckpoint = false;
	void SetLoadingStatus(const FText& Status);
	bool Fail(const FText& Reason);
	void ReturnToMenu(const FText& Reason);
	UPROPERTY(Transient)
	TObjectPtr<AMenuCamera> MenuCamera;
	TWeakObjectPtr<AVoxelWorldManager> WorldManager;
	TWeakObjectPtr<ACharacter> FrozenCharacter;
	uint8 PreviousMovementMode = 0;
	uint8 PreviousCustomMode = 0;
	bool bMenuInitialized = false;
	bool bOwnsCamera = false;
	FVector SpawnAnchor = FVector::ZeroVector;
	double GroundZ = 0;
	double LoadingDeadline = 0;
	FTimerHandle LoadingTimer;
};
