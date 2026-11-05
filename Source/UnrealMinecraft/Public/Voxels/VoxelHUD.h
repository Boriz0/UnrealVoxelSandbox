#pragma once

#include "GameFramework/HUD.h"
#include "VoxelHUD.generated.h"

class SVoxelFace;
class UWidgetComponent;

UCLASS()
class AVoxelHUD : public AHUD
{
	GENERATED_BODY()

public:
	AVoxelHUD();
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void DrawHUD() override;

private:
	void DrawWorldStatistics();
	void DrawControls(float HotbarLeft, float HotbarTop);
	UPROPERTY()
	TObjectPtr<UWidgetComponent> FaceWidget;
	TSharedPtr<SVoxelFace> FaceDrawing;
	bool bActive = false;
	bool bPlacementBlocked = false;
};
