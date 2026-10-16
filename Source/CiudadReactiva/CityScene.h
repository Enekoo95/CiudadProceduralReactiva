// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityScene.generated.h"

class ACityBuilding;
class UStaticMeshComponent;


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

	ACityBuilding* GetHome() const { return Home; }
	ACityBuilding* GetWork() const { return Work; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Scene")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, Category = "Scene")
	TSubclassOf<ACityBuilding> BuildingClass;

	UPROPERTY(EditAnywhere, Category = "Scene", meta = (ClampMin = "1000"))
	float AreaExtent = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "Scene")
	float MinDistanceBetweenBuildings = 2500.0f;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	bool bRandomizeSeed = true;

	UPROPERTY(EditAnywhere, Category = "Scene")
	int32 Seed = 12345;

	UPROPERTY(VisibleAnywhere, Category = "Scene")
	TObjectPtr<UStaticMeshComponent> Ground;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACityBuilding> Home;

	UPROPERTY(Transient)
	TObjectPtr<ACityBuilding> Work;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
