// Copyright (c) Nikita Yesman. All rigths reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelGenerator.generated.h"

class UInstancedStaticMeshComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogLevelGenerator, Log, All);

UCLASS(meta = (PrioritizeCategories = "Generation Rooms Runtime"))
class FRACTUREDDIMENSION_API ALevelGenerator : public AActor
{
	GENERATED_BODY()

public:
	ALevelGenerator();

protected:
	virtual void BeginPlay() override;

private:
	void SpawnStartRoom();
	void SpawnNextRoom();
	void CheckOverlappings();
	void AddOverlappingRoom();
	void CloseExits();
	void SpawnGates();

	USceneComponent* FindComponentByName(AActor* Actor, const FName& Name) const;

	FTimerHandle OverlapCheckTimerHandle;
	FRandomStream RandomStream;

	TObjectPtr<USceneComponent> SelectedExitPoint;

public:
	/** Instanced mesh used to fill sealed exit points at generation end */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation")
	TObjectPtr<UInstancedStaticMeshComponent> Hedges;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bGenerate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bSpawnGates = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSubclassOf<AActor> GateClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1"))
	int32 RoomAmount = 10;

	/** 0 = random seed each run */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 GenerationSeed = 0;

	/** Class of the first room to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms")
	TSubclassOf<AActor> StartRoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms")
	TArray<TSubclassOf<AActor>> BaseRoomList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms")
	TArray<TSubclassOf<AActor>> SpecialRoomList;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rooms")
	TArray<TSubclassOf<AActor>> EndRoomList;

	/** Stored as AActor* since MasterRoom is a Blueprint class */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Runtime")
	TObjectPtr<AActor> LatestRoom;

	/** Working pool, initialized from BaseRoomList at generation start */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Runtime")
	TArray<TSubclassOf<AActor>> RoomList;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Runtime")
	TArray<TObjectPtr<USceneComponent>> ExitsList;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Runtime")
	TArray<TObjectPtr<UPrimitiveComponent>> OverlapList;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Runtime")
	TArray<TObjectPtr<USceneComponent>> DoorList;
};