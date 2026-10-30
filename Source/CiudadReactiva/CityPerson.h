// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CityPerson.generated.h"

class ACityBuilding;
class UStaticMeshComponent;
class ACityClock;



UCLASS()
class CIUDADREACTIVA_API ACityPerson : public ACharacter
{
	GENERATED_BODY()

public:

	ACityPerson();

	void SetRoute(ACityBuilding* InHome, ACityBuilding* InWork, ACityClock* InClock);

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

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "0", ClampMax = "24"))
	float WorkStartTime = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "0", ClampMax = "24"))
	float WorkEndTime = 17.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<ACityBuilding> Home;

	UPROPERTY(Transient)
	TObjectPtr<ACityBuilding> Work;

	UPROPERTY(Transient)
	TObjectPtr<ACityClock> Clock;

};
