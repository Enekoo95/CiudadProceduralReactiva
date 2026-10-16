// Fill out your copyright notice in the Description page of Project Settings.


#include "CityBuilding.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
ACityBuilding::ACityBuilding()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	if(CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BuildingMaterial(TEXT("/Game/Materials/M_Building.M_Building"));
	if (BuildingMaterial.Succeeded()) 
	{
		Mesh->SetMaterial(0, BuildingMaterial.Object);
	}


	Mesh->SetRelativeScale3D(FVector(3.0f, 3.0f, 5.0f));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));

	Entrance = CreateDefaultSubobject<USceneComponent>(TEXT("Entrance"));
	Entrance->SetupAttachment(Root);
	Entrance->SetRelativeLocation(FVector(250.0f, 0.0f, 0.0f));
}


void ACityBuilding::SetBuildingType(EBuildingType NewType)
{
	BuildingType = NewType;
	const bool bIsHome = (BuildingType == EBuildingType::Home);

	if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), bIsHome ? FLinearColor::Blue : FLinearColor::Green);
	}

	DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 600.f),
	bIsHome ? TEXT("CASA") : TEXT("TRABAJO"), nullptr, FColor::White, 100000.f, false, 2.f);
}

FVector ACityBuilding::GetEntranceLocation() const
{
	return Entrance->GetComponentLocation();
}

void ACityBuilding::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACityBuilding::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

