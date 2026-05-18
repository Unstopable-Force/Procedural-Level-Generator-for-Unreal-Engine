// Copyright (c) Nikita Yesman. All rigths reserved.
#include "LevelGenerator.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

DEFINE_LOG_CATEGORY(LogLevelGenerator);

ALevelGenerator::ALevelGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	Hedges = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Hedges"));
	Hedges->SetupAttachment(GetRootComponent());
}

void ALevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (!bGenerate)
		return;

	const int32 Seed = (GenerationSeed == 0) ? FMath::Rand() : GenerationSeed;
	RandomStream.Initialize(Seed);
	UE_LOG(LogLevelGenerator, Log, TEXT("Generation started with seed: %d"), Seed);

	RoomList = BaseRoomList;
	SpawnStartRoom();
	SpawnNextRoom();
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

USceneComponent* ALevelGenerator::FindComponentByName(AActor* Actor, const FName& Name) const
{
	if (!Actor)
		return nullptr;

	TArray<USceneComponent*> Components;
	Actor->GetComponents<USceneComponent>(Components);

	for (USceneComponent* Comp : Components)
	{
		if (Comp->GetFName() == Name)
			return Comp;
	}

	return nullptr;
}

// ─── Generation ──────────────────────────────────────────────────────────────

void ALevelGenerator::SpawnStartRoom()
{
	if (!StartRoom)
	{
		UE_LOG(LogLevelGenerator, Error, TEXT("StartRoom is null"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedRoom =
		GetWorld()->SpawnActor<AActor>(StartRoom, GetRootComponent()->GetComponentTransform(), SpawnParams);
	if (!SpawnedRoom)
	{
		UE_LOG(LogLevelGenerator, Error, TEXT("Failed to spawn StartRoom"));
		return;
	}

	LatestRoom = SpawnedRoom;

	USceneComponent* ExitsComponent = FindComponentByName(SpawnedRoom, TEXT("Exits"));
	if (!ExitsComponent)
	{
		UE_LOG(LogLevelGenerator, Warning, TEXT("StartRoom has no Exits component"));
		return;
	}

	TArray<USceneComponent*> ChildComponents;
	ExitsComponent->GetChildrenComponents(true, ChildComponents);
	ExitsList.Append(ChildComponents);

	UE_LOG(LogLevelGenerator, Log, TEXT("StartRoom spawned with %d exit points"), ChildComponents.Num());
}

void ALevelGenerator::SpawnNextRoom()
{
	if (ExitsList.IsEmpty() || RoomList.IsEmpty())
	{
		UE_LOG(LogLevelGenerator, Warning, TEXT("SpawnNextRoom: ExitsList or RoomList is empty"));
		return;
	}

	SelectedExitPoint = ExitsList[RandomStream.RandRange(0, ExitsList.Num() - 1)];
	if (!SelectedExitPoint)
		return;

	const FTransform ExitTransform = SelectedExitPoint->GetComponentTransform();
	const FTransform SpawnTransform(ExitTransform.GetRotation(), ExitTransform.GetLocation(), FVector::OneVector);

	TSubclassOf<AActor> RandomRoomClass = RoomList[RandomStream.RandRange(0, RoomList.Num() - 1)];

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedRoom = GetWorld()->SpawnActor<AActor>(RandomRoomClass, SpawnTransform, SpawnParams);
	if (!SpawnedRoom)
	{
		UE_LOG(LogLevelGenerator, Error, TEXT("Failed to spawn room"));
		return;
	}

	LatestRoom = SpawnedRoom;

	GetWorldTimerManager().SetTimer(OverlapCheckTimerHandle, this, &ALevelGenerator::CheckOverlappings, 0.2f, false);
}

void ALevelGenerator::CheckOverlappings()
{
	AddOverlappingRoom();

	if (!OverlapList.IsEmpty())
	{
		UE_LOG(LogLevelGenerator, Log, TEXT("Overlap detected — retrying. Exits remaining: %d"), ExitsList.Num());

		OverlapList.Empty();
		LatestRoom->Destroy();

		if (ExitsList.IsEmpty())
		{
			UE_LOG(LogLevelGenerator, Warning, TEXT("No exits left after overlap — finalizing early"));
			CloseExits();
			SpawnGates();
			return;
		}

		SpawnNextRoom();
		return;
	}

	OverlapList.Empty();
	RoomAmount--;
	ExitsList.Remove(SelectedExitPoint);
	DoorList.Add(SelectedExitPoint);

	// Collect exits from the newly placed room using same recursive flag as SpawnStartRoom
	USceneComponent* ExitsComponent = FindComponentByName(LatestRoom.Get(), TEXT("Exits"));
	if (ExitsComponent)
	{
		TArray<USceneComponent*> ChildComponents;
		ExitsComponent->GetChildrenComponents(true, ChildComponents);
		ExitsList.Append(ChildComponents);
	}
	else
	{
		UE_LOG(LogLevelGenerator, Warning, TEXT("Placed room has no Exits component"));
	}

	UE_LOG(LogLevelGenerator, Log, TEXT("Room placed. Remaining: %d | Exits in pool: %d"), RoomAmount, ExitsList.Num());

	if (RoomAmount <= 0)
	{
		CloseExits();
		SpawnGates();
		return;
	}

	if (RoomAmount == 1)
		RoomList = EndRoomList;
	else if (RoomAmount % 10 == 0)
		RoomList = SpecialRoomList;
	else
		RoomList = BaseRoomList;

	SpawnNextRoom();
}

void ALevelGenerator::AddOverlappingRoom()
{
	if (!LatestRoom)
		return;

	USceneComponent* GeneratorCollision = FindComponentByName(LatestRoom.Get(), TEXT("GeneratorCollision"));
	if (!GeneratorCollision)
	{
		UE_LOG(LogLevelGenerator, Warning, TEXT("Room has no GeneratorCollision component — overlap check skipped"));
		return;
	}

	TArray<USceneComponent*> ChildComponents;
	GeneratorCollision->GetChildrenComponents(false, ChildComponents);

	for (USceneComponent* Child : ChildComponents)
	{
		UBoxComponent* BoxComp = Cast<UBoxComponent>(Child);
		if (!BoxComp)
			continue;

		TArray<UPrimitiveComponent*> Overlapping;
		BoxComp->GetOverlappingComponents(Overlapping);
		OverlapList.Append(Overlapping);
	}
}

// ─── Finalization ─────────────────────────────────────────────────────────────

void ALevelGenerator::CloseExits()
{
	UE_LOG(LogLevelGenerator, Log, TEXT("Closing %d unused exits"), ExitsList.Num());

	for (USceneComponent* Exit : ExitsList)
	{
		if (!Exit)
			continue;

		Hedges->AddInstance(Exit->GetComponentTransform(), true);
	}
}

void ALevelGenerator::SpawnGates()
{
	if (!bSpawnGates)
		return;

	if (!GateClass)
	{
		UE_LOG(LogLevelGenerator, Error, TEXT("SpawnGates: GateClass is null"));
		return;
	}

	UE_LOG(LogLevelGenerator, Log, TEXT("Spawning %d gates"), DoorList.Num());

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (USceneComponent* Door : DoorList)
	{
		if (!Door)
			continue;

		GetWorld()->SpawnActor<AActor>(GateClass, Door->GetComponentTransform(), SpawnParams);
	}
}