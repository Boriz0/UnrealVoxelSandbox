#pragma once

#include "Camera/CameraActor.h"
#include "MenuCamera.generated.h"

UCLASS()
class UNREALMINECRAFT_API AMenuCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AMenuCamera();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu Camera", meta=(Units="cm", ClampMin="0"))
	float CameraHeight = 2500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu Camera", meta=(ClampMin="-89", ClampMax="0"))
	float CameraPitch = -20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Menu Camera", meta=(Units="deg/s"))
	float RotationSpeed = 6.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
