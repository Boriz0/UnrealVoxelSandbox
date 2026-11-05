#include "Voxels/VoxelWorldManager.h"

#include "Engine/World.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealMinecraft.h"
#include "Voxels/VoxelChunkActor.h"
#include "Voxels/VoxelChunkData.h"
#include "Voxels/VoxelMesher.h"
#include "Voxels/VoxelWorldGenerator.h"

namespace
{
	uint8 GeneratedVoxel(const FVoxelWorldGenerator& Terrain, const FVoxelCatalogSnapshot& Catalog, FIntVector World)
	{
		return World.Z > Terrain.GetSurfaceHeight(World.X, World.Y)
			? VoxelAir : Catalog.SolidAtHeight(World.Z);
	}

	void SplitWorldVoxel(FIntVector World, FIntVector& Chunk, FIntVector& Local)
	{
		constexpr int32 Size = FVoxelChunkData::ChunkSize;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const int32 Remainder = World[Axis] % Size;
			Local[Axis] = Remainder < 0 ? Remainder + Size : Remainder;
			Chunk[Axis] = World[Axis] / Size - (Remainder < 0);
		}
	}

	template<typename F>
	void ForEachNeighbor(FIntVector Chunk, F&& Visit)
	{
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			for (int32 Sign : {-1, 1})
			{
				const int64 Next = static_cast<int64>(Chunk[Axis]) + Sign;
				if (Next >= MIN_int32 && Next <= MAX_int32)
				{
					FIntVector Neighbor = Chunk;
					Neighbor[Axis] = static_cast<int32>(Next);
					Visit(Neighbor, Axis, Sign);
				}
			}
		}
	}
}

static void SortByDistance(TArray<FIntVector>& Coordinates, FIntVector PlayerChunk)
{
	Coordinates.Sort([PlayerChunk](const FIntVector& A, const FIntVector& B)
	{
		const auto DistanceSquared = [PlayerChunk](const FIntVector& Coordinate)
		{
			const double DX = static_cast<double>(Coordinate.X) - PlayerChunk.X;
			const double DY = static_cast<double>(Coordinate.Y) - PlayerChunk.Y;
			return DX * DX + DY * DY;
		};
		const double DistanceA = DistanceSquared(A);
		const double DistanceB = DistanceSquared(B);
		if (DistanceA != DistanceB)
		{
			return DistanceA < DistanceB;
		}
		// For equally near columns, load the player's vertical layer first.
		const int64 HeightA = FMath::Abs(static_cast<int64>(A.Z) - PlayerChunk.Z);
		const int64 HeightB = FMath::Abs(static_cast<int64>(B.Z) - PlayerChunk.Z);
		if (HeightA != HeightB)
		{
			return HeightA < HeightB;
		}
		if (A.Z != B.Z)
		{
			return A.Z < B.Z;
		}
		if (A.Y != B.Y)
		{
			return A.Y < B.Y;
		}
		return A.X < B.X;
	});
}

AVoxelWorldManager::AVoxelWorldManager()
{
	Catalog = UVoxelCatalog::ConstructorDefault();
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void AVoxelWorldManager::BeginPlay()
{
	Super::BeginPlay();
	FString Error;
	CatalogData = Catalog ? Catalog->BuildSnapshot(Error) : nullptr;
	if (!CatalogData)
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("Cannot stream voxels without a valid catalog: %s"), *Error);
		SetActorTickEnabled(false);
		return;
	}

	if (HorizontalRadius < 0 || UnloadRadius <= HorizontalRadius
		|| MaxChunksPerFrame < 1 || MaxConcurrentTasks < 1 || MaxMeshUploadsPerFrame < 1 || MinVerticalLayer > MaxVerticalLayer)
	{
		UE_LOG(LogUnrealMinecraft, Error, TEXT("%s: Invalid streaming settings. Require 0 <= HorizontalRadius < UnloadRadius, positive build and upload budgets, and MinVerticalLayer <= MaxVerticalLayer."), *GetName());
		SetActorTickEnabled(false);
	}
}

void AVoxelWorldManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const AActor* Source = bPreview ? PreviewCamera.Get() : UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(Source))
	{
		for (auto& Pair : Builds)
		{
			Pair.Value.bObsolete = true;
		}
		// Wait for initial possession or respawn without generating at an obsolete location.
		PendingLoads.Reset();
		NextPendingLoad = 0;
		bHasStreamingCenter = false;
		return;
	}

	const double ChunkWorldSize = FVoxelChunkData::ChunkSize * FVoxelMesher::VoxelSize;
	const FVector PlayerLocation = Source->GetActorLocation();
	// Floor, not truncation: -1 cm belongs to chunk -1, not chunk 0.
	const FIntVector PlayerChunk(
		FMath::FloorToInt(PlayerLocation.X / ChunkWorldSize),
		FMath::FloorToInt(PlayerLocation.Y / ChunkWorldSize),
		FMath::FloorToInt(PlayerLocation.Z / ChunkWorldSize));
	if (!bHasStreamingCenter || bNeedsStreamingUpdate
		|| PlayerChunk.X != StreamingCenter.X || PlayerChunk.Y != StreamingCenter.Y)
	{
		UpdateStreaming(PlayerChunk);
	}

	UploadCompletedBuilds();
	int32 GeneratedThisFrame = 0;
	while (NextPendingLoad < PendingLoads.Num() && GeneratedThisFrame < MaxChunksPerFrame
		&& Builds.Num() < MaxConcurrentTasks)
	{
		const FIntVector Coordinate = PendingLoads[NextPendingLoad++];
		if (Builds.Contains(Coordinate))
		{
			continue;
		}
		AVoxelChunkActor* Chunk = SpawnedChunks.FindRef(Coordinate);
		FBuildRequest Request;
		TOptional<FVoxelChunkData> Snapshot;
		if (IsValid(Chunk))
		{
			if (!DirtyChunks.Contains(Coordinate))
			{
				continue;
			}
			Request.bRemesh = true;
			Request.Source = Chunk;
			Request.Revision = Chunk->Revision;
			Snapshot = Chunk->ChunkData;
		}
		FVoxelBorders Borders;
		TArray<FVoxelProceduralBorder> MissingBorders;
		ForEachNeighbor(Coordinate, [&](FIntVector Neighbor, int32 Axis, int32 Sign)
		{
			const AVoxelChunkActor* Adjacent = SpawnedChunks.FindRef(Neighbor);
			if (IsValid(Adjacent) && Adjacent->GetChunkData())
			{
				Borders.Capture(Axis, Sign, *Adjacent->GetChunkData());
			}
			else if (Neighbor.Z >= MinVerticalLayer && Neighbor.Z <= MaxVerticalLayer)
			{
				MissingBorders.Add(FVoxelProceduralBorder{Neighbor, Axis, Sign,
					EditStore.SnapshotFace(Neighbor, Axis, FVoxelBorders::TouchingLayer(Sign))});
			}
		});
		Request.Task = FVoxelChunkBuild::Launch(CatalogData.ToSharedRef(), WorldSeed, Coordinate, MoveTemp(Snapshot),
			Request.bRemesh ? FVoxelOverrides{} : EditStore.Snapshot(Coordinate), MoveTemp(Borders), MoveTemp(MissingBorders));
		Builds.Add(Coordinate, MoveTemp(Request));
		++GeneratedThisFrame;
	}
	if (NextPendingLoad == PendingLoads.Num())
	{
		PendingLoads.Reset();
		NextPendingLoad = 0;
	}
}

bool AVoxelWorldManager::IsWithinHorizontalRadius(const FIntVector& Coordinate, int32 Radius) const
{
	return FMath::Abs(static_cast<int64>(Coordinate.X) - StreamingCenter.X) <= Radius
		&& FMath::Abs(static_cast<int64>(Coordinate.Y) - StreamingCenter.Y) <= Radius;
}

void AVoxelWorldManager::UpdateStreaming(const FIntVector& PlayerChunk)
{
	StreamingCenter = PlayerChunk;
	bHasStreamingCenter = true;
	bNeedsStreamingUpdate = false;

	// Remove references before destruction; unbind to avoid mutating the map in the callback.
	for (auto It = SpawnedChunks.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Value()) || !IsWithinHorizontalRadius(It.Key(), UnloadRadius)
			|| It.Key().Z < MinVerticalLayer || It.Key().Z > MaxVerticalLayer)
		{
			DirtyChunks.Remove(It.Key());
			AVoxelChunkActor* Chunk = It.Value();
			It.RemoveCurrent();
			if (IsValid(Chunk))
			{
				Chunk->OnDestroyed.RemoveDynamic(this, &AVoxelWorldManager::HandleChunkDestroyed);
				Chunk->Destroy();
			}
		}
	}

	for (auto& Pair : Builds)
	{
		FBuildRequest& Request = Pair.Value;
		// Cancellation stays in effect if the player returns before this task finishes.
		if (Request.bRemesh ? !Request.Source.IsValid() : !IsWithinHorizontalRadius(Pair.Key, HorizontalRadius))
		{
			Request.bObsolete = true;
		}
	}

	// Replacing the queue cancels obsolete requests and enumerates each missing coordinate once.
	// This runs on horizontal chunk transitions, not on every frame or vertical movement.
	PendingLoads.Reset();
	NextPendingLoad = 0;
	const int64 Radius = HorizontalRadius;
	for (int64 Z = MinVerticalLayer; Z <= static_cast<int64>(MaxVerticalLayer); ++Z)
	{
		for (int64 Y = static_cast<int64>(PlayerChunk.Y) - Radius; Y <= static_cast<int64>(PlayerChunk.Y) + Radius; ++Y)
		{
			for (int64 X = static_cast<int64>(PlayerChunk.X) - Radius; X <= static_cast<int64>(PlayerChunk.X) + Radius; ++X)
			{
				if (X < MIN_int32 || X > MAX_int32 || Y < MIN_int32 || Y > MAX_int32)
				{
					continue;
				}
				const FIntVector Coordinate(static_cast<int32>(X), static_cast<int32>(Y), static_cast<int32>(Z));
				if (!SpawnedChunks.Contains(Coordinate) && !Builds.Contains(Coordinate))
				{
					PendingLoads.Add(Coordinate);
				}
			}
		}
	}

	for (const FIntVector& Coordinate : DirtyChunks)
	{
		if (!Builds.Contains(Coordinate))
		{
			PendingLoads.Add(Coordinate);
		}
	}
	SortByDistance(PendingLoads, PlayerChunk);
}

bool AVoxelWorldManager::GetWorldVoxel(FIntVector WorldVoxel, uint8& OutType) const
{
	check(IsInGameThread());
	FIntVector Chunk, Local;
	SplitWorldVoxel(WorldVoxel, Chunk, Local);
	const AVoxelChunkActor* Actor = SpawnedChunks.FindRef(Chunk);
	const FVoxelChunkData* Data = IsValid(Actor) ? Actor->GetChunkData() : nullptr;
	OutType = Data ? Data->GetVoxel(Local) : VoxelAir;
	return Data != nullptr;
}

bool AVoxelWorldManager::IsBottomVoxel(FIntVector WorldVoxel) const
{
	return static_cast<int64>(WorldVoxel.Z) == static_cast<int64>(MinVerticalLayer) * FVoxelChunkData::ChunkSize;
}

bool AVoxelWorldManager::SetWorldVoxel(FIntVector WorldVoxel, uint8 Type)
{
	check(IsInGameThread());
	if (bPreview || (Type == VoxelAir && IsBottomVoxel(WorldVoxel)))
	{
		return false;
	}
	FIntVector Chunk, Local;
	SplitWorldVoxel(WorldVoxel, Chunk, Local);
	AVoxelChunkActor* Actor = SpawnedChunks.FindRef(Chunk);
	if (!IsValid(Actor) || !Actor->GetChunkData() || (Type != VoxelAir && !Catalog->Find(Type)))
	{
		return false;
	}
	const uint8 Previous = Actor->ChunkData->GetVoxel(Local);
	if (Previous != Type)
	{
		Actor->ChunkData->SetVoxel(Local, Type);
		EditStore.Record(Chunk, Local, Type, GeneratedVoxel(FVoxelWorldGenerator(WorldSeed), *CatalogData, WorldVoxel));
		MarkChunkDirty(Chunk);
		if (Previous == VoxelAir || Type == VoxelAir)
		{
			InvalidateNeighbors(Chunk, Local);
		}
	}
	return true;
}

void AVoxelWorldManager::InvalidateNeighbors(FIntVector Chunk, FIntVector EditedLocal)
{
	ForEachNeighbor(Chunk, [&](FIntVector Neighbor, int32 Axis, int32 Sign)
	{
		if (EditedLocal[Axis] != FVoxelBorders::TouchingLayer(-Sign))
		{
			return;
		}
		// Loaded chunks have revisions. An initial build has no Actor yet, so invalidate its snapshot directly.
		const AVoxelChunkActor* Adjacent = SpawnedChunks.FindRef(Neighbor);
		if (IsValid(Adjacent) && Adjacent->GetChunkData())
		{
			FIntVector Across = EditedLocal;
			Across[Axis] = FVoxelBorders::TouchingLayer(Sign);
			if (Adjacent->GetChunkData()->GetVoxel(Across) != VoxelAir)
			{
				MarkChunkDirty(Neighbor);
			}
		}
		else if (FBuildRequest* Pending = Builds.Find(Neighbor))
		{
			Pending->bObsolete = true;
		}
	});
}

bool AVoxelWorldManager::MarkChunkDirty(FIntVector Coordinate)
{
	check(IsInGameThread());
	AVoxelChunkActor* Chunk = SpawnedChunks.FindRef(Coordinate);
	if (!IsValid(Chunk) || !Chunk->GetChunkData())
	{
		return false;
	}
	++Chunk->Revision;
	DirtyChunks.Add(Coordinate);
	bNeedsStreamingUpdate = true;
	return true;
}

void AVoxelWorldManager::UploadCompletedBuilds()
{
	TArray<FIntVector> Ready;
	for (const auto& Pair : Builds)
	{
		if (Pair.Value.Task.IsCompleted())
		{
			Ready.Add(Pair.Key);
		}
	}
	SortByDistance(Ready, StreamingCenter);
	int32 Uploads = 0;
	for (const FIntVector& Coordinate : Ready)
	{
		FBuildRequest& Request = Builds.FindChecked(Coordinate);
		AVoxelChunkActor* Chunk = SpawnedChunks.FindRef(Coordinate);
		const bool bStaleMesh = Request.bRemesh && (!IsValid(Chunk)
			|| Request.Source.Get() != Chunk || Request.Revision != Chunk->Revision);
		if (Request.bObsolete || bStaleMesh)
		{
			Builds.Remove(Coordinate);
			bNeedsStreamingUpdate = true;
			continue;
		}
		if (Uploads >= MaxMeshUploadsPerFrame)
		{
			continue;
		}
		if (!Request.bRemesh)
		{
			Chunk = SpawnChunk(Coordinate);
		}
		if (IsValid(Chunk))
		{
			Chunk->ApplyBuild(Request.Task.GetResult());
			DirtyChunks.Remove(Coordinate);
			++Uploads;
		}
		else
		{
			bNeedsStreamingUpdate = true;
		}
		Builds.Remove(Coordinate);
	}
}

AVoxelChunkActor* AVoxelWorldManager::SpawnChunk(const FIntVector& Coordinate)
{
	if (AVoxelChunkActor** ExistingChunk = SpawnedChunks.Find(Coordinate))
	{
		if (IsValid(*ExistingChunk))
		{
			return *ExistingChunk;
		}
		SpawnedChunks.Remove(Coordinate);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
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
		return nullptr;
	}

	Chunk->WorldSeed = WorldSeed;
	Chunk->ChunkCoordinate = Coordinate;
	Chunk->VoxelMaterial = VoxelMaterial;
	Chunk->Catalog = Catalog;
	SpawnedChunks.Add(Coordinate, Chunk);
	Chunk->OnDestroyed.AddDynamic(this, &AVoxelWorldManager::HandleChunkDestroyed);
	// Managed chunks receive their completed build after BeginPlay.
	Chunk->FinishSpawning(Transform);
	return IsValid(Chunk) ? Chunk : nullptr;
}

void AVoxelWorldManager::HandleChunkDestroyed(AActor* DestroyedActor)
{
	// Find by identity in case the chunk's editable coordinate was changed after spawning.
	for (auto It = SpawnedChunks.CreateIterator(); It; ++It)
	{
		if (It.Value() == DestroyedActor)
		{
			DirtyChunks.Remove(It.Key());
			It.RemoveCurrent();
			bNeedsStreamingUpdate = true;
			break;
		}
	}
}

void AVoxelWorldManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetActorTickEnabled(false);
	ClearStreamedWorld();
	Super::EndPlay(EndPlayReason);
}

void AVoxelWorldManager::SetMenuPreview(AActor* Camera)
{
	ClearStreamedWorld();
	EditStore = {};
	PreviewCamera = Camera;
	bPreview = true;
	bNeedsStreamingUpdate = true;
}

void AVoxelWorldManager::StartWorld(int32 Seed, const TMap<FIntVector, uint8>& WorldEdits)
{
	ClearStreamedWorld();
	EditStore = FVoxelEditStore{};
	const FVoxelWorldGenerator Terrain(Seed);
	for (const auto& Edit : WorldEdits)
	{
		// Restore the natural floor if an older save mined through the bottom row.
		if (Edit.Value == VoxelAir && IsBottomVoxel(Edit.Key))
		{
			continue;
		}
		FIntVector Chunk, Local;
		SplitWorldVoxel(Edit.Key, Chunk, Local);
		EditStore.Record(Chunk, Local, Edit.Value, GeneratedVoxel(Terrain, *CatalogData, Edit.Key));
	}
	WorldSeed = Seed;
	bPreview = false;
	PreviewCamera.Reset();
}

bool AVoxelWorldManager::IsInitialGridReady() const
{
	return !bPreview && bHasStreamingCenter && SpawnedChunks.Num() > 0 && !bNeedsStreamingUpdate
		&& PendingLoads.IsEmpty() && Builds.IsEmpty();
}

bool AVoxelWorldManager::IsRestoreAreaReady(FVector Location, double CapsuleRadius) const
{
	// A saved player can be underground, airborne, or above a player-built platform.
	// Wait for the loaded columns under the capsule, including their async collision cooks.
	const double Span = FVoxelChunkData::ChunkSize * FVoxelMesher::VoxelSize;
	for (int32 X = FMath::FloorToInt((Location.X - CapsuleRadius) / Span); X <= FMath::FloorToInt((Location.X + CapsuleRadius) / Span); ++X)
	{
		for (int32 Y = FMath::FloorToInt((Location.Y - CapsuleRadius) / Span); Y <= FMath::FloorToInt((Location.Y + CapsuleRadius) / Span); ++Y)
		{
			if (!IsWithinHorizontalRadius(FIntVector(X, Y, 0), HorizontalRadius))
			{
				continue;
			}
			for (int64 Z = MinVerticalLayer; Z <= MaxVerticalLayer; ++Z)
			{
				const AVoxelChunkActor* Chunk = SpawnedChunks.FindRef(FIntVector(X, Y, static_cast<int32>(Z)));
				if (!IsValid(Chunk) || !Chunk->GetChunkData())
				{
					return false;
				}
				if (Chunk->ProceduralMesh->ContainsPhysicsTriMeshData(true) && Chunk->ProceduralMesh->GetBodySetup()->TriMeshGeometries.IsEmpty())
				{
					return false;
				}
			}
		}
	}
	return bHasStreamingCenter;
}

bool AVoxelWorldManager::HasSurfaceCollision(const FVector& Surface) const
{
	constexpr double Size = FVoxelMesher::VoxelSize;
	constexpr double Span = Size * FVoxelChunkData::ChunkSize;
	const FIntVector Coordinate(FMath::FloorToInt(Surface.X / Span), FMath::FloorToInt(Surface.Y / Span),
		FMath::FloorToInt((Surface.Z - 1.) / Span));
	const AVoxelChunkActor* Chunk = SpawnedChunks.FindRef(Coordinate);
	if (!IsValid(Chunk))
	{
		return false;
	}
	const FVector Center((FMath::FloorToDouble(Surface.X / Size) + 0.5) * Size,
		(FMath::FloorToDouble(Surface.Y / Size) + 0.5) * Size, Surface.Z);
	FHitResult Hit;
	return Chunk->ProceduralMesh->LineTraceComponent(Hit, Center + FVector(0, 0, 5), Center - FVector(0, 0, 5),
		FCollisionQueryParams(SCENE_QUERY_STAT(MenuSpawnGround), true));
}

void AVoxelWorldManager::ClearStreamedWorld()
{
	// Workers own their inputs. Dropping these handles cannot schedule an upload.
	Builds.Empty();
	DirtyChunks.Empty();
	PendingLoads.Empty();
	NextPendingLoad = 0;
	bHasStreamingCenter = false;
	bNeedsStreamingUpdate = false;
	for (const TPair<FIntVector, AVoxelChunkActor*>& Entry : SpawnedChunks)
	{
		if (IsValid(Entry.Value))
		{
			Entry.Value->OnDestroyed.RemoveDynamic(this, &AVoxelWorldManager::HandleChunkDestroyed);
			Entry.Value->Destroy();
		}
	}
	SpawnedChunks.Empty();
}
