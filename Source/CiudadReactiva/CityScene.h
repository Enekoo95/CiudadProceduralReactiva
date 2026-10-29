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
	ACityScene();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Scene")
	void StartScene();

	UFUNCTION(BlueprintCallable, Category = "Scene")
	void ClearScene();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Scene")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, Category = "Scene")
	bool bRandomizeSeed = true;

	UPROPERTY(EditAnywhere, Category = "Scene")
	int32 Seed = 12345;

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSubclassOf<ACityBuilding> BuildingClass;

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSubclassOf<ACityPerson> PersonClass;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksX = 3;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksY = 3;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "1000"))
	float BlockSize = 2400.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "400"))
	float StreetWidth = 800.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "4"))
	int32 LotsPerSide = 2;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkChance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "City|People", meta = (ClampMin = "0", ClampMax = "1"))
	float WorkRatio = 0.3f;

	UPROPERTY(EditAnywhere, Category = "City|People", meta = (ClampMin = "0"))
	int32 MaxCitizens = 8;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "1"))
	float DayDuration = 240.0f;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "0", ClampMax = "24"))
	float StartHour = 6.0f;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UInstancedStaticMeshComponent> BlockSlabs;

private:
	FVector2D GetBlockCenter(int32 X, int32 Y) const;

	FVector2D GetTotalSize() const;

	void SpawnBuildingsInBlock(const FVector2D& Center, const FActorSpawnParameters& Params);

	void SpawnPark(const FVector2D& Center);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityBuilding>> SpawnedBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityPerson>> SpawnedPeople;

	UPROPERTY(Transient)
	TObjectPtr<ACityClock> Clock;
};