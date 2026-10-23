// Fill out your copyright notice in the Description page of Project Settings.


#include "CityScene.h"
#include "CityBuilding.h"
#include "CityPerson.h"
#include "CityClock.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"



// Sets default values
ACityScene::ACityScene()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	BuildingClass = ACityBuilding::StaticClass();
	PersonClass = ACityPerson::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
	SetRootComponent(Ground);
	if (CubeMesh.Succeeded())
	{
		Ground->SetStaticMesh(CubeMesh.Object);
	}
}

void ACityScene::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float Side = AreaExtent * 2.f;
	Ground->SetRelativeScale3D(FVector(Side / 100.f, Side / 100.f, 0.1f));
	Ground->SetRelativeLocation(FVector(0.f, 0.f, -5.f));
}


// Called when the game starts or when spawned
void ACityScene::BeginPlay()
{
	Super::BeginPlay();
	
	if (bAutoStart)
	{
		StartScene();
	}
}
void ACityScene::ClearScene()
{
	if (Home)
	{
		Home->Destroy();
		Home = nullptr;
	}
	if (Work)
	{
		Work->Destroy();
		Work = nullptr;
	}
	if (Person)
	{
		Person->Destroy();
		Person = nullptr;
	}
	if (Clock)
	{
		Clock->Destroy();
		Clock = nullptr;
	}
}

void ACityScene::StartScene()
{
	ClearScene();

	UWorld* World = GetWorld();
	if (!World || !BuildingClass)
	{
		return;
	}

	if (bRandomizeSeed)
	{
		Seed = FMath::RandRange(0, MAX_int32 - 1);
	}
	FRandomStream Rng(Seed);

	const FVector Origin = GetActorLocation();
	const float Range = AreaExtent * 0.8f;
	auto RandomPoint = [&]()
		{
			return Origin + FVector(Rng.FRandRange(-Range, Range), Rng.FRandRange(-Range, Range), 0.f);
		};

	const FVector HomePos = RandomPoint();
	FVector WorkPos = RandomPoint();
	for (int32 Attempt = 0; Attempt < 50 && FVector::Dist2D(HomePos, WorkPos) < MinDistanceBetweenBuildings; ++Attempt)
	{
		WorkPos = RandomPoint();
	}

	const FRotator HomeRot(0.f, (WorkPos - HomePos).Rotation().Yaw, 0.f);
	const FRotator WorkRot(0.f, (HomePos - WorkPos).Rotation().Yaw, 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Home = World->SpawnActor<ACityBuilding>(BuildingClass, HomePos, HomeRot, Params);
	Work = World->SpawnActor<ACityBuilding>(BuildingClass, WorkPos, WorkRot, Params);

	if (Home) { Home->SetBuildingType(EBuildingType::Home); }
	if (Work) { Work->SetBuildingType(EBuildingType::Work); }
	Clock = World->SpawnActor<ACityClock>(ACityClock::StaticClass(), Origin, FRotator::ZeroRotator, Params);
	if (Clock)
	{
		Clock->Configure(DayDuration, StartHour);
	}

	if (Home && Work && Clock && PersonClass)
	{
		const FVector Start = Home->GetEntranceLocation() + FVector(0.f, 0.f, 100.f);
		Person = World->SpawnActor<ACityPerson>(PersonClass, Start, FRotator::ZeroRotator, Params);

		if (Person) 
		{
			Person->SetRoute(Home, Work, Clock);
		}
	}
}

// Called every frame
void ACityScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

