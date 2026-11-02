#include "UnrealMinecraftPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Menu/MenuCamera.h"
#include "Menu/SessionMenuWidget.h"
#include "PlatformFeatures.h"
#include "UnrealMinecraft.h"
#include "UnrealMinecraftCharacter.h"
#include "TimerManager.h"
#include "Voxels/VoxelChunkActor.h"
#include "Voxels/VoxelWorldGenerator.h"
#include "Voxels/VoxelWorldManager.h"
#include "Voxels/VoxelWorldSave.h"

#define LOCTEXT_NAMESPACE "VoxelMenu"

void AUnrealMinecraftPlayerController::InitializeMenu()
{
	for (TActorIterator<AVoxelWorldManager> It(GetWorld()); It; ++It)
	{
		WorldManager = *It;
		break;
	}
	AActor* Anchor = GetPawn();
	if (!Anchor)
	{
		if (AGameModeBase* Mode = GetWorld()->GetAuthGameMode())
		{
			Anchor = Mode->FindPlayerStart(this);
		}
	}
	SpawnAnchor = Anchor ? Anchor->GetActorLocation() : GetSpawnLocation();
	for (TActorIterator<AMenuCamera> It(GetWorld()); It; ++It)
	{
		MenuCamera = *It;
		break;
	}
	if (!MenuCamera)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FVector CameraAnchor = SpawnAnchor;
		if (WorldManager.IsValid())
		{
			const double Size = FVoxelMesher::VoxelSize;
			const int32 Height = FVoxelWorldGenerator(WorldManager->WorldSeed).GetSurfaceHeight(
				FMath::FloorToInt64(CameraAnchor.X / Size), FMath::FloorToInt64(CameraAnchor.Y / Size));
			CameraAnchor.Z = FMath::Max(CameraAnchor.Z, (Height + 1.) * Size);
		}
		MenuCamera = GetWorld()->SpawnActor<AMenuCamera>(CameraAnchor, GetControlRotation(), Params);
		bOwnsCamera = IsValid(MenuCamera);
	}
	bMenuInitialized = true;
	if (WorldManager.IsValid())
	{
		WorldManager->SetMenuPreview(MenuCamera);
	}
	ApplyState(EVoxelSessionState::MainMenu);
	if (SessionMenuClass && !SessionMenu)
	{
		SessionMenu = CreateWidget<USessionMenuWidget>(this, SessionMenuClass);
		if (SessionMenu) SessionMenu->AddToPlayerScreen(10);
	}
	if (!WorldManager.IsValid() || !IsValid(MenuCamera))
	{
		Fail(LOCTEXT("MissingPreview", "The menu preview requires a voxel world manager and a menu camera."));
	}
}

void AUnrealMinecraftPlayerController::ReleasePawnControls()
{
	if (ACharacter* PlayerCharacter = FrozenCharacter.Get())
	{
		PlayerCharacter->GetCharacterMovement()->SetMovementMode(static_cast<EMovementMode>(PreviousMovementMode), PreviousCustomMode);
	}
	FrozenCharacter.Reset();
}

void AUnrealMinecraftPlayerController::ApplyPawnControls()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}
	if (SessionState == EVoxelSessionState::Playing)
	{
		ReleasePawnControls();
		ControlledPawn->EnableInput(this);
		return;
	}
	ControlledPawn->DisableInput(this);
	if (AUnrealMinecraftCharacter* VoxelCharacter = Cast<AUnrealMinecraftCharacter>(ControlledPawn))
	{
		VoxelCharacter->ResetMining();
	}
	// Paused already freezes the world. Preserve the player's exact movement on resume.
	if (SessionState != EVoxelSessionState::Paused && !FrozenCharacter.IsValid())
	{
		if (ACharacter* PlayerCharacter = Cast<ACharacter>(ControlledPawn))
		{
			auto* Movement = PlayerCharacter->GetCharacterMovement();
			FrozenCharacter = PlayerCharacter;
			PreviousMovementMode = Movement->MovementMode;
			PreviousCustomMode = Movement->CustomMovementMode;
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
}

void AUnrealMinecraftPlayerController::SetPawn(APawn* InPawn)
{
	if (bMenuInitialized && InPawn != GetPawn())
	{
		ReleasePawnControls();
	}
	Super::SetPawn(InPawn);
	if (bMenuInitialized)
	{
		ApplyPawnControls();
	}
}

void AUnrealMinecraftPlayerController::ApplyState(EVoxelSessionState State)
{
	SessionState = State;
	const bool bPlaying = State == EVoxelSessionState::Playing;
	const bool bShowPreview = State == EVoxelSessionState::MainMenu || State == EVoxelSessionState::Loading;
	bAutoManageActiveCameraTarget = !bShowPreview;
	if (bShowPreview && IsValid(MenuCamera))
	{
		SetViewTarget(MenuCamera);
	}
	else if (GetPawn())
	{
		SetViewTarget(GetPawn());
	}
	ApplyPawnControls();
	FlushPressedKeys();
	bShowMouseCursor = !bPlaying;
	if (bPlaying)
	{
		SetInputMode(FInputModeGameOnly());
	}
	else
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
	}
	if (MobileControlsWidget)
	{
		MobileControlsWidget->SetVisibility(bPlaying ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	OnSessionStateChanged.Broadcast(State);
}

bool AUnrealMinecraftPlayerController::HasCompatibleSave() const { return UVoxelWorldSave::LoadLatest() != nullptr; }
bool AUnrealMinecraftPlayerController::HasExistingSave() const { return UVoxelWorldSave::Exists(); }
bool AUnrealMinecraftPlayerController::IsPersistenceAvailable() const { return IPlatformFeaturesModule::Get().GetSaveGameSystem() != nullptr; }

bool AUnrealMinecraftPlayerController::SaveCheckpoint()
{
	if (!WorldManager.IsValid() || !GetPawn())
	{
		return Fail(LOCTEXT("MissingSaveSource", "Cannot save without the active world and player."));
	}
	auto* Save = NewObject<UVoxelWorldSave>();
	Save->Seed = WorldManager->WorldSeed;
	Save->PlayerTransform = GetPawn()->GetActorTransform();
	if (const auto* PlayerCharacter = Cast<AUnrealMinecraftCharacter>(GetPawn()))
	{
		Save->bNoclip = PlayerCharacter->GetCharacterMovement()->IsFlying();
	}
	Save->ViewRotation = GetControlRotation();
	WorldManager->ExportEdits(Save->Edits);
	return Save->Commit();
}

bool AUnrealMinecraftPlayerController::Fail(const FText& Reason)
{
	LastSessionError = Reason;
	UE_LOG(LogUnrealMinecraft, Warning, TEXT("Session operation failed: %s"), *Reason.ToString());
	OnSessionOperationFailed.Broadcast(Reason);
	return false;
}

void AUnrealMinecraftPlayerController::SetLoadingStatus(const FText& Status)
{
	if (!LoadingStatus.EqualTo(Status))
	{
		LoadingStatus = Status;
		OnLoadingStatusChanged.Broadcast(Status);
	}
}

bool AUnrealMinecraftPlayerController::StartNewGame(int32 Seed)
{
	const bool bReplacingPausedGame = SessionState == EVoxelSessionState::Paused;
	if (!bMenuInitialized || (SessionState != EVoxelSessionState::MainMenu && !bReplacingPausedGame)
		|| !WorldManager.IsValid() || !GetPawn())
	{
		return Fail(LOCTEXT("CannotStartFromMenu", "Starting a new game requires the main or pause menu, a player pawn, and a voxel world manager."));
	}
	AVoxelWorldManager* Manager = WorldManager.Get();
	if (!Manager->IsActorTickEnabled() || Manager->MinVerticalLayer > Manager->MaxVerticalLayer)
	{
		return Fail(LOCTEXT("InvalidWorld", "The voxel manager cannot stream with its current settings."));
	}
	// Use the existing GameMode's spawn point. A different seed may place solid terrain above it.
	SpawnAnchor = GetPawn()->GetActorLocation();
	if (AGameModeBase* Mode = GetWorld()->GetAuthGameMode())
	{
		if (AActor* Start = Mode->FindPlayerStart(this))
		{
			SpawnAnchor = Start->GetActorLocation();
		}
	}
	constexpr double VoxelSize = FVoxelMesher::VoxelSize;
	const int64 X = FMath::FloorToInt64(SpawnAnchor.X / VoxelSize);
	const int64 Y = FMath::FloorToInt64(SpawnAnchor.Y / VoxelSize);
	const int64 MinZ = static_cast<int64>(Manager->MinVerticalLayer) * FVoxelChunkData::ChunkSize;
	const int64 MaxZ = (static_cast<int64>(Manager->MaxVerticalLayer) + 1) * FVoxelChunkData::ChunkSize - 1;
	const int64 Surface = FMath::Min<int64>(FVoxelWorldGenerator(Seed).GetSurfaceHeight(X, Y), MaxZ);
	if (Surface < MinZ)
	{
		return Fail(LOCTEXT("NoGround", "There is no terrain under Player Start within the configured chunk layers."));
	}
	// The seed dialog calls this only on Start. Keep the current session paused through validation.
	if (bReplacingPausedGame && !SetPause(false))
	{
		return Fail(LOCTEXT("CannotRestartPaused", "Could not leave pause to start the new world."));
	}
	GroundZ = (Surface + 1) * VoxelSize;
	Manager->StartWorld(Seed, {});
	// Loading streams from the pawn at the spawn column, while the view remains on the camera.
	const float HalfHeight = GetPawn()->GetSimpleCollisionHalfHeight();
	FVector Location = SpawnAnchor;
	Location.Z = FMath::Max(Location.Z, GroundZ + HalfHeight + 5.);
	GetPawn()->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	BeginLoading(false);
	if (PreviousMovementMode == MOVE_Flying)
	{
		PreviousMovementMode = MOVE_Falling;
	}
	return true;
}

void AUnrealMinecraftPlayerController::BeginLoading(bool bRestore)
{
	bRestoringCheckpoint = bRestore;
	LastSessionError = FText::GetEmpty();
	LoadingDeadline = FPlatformTime::Seconds() + 60.0;
	GetWorldTimerManager().SetTimer(LoadingTimer, this, &ThisClass::PollLoading, 0.1f, true);
	SetLoadingStatus(LOCTEXT("Generating", "Generating spawn terrain"));
	ApplyState(EVoxelSessionState::Loading);
}

void AUnrealMinecraftPlayerController::PollLoading()
{
	if (!WorldManager.IsValid() || !GetPawn() || FPlatformTime::Seconds() >= LoadingDeadline)
	{
		ReturnToMenu(LOCTEXT("LoadFailed", "Spawn terrain or collision did not become ready. Check Player Start and the voxel manager settings."));
		return;
	}
	// Only the spawn column must be ready. The configured grid can keep streaming during play.
	SetLoadingStatus(LOCTEXT("Collision", "Preparing spawn collision"));
	// Async cooking can finish after the last mesh upload. Trace only the terrain at this column.
	const bool bReady = bRestoringCheckpoint
		? WorldManager->IsRestoreAreaReady(GetPawn()->GetActorLocation(), GetPawn()->GetSimpleCollisionRadius())
		: WorldManager->HasSurfaceCollision(FVector(SpawnAnchor.X, SpawnAnchor.Y, GroundZ));
	if (!bReady)
	{
		return;
	}
	const bool bCanRecoverLegacyFlight = bRestoringCheckpoint && bLegacyCheckpoint && Cast<AUnrealMinecraftCharacter>(FrozenCharacter.Get());
	if (bCanRecoverLegacyFlight)
	{
		// A pawn deep inside solid terrain may not overlap any collision triangles.
		const FVector Cell = GetPawn()->GetActorLocation() / FVoxelMesher::VoxelSize;
		EVoxelType Type;
		if (WorldManager->GetWorldVoxel(FIntVector(FMath::FloorToInt(Cell.X), FMath::FloorToInt(Cell.Y), FMath::FloorToInt(Cell.Z)), Type)
			&& Type != EVoxelType::Air)
		{
			PreviousMovementMode = MOVE_Flying;
		}
	}
	const bool bNoclipRestore = bRestoringCheckpoint && PreviousMovementMode == MOVE_Flying && FrozenCharacter.IsValid();
	if (!bNoclipRestore && !GetPawn()->TeleportTo(GetPawn()->GetActorLocation(), GetPawn()->GetActorRotation()))
	{
		// Older checkpoints did not record flight. Let a buried player fly out.
		if (bCanRecoverLegacyFlight)
		{
			PreviousMovementMode = MOVE_Flying;
		}
		else
		{
			ReturnToMenu(LOCTEXT("SpawnBlocked", "Player Start is obstructed. Choose a clear spawn point above the terrain."));
			return;
		}
	}
	GetWorldTimerManager().ClearTimer(LoadingTimer);
	if (!bRestoringCheckpoint && !SaveCheckpoint())
	{
		ReturnToMenu(LOCTEXT("InitialSaveFailed", "Could not save the new world. Check the Output Log and available disk space."));
		return;
	}
	SetLoadingStatus(FText::GetEmpty());
	ApplyState(EVoxelSessionState::Playing);
}

void AUnrealMinecraftPlayerController::ReturnToMenu(const FText& Reason)
{
	GetWorldTimerManager().ClearTimer(LoadingTimer);
	if (WorldManager.IsValid())
	{
		WorldManager->SetMenuPreview(MenuCamera);
	}
	SetLoadingStatus(FText::GetEmpty());
	ApplyState(EVoxelSessionState::MainMenu);
	Fail(Reason);
}

bool AUnrealMinecraftPlayerController::ContinueSession()
{
	if (SessionState == EVoxelSessionState::MainMenu)
	{
		const auto* Save = UVoxelWorldSave::LoadLatest();
		if (!Save || !WorldManager.IsValid() || !WorldManager->IsActorTickEnabled() || !GetPawn())
		{
			return Fail(LOCTEXT("NoSave", "Cannot load the world. Check the save, player, and world manager in the Output Log."));
		}
		WorldManager->StartWorld(Save->Seed, Save->Edits);
		GetPawn()->SetActorTransform(Save->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
		SetControlRotation(Save->ViewRotation);
		BeginLoading(true);
		bLegacyCheckpoint = Save->FormatVersion == 1;
		if (Cast<AUnrealMinecraftCharacter>(FrozenCharacter.Get()))
		{
			PreviousMovementMode = Save->bNoclip ? MOVE_Flying : MOVE_Falling;
		}
		return true;
	}
	if (SessionState != EVoxelSessionState::Paused || !SetPause(false))
	{
		return Fail(LOCTEXT("CannotResume", "The game could not resume."));
	}
	LastSessionError = FText::GetEmpty();
	ApplyState(EVoxelSessionState::Playing);
	return true;
}

bool AUnrealMinecraftPlayerController::TogglePauseMenu()
{
	if (SessionState == EVoxelSessionState::Paused)
	{
		return ContinueSession();
	}
	if (SessionState != EVoxelSessionState::Playing || !SetPause(true))
	{
		return Fail(LOCTEXT("CannotPause", "The game cannot pause in its current state."));
	}
	ApplyState(EVoxelSessionState::Paused);
	return true;
}

void AUnrealMinecraftPlayerController::HandleEscape()
{
	if (SessionState == EVoxelSessionState::Paused || SessionState == EVoxelSessionState::Playing)
	{
		TogglePauseMenu();
	}
}

bool AUnrealMinecraftPlayerController::ExitGame(bool bDiscardUnsavedSession)
{
	if (SessionState == EVoxelSessionState::Loading)
	{
		return Fail(LOCTEXT("Busy", "Wait for world initialization before exiting."));
	}
	if (SessionState != EVoxelSessionState::MainMenu && !bDiscardUnsavedSession && !SaveCheckpoint())
	{
		return Fail(LOCTEXT("Unsaved", "Could not save your progress. Your session is still open. Check the Output Log and try again."));
	}
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
	return true;
}

bool AUnrealMinecraftPlayerController::TryParseWorldSeed(const FString& Text, int32& Seed)
{
	Seed = 0;
	const FString Value = Text.TrimStartAndEnd();
	const bool Negative = Value.StartsWith(TEXT("-"));
	int32 I = Negative || Value.StartsWith(TEXT("+")) ? 1 : 0;
	if (I >= Value.Len())
	{
		return false;
	}
	const int64 Limit = Negative ? -static_cast<int64>(MIN_int32) : MAX_int32;
	int64 Number = 0;
	for (; I < Value.Len(); ++I)
	{
		if (Value[I] < '0' || Value[I] > '9')
		{
			return false;
		}
		Number = Number * 10 + (Value[I] - '0');
		if (Number > Limit)
		{
			return false;
		}
	}
	Seed = static_cast<int32>(Negative ? -Number : Number);
	return true;
}

int32 AUnrealMinecraftPlayerController::GenerateWorldSeed()
{
	return static_cast<int32>(FGuid::NewGuid().A);
}

void AUnrealMinecraftPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (SessionMenu) SessionMenu->RemoveFromParent();
	GetWorldTimerManager().ClearTimer(LoadingTimer);
	ReleasePawnControls();
	if (bOwnsCamera && IsValid(MenuCamera))
	{
		MenuCamera->Destroy();
	}
	Super::EndPlay(Reason);
}

#undef LOCTEXT_NAMESPACE
