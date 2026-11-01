#include "Menu/MenuCamera.h"

AMenuCamera::AMenuCamera()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMenuCamera::BeginPlay()
{
	Super::BeginPlay();
	AddActorWorldOffset(FVector(0, 0, CameraHeight));
	SetActorRotation(FRotator(CameraPitch, GetActorRotation().Yaw, 0));
}

void AMenuCamera::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SetActorRotation(FRotator(CameraPitch, GetActorRotation().Yaw + RotationSpeed * DeltaSeconds, 0));
}
