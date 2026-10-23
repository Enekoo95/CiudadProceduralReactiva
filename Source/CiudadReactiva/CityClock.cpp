// Fill out your copyright notice in the Description page of Project Settings.


#include "CityClock.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"

// Sets default values
ACityClock::ACityClock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

}

void ACityClock::Configure(float InDayDuration, float InStartHour)
{
	DayDuration = FMath::Max(1.0f, InDayDuration);
	StartHour = FMath::Clamp(InStartHour, 0.0f, 24.0f);
	Hour = StartHour;
	Day = 1;
}

void ACityClock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float HourPerSecond = 24.0f / FMath::Max(DayDuration, 1.0f);
	Hour += HourPerSecond * DeltaTime;
	while (Hour >= 24.0f)
	{
		Hour -= 24.0f;
		Day++;
	}

	if (bShowClock && GEngine)
	{
		const int32 H = FMath::FloorToInt(Hour);
		const int32 M = FMath::FloorToInt((Hour - H) * 60.0f);
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("Day: %d, Time: %02d:%02d"), Day, H, M));
	}
}

// Called when the game starts or when spawned
void ACityClock::BeginPlay()
{
	Super::BeginPlay();
	
}


