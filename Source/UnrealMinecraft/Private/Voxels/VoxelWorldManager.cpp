#include "Voxels/VoxelWorldManager.h"

#include "Engine/World.h"
#include "UnrealMinecraft.h"
#include "Voxels/VoxelChunkActor.h"
#include "Voxels/VoxelChunkData.h"
#include "Voxels/VoxelMesher.h"

AVoxelWorldManager::AVoxelWorldManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVoxelWorldManager::BeginPlay()
{
	Super::BeginPlay();

	if (HorizontalRadius < 0 || MinVerticalLayer > MaxVerticalLayer)
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("%s: Invalid voxel grid range. Radius must be nonnegative and MinVerticalLayer <= MaxVerticalLayer."), *GetName());
		return;
	}

	// Widen loop counters so inclusive endpoints cannot overflow int32.
	const int64 Radius = HorizontalRadius;
	for (int64 Z = MinVerticalLayer; Z <= static_cast<int64>(MaxVerticalLayer); ++Z)
	{
		for (int64 Y = -Radius; Y <= Radius; ++Y)
		{
			for (int64 X = -Radius; X <= Radius; ++X)
			{
				SpawnChunk(FIntVector(static_cast<int32>(X), static_cast<int32>(Y), static_cast<int32>(Z)));
			}
		}
	}
}

void AVoxelWorldManager::SpawnChunk(const FIntVector& Coordinate)
{
	if (AVoxelChunkActor** ExistingChunk = SpawnedChunks.Find(Coordinate))
	{
		if (IsValid(*ExistingChunk))
		{
			return;
		}
		SpawnedChunks.Remove(Coordinate);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double ChunkWorldSize = FVoxelChunkData::ChunkSize * FVoxelMesher::VoxelSize;
	const FVector Location(
		static_cast<double>(Coordinate.X) * ChunkWorldSize,
		static_cast<double>(Coordinate.Y) * ChunkWorldSize,
		static_cast<double>(Coordinate.Z) * ChunkWorldSize);
	const FTransform Transform(FQuat::Identity, Location, FVector::OneVector);
	AVoxelChunkActor* Chunk = World->SpawnActorDeferred<AVoxelChunkActor>(
		AVoxelChunkActor::StaticClass(), Transform, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Chunk)
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("%s: Failed to spawn voxel chunk %s."), *GetName(), *Coordinate.ToString());
		return;
	}

	Chunk->WorldSeed = WorldSeed;
	Chunk->ChunkCoordinate = Coordinate;
	Chunk->VoxelMaterial = VoxelMaterial;
	SpawnedChunks.Add(Coordinate, Chunk);
	Chunk->OnDestroyed.AddDynamic(this, &AVoxelWorldManager::HandleChunkDestroyed);
	// FinishSpawning invokes BeginPlay; the chunk generates exactly once with initialized settings.
	Chunk->FinishSpawning(Transform);
}

void AVoxelWorldManager::HandleChunkDestroyed(AActor* DestroyedActor)
{
	// Find by identity in case the chunk's editable coordinate was changed after spawning.
	for (auto It = SpawnedChunks.CreateIterator(); It; ++It)
	{
		if (It.Value() == DestroyedActor)
		{
			It.RemoveCurrent();
			break;
		}
	}
}

void AVoxelWorldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TPair<FIntVector, AVoxelChunkActor*>& Entry : SpawnedChunks)
	{
		if (IsValid(Entry.Value))
		{
			Entry.Value->OnDestroyed.RemoveDynamic(this, &AVoxelWorldManager::HandleChunkDestroyed);
			Entry.Value->Destroy();
		}
	}
	SpawnedChunks.Empty();
	Super::EndPlay(EndPlayReason);
}
