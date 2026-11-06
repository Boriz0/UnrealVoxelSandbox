#include "Menu/SessionMenuWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "VoxelBuildMetadata.h"

#define LOCTEXT_NAMESPACE "VoxelSessionMenu"
namespace { enum EPage { Home, Confirm, Seed, Loading }; }

AUnrealMinecraftPlayerController* USessionMenuWidget::Controller() const
{
	return GetOwningPlayer<AUnrealMinecraftPlayerController>();
}

void USessionMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (RepositoryCredit)
	{
		RepositoryCredit->SetText(FText::Format(
			LOCTEXT("CreditRevisionAndBuildNumber", "Boris Vacula  ·  GitHub ↗\nRevision {0} · {1}\nBuild {2}\nBuilt {3} UTC"),
			FText::FromString(ANSI_TO_TCHAR(VOXEL_REVISION_COUNT)),
			FText::FromString(FString(ANSI_TO_TCHAR(VOXEL_BUILD_REVISION)).Left(12)),
			FText::FromString(ANSI_TO_TCHAR(VOXEL_BUILD_NUMBER)),
			FText::FromString(ANSI_TO_TCHAR(VOXEL_BUILD_DATE))));
		RepositoryCredit->SetToolTipText(FText::Format(
			LOCTEXT("BuildRevisionDetails", "Git commit: {0}\nUncommitted changes are not identified by this revision."),
			FText::FromString(ANSI_TO_TCHAR(VOXEL_BUILD_REVISION))));
		RepositoryCredit->SetJustification(ETextJustify::Center);
		RepositoryCredit->SetAutoWrapText(true);
	}
	if (Title)
	{
		Title->SetText(LOCTEXT("ProjectTitle", "UNREAL VOXEL SANDBOX"));
		Title->SetAutoWrapText(true);
		Title->SetJustification(ETextJustify::Center);
	}
	if (SeedBox)
	{
		SeedBox->SetForegroundColor(FLinearColor(.015f, .015f, .015f, 1.f));
	}
}

void USessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	ContinueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ContinueClicked);
	NewButton->OnClicked.AddUniqueDynamic(this, &ThisClass::NewClicked);
	ExitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ExitClicked);
	RepositoryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OpenRepository);
	ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::SeedPage);
	BackButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Cancel);
	CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Cancel);
	StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Start);
	RandomButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Randomize);
	if (auto* PC = Controller())
	{
		PC->OnSessionStateChanged.AddUniqueDynamic(this, &ThisClass::Refresh);
		PC->OnSessionOperationFailed.AddUniqueDynamic(this, &ThisClass::ShowError);
		PC->OnLoadingStatusChanged.AddUniqueDynamic(this, &ThisClass::ShowLoading);
		Refresh(PC->SessionState);
		ShowError(PC->LastSessionError);
	}
}

void USessionMenuWidget::NativeDestruct()
{
	if (auto* PC = Controller())
	{
		PC->OnSessionStateChanged.RemoveAll(this);
		PC->OnSessionOperationFailed.RemoveAll(this);
		PC->OnLoadingStatusChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void USessionMenuWidget::Refresh(EVoxelSessionState State)
{
	SetVisibility(State == EVoxelSessionState::Playing ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (State == EVoxelSessionState::Playing) return;
	const bool Paused = State == EVoxelSessionState::Paused;
	Pages->SetActiveWidgetIndex(State == EVoxelSessionState::Loading ? Loading : Home);
	ContinueLabel->SetText(Paused ? LOCTEXT("Resume","Resume") : LOCTEXT("Continue","Continue"));
	ContinueButton->SetIsEnabled(Paused || Controller()->HasCompatibleSave());
	ShowError(FText::GetEmpty());
	ShowLoading(Controller()->LoadingStatus);
	SetUserFocus(Controller());
}

void USessionMenuWidget::ShowError(FText Message)
{
	Status->SetText(Message);
	Status->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
void USessionMenuWidget::ShowLoading(FText Message) { LoadingLabel->SetText(Message); }
void USessionMenuWidget::ContinueClicked() { Controller()->ContinueSession(); }
void USessionMenuWidget::ExitClicked() { Controller()->ExitGame(false); }

void USessionMenuWidget::OpenRepository()
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/Boriz0/UnrealMinecraft"), nullptr, nullptr);
}

void USessionMenuWidget::NewClicked()
{
	ShowError(FText::GetEmpty());
	const bool Saved = Controller()->HasExistingSave();
	if (Saved || Controller()->SessionState == EVoxelSessionState::Paused)
	{
		Warning->SetText(Saved ? LOCTEXT("Overwrite","Starting a new world will replace your current save. Continue?")
			: LOCTEXT("Replace","Starting a new world will discard this session's changes. Continue?"));
		Pages->SetActiveWidgetIndex(Confirm);
		BackButton->SetUserFocus(Controller());
	}
	else SeedPage();
}

void USessionMenuWidget::SeedPage()
{
	SeedBox->SetText(FText::GetEmpty());
	Pages->SetActiveWidgetIndex(Seed);
	SeedBox->SetUserFocus(Controller());
}
void USessionMenuWidget::Randomize()
{
	SeedBox->SetText(FText::FromString(FString::FromInt(Controller()->GenerateWorldSeed())));
	ShowError(FText::GetEmpty());
}
void USessionMenuWidget::Start()
{
	const FString Value = SeedBox->GetText().ToString().TrimStartAndEnd();
	int32 Number;
	if (Value.IsEmpty()) Number = Controller()->GenerateWorldSeed();
	else if (!Controller()->TryParseWorldSeed(Value, Number))
	{
		ShowError(LOCTEXT("Invalid","Enter an integer from -2147483648 to 2147483647, or leave the seed empty."));
		SeedBox->SetUserFocus(Controller());
		return;
	}
	Controller()->StartNewGame(Number);
}
void USessionMenuWidget::Cancel() { Refresh(Controller()->SessionState); }

FReply USessionMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Key)
{
	if (Key.GetKey() == EKeys::Escape)
	{
		if (!Key.IsRepeat())
		{
			if (Pages->GetActiveWidgetIndex() == Seed || Pages->GetActiveWidgetIndex() == Confirm) Cancel();
			else if (Controller()->SessionState == EVoxelSessionState::Paused) ContinueClicked();
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Key);
}
#undef LOCTEXT_NAMESPACE
