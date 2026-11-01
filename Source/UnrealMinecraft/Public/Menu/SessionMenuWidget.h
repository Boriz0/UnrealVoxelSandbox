#pragma once

#include "Blueprint/UserWidget.h"
#include "UnrealMinecraftPlayerController.h"
#include "SessionMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UWidgetSwitcher;

/** Presentation for the first-person controller's session API. Pages follow the designer switcher order. */
UCLASS(Abstract)
class UNREALMINECRAFT_API USessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Key) override;

private:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ContinueButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> NewButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ExitButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> BackButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> RandomButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> StartButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CancelButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UEditableTextBox> SeedBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ContinueLabel;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LoadingLabel;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Warning;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Status;

	AUnrealMinecraftPlayerController* Controller() const;
	UFUNCTION() void Refresh(EVoxelSessionState State);
	UFUNCTION() void ShowError(FText Message);
	UFUNCTION() void ShowLoading(FText Message);
	UFUNCTION() void ContinueClicked();
	UFUNCTION() void NewClicked();
	UFUNCTION() void ExitClicked();
	UFUNCTION() void SeedPage();
	UFUNCTION() void Cancel();
	UFUNCTION() void Randomize();
	UFUNCTION() void Start();
};
