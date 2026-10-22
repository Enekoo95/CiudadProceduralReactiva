// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CityPerson.generated.h"

class ACityBuilding;
class UStaticMeshComponent;

UCLASS()
class CIUDADREACTIVA_API ACityPerson : public ACharacter
{
	GENERATED_BODY()

public:

	ACityPerson();

	void SetRoute(ACityBuilding* InHome, ACityBuilding* InWork);

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(EditAnywhere, Category = "Person")
	float WalkSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Person")
	float BuildingRadius = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Person")
	float WaitTime = 2.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACityBuilding> Home;

	UPROPERTY(Transient)
	TObjectPtr<ACityBuilding> Work;

	UPROPERTY(Transient)
	TObjectPtr<ACityBuilding> TargetBuilding;

	float WaitRemaining = 0.0f;
	bool bWaiting = false;



};
