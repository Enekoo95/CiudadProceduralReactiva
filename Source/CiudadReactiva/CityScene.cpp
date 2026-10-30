#include "CityScene.h"

#include "CityBuilding.h"
#include "CityClock.h"
#include "CityPerson.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float SlabHeight = 10.f;
}

ACityScene::ACityScene()
{
	PrimaryActorTick.bCanEverTick = false;

	BuildingClass = ACityBuilding::StaticClass();
	PersonClass = ACityPerson::StaticClass();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(TEXT("/Game/Materials/M_Building.M_Building"));

	Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
	Ground->SetupAttachment(Root);

	BlockSlabs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BlockSlabs"));
	BlockSlabs->SetupAttachment(Root);

	if (CubeMesh.Succeeded())
	{
		Ground->SetStaticMesh(CubeMesh.Object);
		BlockSlabs->SetStaticMesh(CubeMesh.Object);
	}
	if (BaseMaterial.Succeeded())
	{
		Ground->SetMaterial(0, BaseMaterial.Object);
		BlockSlabs->SetMaterial(0, BaseMaterial.Object);
	}
}

FVector2D ACityScene::GetTotalSize() const
{
	const float Pitch = BlockSize + StreetWidth;
	return FVector2D(BlocksX * Pitch + StreetWidth, BlocksY * Pitch + StreetWidth);
}

FVector2D ACityScene::GetBlockCenter(int32 X, int32 Y) const
{
	const float Pitch = BlockSize + StreetWidth;
	const FVector2D Total = GetTotalSize();

	return FVector2D(
		-Total.X * 0.5f + StreetWidth + BlockSize * 0.5f + X * Pitch,
		-Total.Y * 0.5f + StreetWidth + BlockSize * 0.5f + Y * Pitch);
}

void ACityScene::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SetActorScale3D(FVector::OneVector);

	const FVector2D Total = GetTotalSize();
	Ground->SetRelativeScale3D(FVector(Total.X / 100.f, Total.Y / 100.f, 1.0f));
	Ground->SetRelativeLocation(FVector(0.f, 0.f, -50.f));   

	BlockSlabs->SetRelativeScale3D(FVector::OneVector);
	BlockSlabs->ClearInstances();

	BlockSlabs->ClearInstances();
	for (int32 X = 0; X < BlocksX; ++X)
	{
		for (int32 Y = 0; Y < BlocksY; ++Y)
		{
			const FVector2D C = GetBlockCenter(X, Y);
			BlockSlabs->AddInstance(FTransform(
				FRotator::ZeroRotator,
				FVector(C.X, C.Y, SlabHeight * 0.5f),
				FVector(BlockSize / 100.f, BlockSize / 100.f, SlabHeight / 100.f)));
		}
	}

	if (UMaterialInstanceDynamic* StreetMID = Ground->CreateAndSetMaterialInstanceDynamic(0))
	{
		StreetMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.02f, 0.02f));
	}
	if (UMaterialInstanceDynamic* BlockMID = BlockSlabs->CreateAndSetMaterialInstanceDynamic(0))
	{
		BlockMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.25f, 0.25f, 0.25f));
	}
}

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
	for (ACityPerson* Person : SpawnedPeople)
	{
		if (Person) { Person->Destroy(); }
	}
	SpawnedPeople.Empty();

	for (ACityBuilding* Building : SpawnedBuildings)
	{
		if (Building) { Building->Destroy(); }
	}
	SpawnedBuildings.Empty();

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
	if (!World || !BuildingClass || !PersonClass)
	{
		return;
	}

	if (bRandomizeSeed)
	{
		Seed = FMath::RandRange(0, MAX_int32 - 1);
	}
	FRandomStream Rng(Seed);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 X = 0; X < BlocksX; ++X)
	{
		for (int32 Y = 0; Y < BlocksY; ++Y)
		{
			const FVector2D Center = GetBlockCenter(X, Y);

			if (Rng.FRand() < ParkChance)
			{
				SpawnPark(Center);
			}
			else
			{
				SpawnBuildingsInBlock(Center, Rng, Params);
			}
		}
	}

	const int32 NumBuildings = SpawnedBuildings.Num();
	if (NumBuildings < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("CityScene: hacen falta al menos 2 edificios (casa y trabajo)."));
		return;
	}

	for (int32 I = NumBuildings - 1; I > 0; --I)
	{
		SpawnedBuildings.Swap(I, Rng.RandRange(0, I));
	}

	const int32 WorkCount = FMath::Clamp(FMath::RoundToInt(NumBuildings * WorkRatio), 1, NumBuildings - 1);

	TArray<ACityBuilding*> Homes;
	TArray<ACityBuilding*> Works;
	for (int32 I = 0; I < NumBuildings; ++I)
	{
		ACityBuilding* Building = SpawnedBuildings[I];
		if (I < WorkCount)
		{
			Building->SetBuildingType(EBuildingType::Work);
			Works.Add(Building);
		}
		else
		{
			Building->SetBuildingType(EBuildingType::Home);
			Homes.Add(Building);
		}
	}

	Clock = World->SpawnActor<ACityClock>(ACityClock::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params);
	if (Clock)
	{
		Clock->Configure(DayDuration, StartHour);
	}

	if (!Clock)
	{
		return;
	}

	const int32 NumCitizens = FMath::Min(Homes.Num(), MaxCitizens);
	for (int32 I = 0; I < NumCitizens; ++I)
	{
		ACityBuilding* Home = Homes[I];
		ACityBuilding* Work = Works[Rng.RandRange(0, Works.Num() - 1)];

		const FVector Start = Home->GetEntranceLocation() + FVector(0.f, 0.f, 100.f);
		ACityPerson* Person = World->SpawnActor<ACityPerson>(PersonClass, Start, FRotator::ZeroRotator, Params);
		if (Person)
		{
			Person->SetRoute(Home, Work, Clock);
			SpawnedPeople.Add(Person);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CityScene: %d edificios (%d casas, %d trabajos), %d ciudadanos. Seed=%d"),
		NumBuildings, Homes.Num(), Works.Num(), SpawnedPeople.Num(), Seed);
}

void ACityScene::SpawnBuildingsInBlock(const FVector2D& Center, FRandomStream& Rng, const FActorSpawnParameters& Params)
{
	UWorld* World = GetWorld();
	const FTransform SceneTransform(GetActorRotation(), GetActorLocation(), FVector::OneVector);	
	const float Lot = BlockSize / LotsPerSide;

	for (int32 I = 0; I < LotsPerSide; ++I)
	{
		for (int32 J = 0; J < LotsPerSide; ++J)
		{
			const float OffsetX = (I + 0.5f) * Lot - BlockSize * 0.5f;
			const float OffsetY = (J + 0.5f) * Lot - BlockSize * 0.5f;

			float Yaw = 0.f;
			if (FMath::Abs(OffsetX) >= FMath::Abs(OffsetY))
			{
				Yaw = (OffsetX >= 0.f) ? 0.f : 180.f;
			}
			else
			{
				Yaw = (OffsetY >= 0.f) ? 90.f : -90.f;
			}

			const FTransform LocalTransform(
				FRotator(0.f, Yaw, 0.f),
				FVector(Center.X + OffsetX, Center.Y + OffsetY, SlabHeight),
				FVector::OneVector);
			const FTransform WorldTransform = LocalTransform * SceneTransform;

			ACityBuilding* Building = World->SpawnActor<ACityBuilding>(BuildingClass, WorldTransform, Params);
			if (Building)
			{
				const float Depth = Lot * Rng.FRandRange(MinFootprint, MaxFootprint);
				const float Width = Lot * Rng.FRandRange(MinFootprint, MaxFootprint);
				const float Height = Lot * Rng.FRandRange(MinHeightRatio, MaxHeightRatio);
				Building->SetSize(FVector(Depth, Width, Height));

				SpawnedBuildings.Add(Building);
			}
		}
	}
}


void ACityScene::SpawnPark(const FVector2D& Center)
{
}