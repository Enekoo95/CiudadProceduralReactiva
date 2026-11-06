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


	// La Tool
	UFUNCTION(BlueprintCallable, Category = "City|Tool")
	void GenerateLayout();

	UFUNCTION(BlueprintPure, Category = "City|Tool")
	int32 GetNumBuildings() const { return SpawnedBuildings.Num(); }

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

	//El layout de la ciudad
	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksX = 6;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksY = 6;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "200"))
	float StreetWidth = 300.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "200"))
	float MinBlockSize = 600.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "200"))
	float MaxBlockSize = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float TargetLotSize = 500.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkChance = 0.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MinFootprint = 0.5f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MaxFootprint = 0.75f;

	UPROPERTY(EditAnywhere, Category = "City|Layout")
	float MinBuildingHeight = 600.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout")
	float MaxBuildingHeight = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "0.5"))
	float MinHeightRatio = 1.5f;

	UPROPERTY(EditAnywhere, Category = "City|Layout", meta = (ClampMin = "0.5"))
	float MaxHeightRatio = 5.0f;

	//Perlin
	UPROPERTY(EditAnywhere, Category = "City|Perlin")
	bool bUsePerlinNoise = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0.01", ClampMax = "2"))
	float BlockSizeNoiseScale = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ParkNoiseScale = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkThreshold = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0.01", ClampMax = "1"))
	float HeightNoiseScale = 0.2f;

	// La people y el reloj
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

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UInstancedStaticMeshComponent> ParkSlabs;

private:
	float Perlin01(float X, float Y, float Scale) const;

	float GetBlockExtent(int32 Index, int32 Axis) const;

	bool IsParkBlock(int32 X, int32 Y) const;

	void BuildLayoutCache();
	FVector2D GetBlockCenter(int32 X, int32 Y) const;
	FVector2D GetBlockSize(int32 X, int32 Y) const;

	void RebuildGround();
	void BuildLayout(FRandomStream& Rng);
	void SpawnBuildingsInBlock(int32 BlockX, int32 BlockY, FRandomStream& Rng, const FActorSpawnParameters& Params);

	FName GetOwnTag() const;

	TArray<float> ColSize;
	TArray<float> RowSize;
	TArray<float> ColStart;
	TArray<float> RowStart;
	FVector2D TotalSize = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityBuilding>> SpawnedBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityPerson>> SpawnedPeople;

	UPROPERTY(Transient)
	TObjectPtr<ACityClock> Clock;
};