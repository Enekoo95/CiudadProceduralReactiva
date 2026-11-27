#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityLayout.h"
#include "CityScene.generated.h"

class ACityBuilding;
class ACityPerson;
class ACityClock;
class USceneComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UPCGComponent;
class UPCGGraphInterface;

UCLASS()
class CIUDADREACTIVA_API ACityScene : public AActor, public ICityLayoutSource
{
	GENERATED_BODY()

public:
	ACityScene();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** ICityLayoutSource: el nodo PCG "City Layout" lee los parametros de aqui, asi grafo y edificios siempre coinciden. */
	virtual FCityLayoutParams GetCityLayoutParams() const override;

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

	/** Grafo PCG que genera calles y aceras (nodos City Layout + Street Surfaces del plugin EnekoMartinTools). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Tool")
	TObjectPtr<UPCGGraphInterface> CityGraph;

	/** Si esta activo y hay CityGraph, calles y aceras las genera el plugin. Si no, se usan las losas simples de antes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Tool")
	bool bUsePCGStreets = true;

	// ---- Assets (mallas y materiales) ----

	/** Mallas para edificios bajos. Por defecto, las casas de CARLA. Si estan vacias se usan cubos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	TArray<TObjectPtr<UStaticMesh>> HouseMeshes;

	/** Mallas para edificios altos. Por defecto, los bloques de pisos de CARLA. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	TArray<TObjectPtr<UStaticMesh>> ApartmentMeshes;

	/** Por encima de este punto de la altura (0 = minima, 1 = maxima) el edificio es un bloque de pisos y no una casa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets", meta = (ClampMin = "0", ClampMax = "1"))
	float ApartmentHeightThreshold = 0.5f;

	/** Giro extra (grados) para que la fachada de la malla mire a la calle. Prueba 90, 180 o 270 si salen de lado. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	float BuildingMeshYawOffset = 0.0f;

	/** Mallas para edificios de TRABAJO (oficinas, hotel, museo...). Las casas y bloques de pisos son siempre de tipo casa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	TArray<TObjectPtr<UStaticMesh>> WorkMeshes;

	/** Material de las calles (por defecto, el asfalto de CARLA). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	TObjectPtr<UMaterialInterface> RoadMaterial;

	/** Material de las aceras / manzanas (por defecto, la acera de CARLA). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	TObjectPtr<UMaterialInterface> SidewalkMaterial;

	/** Tamaño (cm) de cada baldosa de suelo; la textura se repite una vez por baldosa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets", meta = (ClampMin = "100"))
	float GroundTileSize = 400.0f;

	/** Arboles que se plantan dentro de los parques. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	TArray<TObjectPtr<UStaticMesh>> TreeMeshes;

	/** Distancia aproximada entre arboles (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets", meta = (ClampMin = "100"))
	float TreeSpacing = 500.0f;

	/** Escala de los arboles (los de CARLA miden ~10 m; esta ciudad es pequena). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets", meta = (ClampMin = "0.05"))
	float TreeScale = 0.35f;

	/** Multiplicador de las medidas (calle, bloques, parcela) que llegan desde el widget: los assets de CARLA son grandes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0.1"))
	float WidgetUnitScale = 3.0f;

	/** Altura aproximada de los arboles en cm (cada malla se escala a esta altura, con algo de variacion). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets", meta = (ClampMin = "100"))
	float TreeHeight = 450.0f;

	/** Material de los parques (por defecto, el cesped de CARLA). Si es null se usa un color liso. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Assets")
	TObjectPtr<UMaterialInterface> ParkMaterial;

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

	/** Altura de la acera. Debe coincidir con SidewalkHeight del nodo Street Surfaces en el grafo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0"))
	float SidewalkHeight = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksX = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "1", ClampMax = "8"))
	int32 BlocksY = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float StreetWidth = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float MinBlockSize = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float MaxBlockSize = 5400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "200"))
	float TargetLotSize = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0", ClampMax = "1"))
	float ParkChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MinFootprint = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "0.2", ClampMax = "0.9"))
	float MaxFootprint = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "100"))
	float MinBuildingHeight = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City|Layout", meta = (ClampMin = "100"))
	float MaxBuildingHeight = 3000.0f;

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

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UInstancedStaticMeshComponent> RoadTiles;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UPCGComponent> PCG;

protected:
	virtual void BeginPlay() override;

private:
	void SanitizeParams();

	void RebuildGround();
	void RebuildStreets();
	void RebuildTrees();
	UStaticMesh* PickBuildingMesh(const FCityLot& Lot, int32 LotIndex, bool bWork) const;
	bool IsWorkLot(int32 LotIndex) const;

	/** Indices de las parcelas que son trabajo (las mas altas). */
	TSet<int32> WorkLots;
	void BuildLayout();
	void GenerateLayoutInternal(bool bNewSeed);

	FName GetOwnTag() const;

	/** Layout compartido con el plugin; lo calcula RebuildGround. */
	FCityLayout Layout;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> TreeComponents;

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
