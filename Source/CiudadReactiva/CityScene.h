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
class UMaterialInstanceDynamic;

UCLASS()
class CIUDADREACTIVA_API ACityScene : public AActor
{
	GENERATED_BODY()

public:
	ACityScene();

	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Scene")
	void StartScene();

	UFUNCTION(BlueprintCallable, Category = "Scene")
	void ClearScene();

	// ---- La Tool ----

	/** Boton "Generate": ciudad nueva. Si bRandomizeSeed esta activo, usa una seed nueva. */
	UFUNCTION(BlueprintCallable, Category = "City|Tool")
	void GenerateLayout();

	/** Regenera con la seed actual (el mismo "mapa" pero con los parametros nuevos). */
	UFUNCTION(BlueprintCallable, Category = "City|Tool")
	void RegenerateLayout();

	/** Aplica todos los parametros del widget de una vez y regenera con la seed actual. */
	UFUNCTION(BlueprintCallable, Category = "City|Tool")
	void ApplyParamsAndRegenerate(
		int32 InBlocksX, int32 InBlocksY,
		float InStreetWidth,
		float InMinBlockSize, float InMaxBlockSize,
		float InLotSize,
		float InBlockNoise, float InParkNoise, float InParkThreshold, float InHeightNoise);

	UFUNCTION(BlueprintPure, Category = "City|Tool")
	int32 GetNumBuildings() const { return SpawnedBuildings.Num(); }

	/** En el editor: al cambiar una propiedad en el panel de detalles, regenera tambien los edificios. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Tool")
	bool bAutoRegenerateInEditor = true;

	// ---- Escena ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	bool bRandomizeSeed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene")
	int32 Seed = 12345;

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSubclassOf<ACityBuilding> BuildingClass;

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSubclassOf<ACityPerson> PersonClass;

	// ---- Layout: cuantas manzanas, calles y tamaños ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "20"))
	int32 BlocksX = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "20"))
	int32 BlocksY = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200", ClampMax = ""))
	float StreetWidth = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float MinBlockSize = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float MaxBlockSize = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float TargetLotSize = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MinFootprint = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MaxFootprint = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "100"))
	float MinBuildingHeight = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "100"))
	float MaxBuildingHeight = 2000.0f;

	// ---- Perlin ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin")
	bool bUsePerlinNoise = true;

	/** Lo bruscamente que cambia el tamaño de una manzana a la siguiente (bajo = gradual, alto = caotico). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0.01", ClampMax = "2"))
	float BlockSizeNoiseScale = 0.6f;

	/** Agrupacion de los parques (bajo = zonas grandes de parque, alto = repartidos). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ParkNoiseScale = 0.35f;

	/** Cantidad de parques: 0 = ninguno, 1 = todas las manzanas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkThreshold = 0.2f;

	/** Variacion de altura (bajo = zonas grandes altas/bajas, alto = edificio a edificio). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Perlin", meta = (ClampMin = "0.01", ClampMax = "1"))
	float HeightNoiseScale = 0.2f;

	// ---- Gente y reloj ----

	UPROPERTY(EditAnywhere, Category = "City|People", meta = (ClampMin = "0", ClampMax = "1"))
	float WorkRatio = 0.3f;

	UPROPERTY(EditAnywhere, Category = "City|People", meta = (ClampMin = "0"))
	int32 MaxCitizens = 8;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "1"))
	float DayDuration = 240.0f;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "0", ClampMax = "24"))
	float StartHour = 6.0f;

	// ---- Componentes ----

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UStaticMeshComponent> Ground;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UInstancedStaticMeshComponent> BlockSlabs;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UInstancedStaticMeshComponent> ParkSlabs;

protected:
	virtual void BeginPlay() override;

private:
	float Perlin01(float X, float Y, float Scale) const;

	float NoiseNorm(float X, float Y, float Scale) const;

	float GetBlockExtent(int32 Index, int32 Axis) const;
	bool IsParkBlock(int32 X, int32 Y) const;

	void SanitizeParams();

	void BuildLayoutCache();
	FVector2D GetBlockCenter(int32 X, int32 Y) const;
	FVector2D GetBlockSize(int32 X, int32 Y) const;

	void RebuildGround();
	void BuildLayout();
	void GenerateLayoutInternal(bool bNewSeed);
	void SpawnBuildingsInBlock(int32 BlockX, int32 BlockY, const FActorSpawnParameters& Params);

	FName GetOwnTag() const;

	TArray<float> ColSize;
	TArray<float> RowSize;
	TArray<float> ColStart;
	TArray<float> RowStart;
	FVector2D TotalSize = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GroundMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BlockMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ParkMID;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityBuilding>> SpawnedBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACityPerson>> SpawnedPeople;

	UPROPERTY(Transient)
	TObjectPtr<ACityClock> Clock;
};