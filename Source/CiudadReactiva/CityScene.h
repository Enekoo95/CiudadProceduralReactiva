// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityScene.generated.h"

class ACityBuilding;
class ACityPerson;
class ACityClock;
class USceneComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;


UCLASS()
class CIUDADREACTIVA_API ACityScene : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACityScene();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Scene")
	void StartScene();

	UFUNCTION(BlueprintCallable, Category = "Scene")
	void ClearScene();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Scene")
	bool bAutoStart = true;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	bool bRandomizeSeed = true;

	UPROPERTY(EditAnywhere, Category = "Scene")
	int32 Seed = 12345;

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSubclassOf<ACityPerson> PersonClass;

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSubclassOf<ACityBuilding> BuildingClass;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UInstancedStaticMeshComponent> BlockSlabs;

	UPROPERTY(EditAnywhere, Category = "City", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlockX = 3;

	UPROPERTY(EditAnywhere, Category = "City", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlockY = 3;

	UPROPERTY(EditAnywhere, Category = "City", meta = (ClampMin = "1000"))
	float BlockSize = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "City", meta = (ClampMin = "400"))
	float StreetWidth = 800.0f;

	UPROPERTY(EditAnywhere, Category = "City", meta = (ClampMin = "1", ClampMax = "4"))
	int32 LotsPerSide = 3;

	UPROPERTY(EditAnywhere, Category = "City", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkChance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "City", meta = (ClampMin = "0", ClampMax = "1"))
	float WorkRatio = 0.3f;

	UPROPERTY(EditAnywhere, Category = "City/People", meta = (ClampMin = "0"))
	int32 Persons = 8;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = 0.0f, ClampMax = 24.0f))
	float DayDuration = 240.0f;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = 0.0f, ClampMax = 24.0f))
	float StartHour = 6.0f;



private:
	FVector2D GetBlockCenter(int32 X, int32 Y) const;

	FVector2D GetTotalSize() const;

	void SpawnBuildingsInBlock(const FVector2D& Center, const FActorSpawnParameters& Params);

	void SpawnPark() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityBuilding>> SpawnedBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityBuilding>> SpawnedPeople;

	UPROPERTY(Transient)
	TObjectPtr<ACityClock> Clock;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
