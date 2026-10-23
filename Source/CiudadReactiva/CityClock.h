// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityClock.generated.h"

class USceneComponent;

UCLASS()
class CIUDADREACTIVA_API ACityClock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACityClock();

	void Configure( float DayDuration, float StartHour);

	float getHour() const { return Hour; }

	int32 getDay() const { return Day; }

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, Category = "Clock")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "1"))
	float DayDuration = 240.0f;

	UPROPERTY(EditAnywhere, Category = "Clock", meta = (ClampMin = "0", ClampMax = "24"))
	float StartHour = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Clock")
	bool bShowClock = true;

private: 
	float Hour = 0.0f;
	int32 Day = 1;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
