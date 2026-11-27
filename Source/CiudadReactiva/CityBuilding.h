// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityBuilding.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

UENUM(BlueprintType)
enum class EBuildingType : uint8
{
	Home,
	Work
};

UCLASS()
class CIUDADREACTIVA_API ACityBuilding : public AActor
{
	GENERATED_BODY()
	
public:	
	ACityBuilding();

	void SetBuildingType(EBuildingType NewType);

	void SetSize(const FVector& NewSize);

	/**
	 * Usa una malla de los assets en vez del cubo. La malla se escala para caber en TargetSize (huella X/Y y altura),
	 * se apoya en el suelo y se gira 0 o 90 grados (mas YawOffset) segun lo que mejor encaje en la parcela.
	 * Si InMesh es null cae al cubo con SetSize.
	 */
	void SetBuildingMesh(UStaticMesh* InMesh, const FVector& TargetSize, float YawOffset = 0.f,
		const FLinearColor& Tint = FLinearColor::White, float MaxUniformScale = 1.f);
	EBuildingType GetBuildingType() const { return BuildingType; }

	FVector GetEntranceLocation() const;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Building")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Building")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Building")
	TObjectPtr<USceneComponent> Entrance;

	UPROPERTY(EditAnywhere, Category = "Building")
	EBuildingType BuildingType = EBuildingType::Home;

	UPROPERTY(EditAnywhere, Category = "Building")
	bool bShowLabel = false;

	/** True cuando el edificio usa una malla de los assets (entonces no se tiñe de azul/verde). */
	bool bUsesAssetMesh = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
