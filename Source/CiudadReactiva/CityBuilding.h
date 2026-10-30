// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityBuilding.generated.h"

class UStaticMeshComponent;
class USceneComponent;

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
