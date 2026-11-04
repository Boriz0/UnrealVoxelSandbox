#include "Voxels/VoxelHUD.h"

#include "Components/WidgetComponent.h"
#include "DebugViewModeHelpers.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "ShaderCore.h"
#include "Rendering/DrawElements.h"
#include "SceneInterface.h"
#include "UnrealMinecraftCharacter.h"
#include "UnrealMinecraftPlayerController.h"
#include "Voxels/VoxelMesher.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	constexpr float FacePixels = 128.f;

	// Each branch starts on an earlier branch and grows outwards in face-local pixels.
	struct FCrackBranch
	{
		float StartProgress;
		FVector2D Points[4];
	};

	const FCrackBranch Branches[] = {
		{0.f,  {{63,65}, {55,56}, {58,48}, {17,13}}},
		{0.f,  {{63,65}, {73,71}, {79,68}, {117,92}}},
		{.18f, {{55,56}, {43,58}, {37,52}, {9,57}}},
		{.32f, {{73,71}, {67,84}, {71,96}, {56,122}}},
		{.48f, {{58,48}, {74,39}, {71,31}, {89,6}}},
		{.62f, {{79,68}, {91,55}, {97,56}, {120,34}}}
	};
}

class SVoxelFace : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVoxelFace) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); }

	bool SetProgress(float Progress)
	{
		const int32 NewStage = Progress < 0.f ? -1 : FMath::RoundToInt(Progress * 32);
		if (NewStage == Stage)
		{
			return false;
		}
		Stage = NewStage;
		Invalidate(EInvalidateWidgetReason::Paint);
		return true;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(FacePixels); }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
	{
		const FPaintGeometry Paint = Geometry.ToPaintGeometry();
		const TArray<FVector2D> Border = {{1,1}, {127,1}, {127,127}, {1,127}, {1,1}};
		FSlateDrawElement::MakeLines(Elements, Layer, Paint, Border, ESlateDrawEffect::None,
			FLinearColor(1.f, 1.f, 1.f, .47f), true, 1.4f);
		if (Stage >= 0)
		{
			const float Progress = .08f + .92f * Stage / 32.f;
			for (const FCrackBranch& Branch : Branches)
			{
				const float Length = 3.f * (Progress - Branch.StartProgress) / (1.f - Branch.StartProgress);
				if (Length <= 0.f)
				{
					continue;
				}
				TArray<FVector2D> Points = { Branch.Points[0] };
				for (int32 I = 0; I < 3 && I < Length; ++I)
				{
					Points.Add(FMath::Lerp(Branch.Points[I], Branch.Points[I+1], FMath::Min(Length - I, 1.f)));
				}
				FSlateDrawElement::MakeLines(Elements, Layer + 1, Paint, Points, ESlateDrawEffect::None,
					FLinearColor(.018f, .012f, .01f, .92f), true, 2.2f);
			}
		}
		return Layer + 1;
	}

private:
	int32 Stage = -1;
};

AVoxelHUD::AVoxelHUD()
{
	SetHidden(false);
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	FaceWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("VoxelFace"));
	SetRootComponent(FaceWidget);
	FaceWidget->SetDrawSize(FVector2D(FacePixels));
	FaceWidget->SetWorldScale3D(FVector(FVoxelMesher::VoxelSize / FacePixels));
	FaceWidget->SetBlendMode(EWidgetBlendMode::Transparent);
	FaceWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FaceWidget->SetGenerateOverlapEvents(false);
	FaceWidget->SetCastShadow(false);
	FaceWidget->SetWindowFocusable(false);
	FaceWidget->SetManuallyRedraw(true);
	FaceWidget->SetTickWhenOffscreen(true);
	FaceWidget->SetVisibility(false);
}

void AVoxelHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Paint the updated crack stage in the same frame that the HUD moves the face.
	FaceWidget->AddTickPrerequisiteActor(this);
}

void AVoxelHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const auto* PC = Cast<AUnrealMinecraftPlayerController>(PlayerOwner);
	const auto* Character = PC ? Cast<AUnrealMinecraftCharacter>(PC->GetPawn()) : nullptr;
	bActive = bShowHUD && Character && PC->SessionState == EVoxelSessionState::Playing;
	FIntVector Voxel, Face;
	float Progress;
	bool bCanPlace;
	if (!bActive || !Character->GetVoxelAimFeedback(Voxel, Face, Progress, bCanPlace))
	{
		FaceWidget->SetVisibility(false);
		bPlacementBlocked = false;
		return;
	}
	bPlacementBlocked = !bCanPlace;
	const FVector Normal(Face);
	const FVector Center = (FVector(Voxel) + FVector(.5)) * FVoxelMesher::VoxelSize;
	FaceWidget->SetWorldLocationAndRotation(Center + Normal * (FVoxelMesher::VoxelSize * .5 + .15), Normal.Rotation());
	if (!FaceDrawing)
	{
		FaceWidget->SetSlateWidget(SAssignNew(FaceDrawing, SVoxelFace));
	}
	const bool bChanged = FaceDrawing->SetProgress(Progress);
	if (bChanged || !FaceWidget->IsVisible())
	{
		FaceWidget->SetVisibility(true);
		FaceWidget->RequestRenderUpdate();
	}
}

void AVoxelHUD::DrawHUD()
{
	Super::DrawHUD();
	if (bActive && Canvas)
	{
		const float X = Canvas->ClipX * .5f;
		const float Y = Canvas->ClipY * .5f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, .7f), X - 2.5f, Y - 2.5f, 5.f, 5.f);
		DrawRect(bPlacementBlocked ? FLinearColor(1.f, .55f, .1f) : FLinearColor(.93f, .93f, .93f),
			X - 1.5f, Y - 1.5f, 3.f, 3.f);

		const auto* Character = Cast<AUnrealMinecraftCharacter>(GetOwningPawn());
		if (!Character)
		{
			return;
		}
		const FVoxelInventory& Inventory = Character->Inventory;
		static const TCHAR* Names[] = {TEXT("Grass"), TEXT("Stone"), TEXT("Snow")};
		const float Scale = FMath::Min3(Canvas->ClipX / 370.f, Canvas->ClipY / 160.f, 1.5f);
		const float Width = 108.f * Scale;
		const float Stride = Width + 4.f * Scale;
		const float Left = X - (Stride * (Inventory.SlotCount - 1) + Width) * .5f;
		const float Top = Canvas->ClipY - 58.f * Scale;
		DrawControls(Left, Top);
		for (int32 Slot = 0; Slot < Inventory.SlotCount; ++Slot)
		{
			const float SlotX = Left + Slot * Stride;
			const bool bSelected = Slot == Inventory.SelectedSlot;
			DrawRect(bSelected ? FLinearColor(1.f, .72f, .2f) : FLinearColor(.25f, .25f, .25f),
				SlotX, Top, Width, 44.f * Scale);
			DrawRect(FLinearColor(.035f, .035f, .035f), SlotX + 2.f * Scale, Top + 2.f * Scale,
				Width - 4.f * Scale, 40.f * Scale);
			DrawText(FString::Printf(TEXT("%d  %s"), Slot + 1, Names[Slot]), FLinearColor::White,
				SlotX + 7.f * Scale, Top + 4.f * Scale, GEngine->GetSmallFont(), Scale);
			DrawText(FString::Printf(TEXT("x%d"), Inventory.Counts[Slot]),
				Inventory.Counts[Slot] ? FLinearColor::White : FLinearColor::Gray,
				SlotX + 7.f * Scale, Top + 23.f * Scale, GEngine->GetSmallFont(), Scale);
		}
	}
}

void AVoxelHUD::DrawControls(float HotbarLeft, float HotbarTop)
{
	TArray<const TCHAR*, TInlineAllocator<16>> Lines = {
		TEXT("CONTROLS"),
		TEXT("WASD / arrows   Move     |     Mouse   Look"),
		TEXT("Space   Jump / fly up     |     Ctrl   Fly down"),
		TEXT("Left Shift   Toggle noclip flight"),
		TEXT("Wheel up / down   Speed & acceleration +20% / -20%"),
		TEXT("Hold left click   Mine     |     Right click   Place"),
		TEXT("1   Grass     |     2   Stone     |     3   Snow"),
		TEXT("L   Toggle Lumen     |     Esc   Pause / resume"),
		TEXT("Gamepad   Left stick: move    Right stick: look"),
		TEXT("Gamepad   Bottom face button: jump / fly up")
	};
#if UE_BUILD_DEVELOPMENT
	if (PlayerOwner->CheatManager)
	{
		const FSceneInterface* Scene = GetWorld()->Scene;
		const bool bComplexity = Scene && AllowDebugViewShaderMode(DVSM_ShaderComplexity, Scene->GetShaderPlatform(), Scene->GetFeatureLevel());
		if (AllowDebugViewmodes())
		{
			Lines.Add(TEXT("F1   Wireframe     F2   Unlit     F3   Lit"));
			Lines.Add(bComplexity ? TEXT("F4   Detail lighting     F5   Shader complexity") : TEXT("F4   Detail lighting"));
		}
		else if (bComplexity)
		{
			Lines.Add(TEXT("F3   Lit     F5   Shader complexity"));
		}
		Lines.Add(TEXT("Semicolon   Toggle debug camera"));
	}
	Lines.Add(TEXT("F9   Screenshot     |     ~   Console"));
#endif
	Lines.Add(IsRunningGame() ? TEXT("F11 / Alt+Enter   Fullscreen") : TEXT("Alt+Enter   Fullscreen"));

	UFont* Font = GEngine->GetSmallFont();
	float MaxWidth = 0.f;
	float LineHeight = 0.f;
	for (const TCHAR* Line : Lines)
	{
		float W, H;
		GetTextSize(Line, W, H, Font);
		MaxWidth = FMath::Max(MaxWidth, W);
		LineHeight = FMath::Max(LineHeight, H);
	}
	LineHeight += 2.f;
	const float Scale = FMath::Min3(FMath::Clamp(Canvas->ClipY / 850.f, 0.9f, 3.f),
		Canvas->ClipX / (MaxWidth + 32.f), Canvas->ClipY * .65f / (Lines.Num() * LineHeight + 32.f));
	const float Padding = 8.f * Scale;
	const float Width = MaxWidth * Scale + 2.f * Padding;
	const float Height = Lines.Num() * LineHeight * Scale + 2.f * Padding;
	const float Bottom = Padding + Width + Padding > HotbarLeft ? HotbarTop : Canvas->ClipY;
	const float Top = Bottom - Height - Padding;
	DrawRect(FLinearColor(0.015f, 0.022f, 0.018f, .78f), Padding, Top, Width, Height);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		DrawText(Lines[Index], Index == 0 ? FLinearColor(.65f, 1.f, .7f) : FLinearColor::White,
			2.f * Padding, Top + Padding + Index * LineHeight * Scale, Font, Scale);
	}
}
