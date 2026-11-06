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
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float SlabHeight = 10.f;
	constexpr int32 MaxLotsPerAxis = 6;
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

	ParkSlabs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ParkSlabs"));
	ParkSlabs->SetupAttachment(Root);

	if (CubeMesh.Succeeded())
	{
		Ground->SetStaticMesh(CubeMesh.Object);
		BlockSlabs->SetStaticMesh(CubeMesh.Object);
		ParkSlabs->SetStaticMesh(CubeMesh.Object);
	}
	if (BaseMaterial.Succeeded())
	{
		Ground->SetMaterial(0, BaseMaterial.Object);
		BlockSlabs->SetMaterial(0, BaseMaterial.Object);
		ParkSlabs->SetMaterial(0, BaseMaterial.Object);
	}
}

float ACityScene::Perlin01(float X, float Y, float Scale) const
{
	const float Offset = (Seed % 1000) * 17.31;
	const FVector2D P((X + 0.37f) * Scale + Offset, (Y + 0.37f) * Scale + Offset * 0.73f);
	return FMath::Clamp((FMath::PerlinNoise2D(P) + 1.f) * 0.5f, 0.f, 1.f);
}

float ACityScene::GetBlockExtent(int32 Index, int32 Axis) const
{
	float N;

	if (bUsePerlinNoise) 
	{
		const float Raw = Perlin01(Index, Axis * 37.f, BlockSizeNoiseScale);
		N = FMath::Clamp((Raw - 0.25f) / 0.5f, 0.f, 1.f);
	}
	else
	{
		const int32 Hash = static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(Index * 2 + Axis)));
		N = FRandomStream(Hash).FRand();
	}
	return FMath::Lerp(MinBlockSize, MaxBlockSize, N);
}

//Parques
bool ACityScene::IsParkBlock(int32 X, int32 Y) const
{
	if (bUsePerlinNoise)
	{
		return Perlin01(X, Y, ParkNoiseScale) < ParkThreshold;
	}
	const int32 Hash = static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(X * 1000 + Y)));
	return FRandomStream(Hash).FRand() < ParkChance;
}

//Layout
void ACityScene::BuildLayoutCache()
{
	ColSize.SetNum(BlocksX);
	ColStart.SetNum(BlocksX);
	RowSize.SetNum(BlocksY);
	RowStart.SetNum(BlocksY);

	float SumX = 0.f;
	for (int32 X = 0; X < BlocksX; ++X)
	{
		ColSize[X] = GetBlockExtent(X, 0);
		SumX += ColSize[X];
	}
	float SumY = 0.f;
	for (int32 Y = 0; Y < BlocksY; ++Y)
	{
		RowSize[Y] = GetBlockExtent(Y, 1);
		SumY += RowSize[Y];
	}

	TotalSize.X = SumX + StreetWidth * (BlocksX + 1);
	TotalSize.Y = SumY + StreetWidth * (BlocksY + 1);

	float Pos = -TotalSize.X * 0.5f + StreetWidth;
	for (int32 X = 0; X < BlocksX; ++X)
	{
		ColStart[X] = Pos;
		Pos += ColSize[X] + StreetWidth;
	}
	Pos = -TotalSize.Y * 0.5f + StreetWidth;
	for (int32 Y = 0; Y < BlocksY; ++Y)
	{
		RowStart[Y] = Pos;
		Pos += RowSize[Y] + StreetWidth;
	}
}

FVector2D ACityScene::GetBlockCenter(int32 X, int32 Y) const
{
	return FVector2D(ColStart[X] + ColSize[X] * 0.5f, RowStart[Y] + RowSize[Y] * 0.5f);
}

FVector2D ACityScene::GetBlockSize(int32 X, int32 Y) const
{
	return FVector2D(ColSize[X], RowSize[Y]);
}
FName ACityScene::GetOwnTag() const
{
	return FName(*FString::Printf(TEXT("CityOf_%s"), *GetName()));
}

void ACityScene::RebuildGround()
{
	BuildLayoutCache();

	SetActorScale3D(FVector::OneVector);

	Ground->SetRelativeScale3D(FVector(TotalSize.X / 100.f, TotalSize.Y / 100.f, 1.0f));
	Ground->SetRelativeLocation(FVector(0.f, 0.f, -50.f));

	BlockSlabs->SetRelativeScale3D(FVector::OneVector);
	ParkSlabs->SetRelativeScale3D(FVector::OneVector);
	BlockSlabs->ClearInstances();
	ParkSlabs->ClearInstances();

	for (int32 X = 0; X < BlocksX; ++X)
	{
		for (int32 Y = 0; Y < BlocksY; ++Y)
		{
			const FVector2D C = GetBlockCenter(X, Y);
			const FVector2D S = GetBlockSize(X, Y);

			UInstancedStaticMeshComponent* Target = IsParkBlock(X, Y) ? ParkSlabs.Get() : BlockSlabs.Get();
			Target->AddInstance(FTransform(
				FRotator::ZeroRotator,
				FVector(C.X, C.Y, SlabHeight * 0.5f),
				FVector(S.X / 100.f, S.Y / 100.f, SlabHeight / 100.f)));
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
	if (UMaterialInstanceDynamic* ParkMID = ParkSlabs->CreateAndSetMaterialInstanceDynamic(0))
	{
		ParkMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.35f, 0.07f));
	}
}

void ACityScene::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildGround();
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
	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> ToDestroy;
		UGameplayStatics::GetAllActorsWithTag(World, GetOwnTag(), ToDestroy);
		for (AActor* Actor : ToDestroy)
		{
			if (Actor) { Actor->Destroy(); }
		}
	}

	SpawnedPeople.Empty();
	SpawnedBuildings.Empty();
	Clock = nullptr;
}

void ACityScene::BuildLayout(FRandomStream& Rng)
{
	UWorld* World = GetWorld();
	if (!World || !BuildingClass)
	{
		return;
	}

	RebuildGround();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 X = 0; X < BlocksX; ++X)
	{
		for (int32 Y = 0; Y < BlocksY; ++Y)
		{
			if (!IsParkBlock(X, Y))
			{
				SpawnBuildingsInBlock(X, Y, Rng, Params);
			}
		}
	}
}

void ACityScene::GenerateLayout()
{
	ClearScene();

	if (!GetWorld() || !BuildingClass)
	{
		return;
	}

	if (bRandomizeSeed)
	{
		Seed = FMath::RandRange(0, MAX_int32 - 1);
	}
	FRandomStream Rng(Seed);
	BuildLayout(Rng);

	UE_LOG(LogTemp, Log, TEXT("CityScene: layout generado, %d edificios. Seed=%d"), SpawnedBuildings.Num(), Seed);
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

	BuildLayout(Rng);

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
	if (!Clock)
	{
		return;
	}
	Clock->Tags.Add(GetOwnTag());
	Clock->Configure(DayDuration, StartHour);

	const int32 NumCitizens = FMath::Min(Homes.Num(), MaxCitizens);
	for (int32 I = 0; I < NumCitizens; ++I)
	{
		ACityBuilding* Home = Homes[I];
		ACityBuilding* Work = Works[Rng.RandRange(0, Works.Num() - 1)];

		const FVector Start = Home->GetEntranceLocation() + FVector(0.f, 0.f, 100.f);
		ACityPerson* Person = World->SpawnActor<ACityPerson>(PersonClass, Start, FRotator::ZeroRotator, Params);
		if (Person)
		{
			Person->Tags.Add(GetOwnTag());
			Person->SetRoute(Home, Work, Clock);
			SpawnedPeople.Add(Person);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CityScene: %d edificios (%d casas, %d trabajos), %d ciudadanos. Seed=%d"),
		NumBuildings, Homes.Num(), Works.Num(), SpawnedPeople.Num(), Seed);
}

//Edificios
void ACityScene::SpawnBuildingsInBlock(int32 BlockX, int32 BlockY, FRandomStream& Rng, const FActorSpawnParameters& Params)
{
	UWorld* World = GetWorld();
	const FTransform SceneTransform(GetActorRotation(), GetActorLocation(), FVector::OneVector);

	const FVector2D Center = GetBlockCenter(BlockX, BlockY);
	const FVector2D Size = GetBlockSize(BlockX, BlockY);

	const int32 LotsX = FMath::Clamp(FMath::RoundToInt(Size.X / TargetLotSize), 1, MaxLotsPerAxis);
	const int32 LotsY = FMath::Clamp(FMath::RoundToInt(Size.Y / TargetLotSize), 1, MaxLotsPerAxis);
	const float LotW = Size.X / LotsX;
	const float LotD = Size.Y / LotsY;
	const float RefLot = (LotW + LotD) * 0.5f;

	for (int32 I = 0; I < LotsX; ++I)
	{
		for (int32 J = 0; J < LotsY; ++J)
		{
			const float OffsetX = (I + 0.5f) * LotW - Size.X * 0.5f;
			const float OffsetY = (J + 0.5f) * LotD - Size.Y * 0.5f;

			float Yaw;
			bool bFaceX;
			if (FMath::Abs(OffsetX) / Size.X >= FMath::Abs(OffsetY) / Size.Y)
			{
				Yaw = (OffsetX >= 0.f) ? 0.f : 180.f;
				bFaceX = true;
			}
			else
			{
				Yaw = (OffsetY >= 0.f) ? 90.f : -90.f;
				bFaceX = false;
			}

			const FTransform LocalTransform(
				FRotator(0.f, Yaw, 0.f),
				FVector(Center.X + OffsetX, Center.Y + OffsetY, SlabHeight),
				FVector::OneVector);
			const FTransform WorldTransform = LocalTransform * SceneTransform;

			ACityBuilding* Building = World->SpawnActor<ACityBuilding>(BuildingClass, WorldTransform, Params);
			if (!Building)
			{
				continue;
			}

			const float DepthLot = bFaceX ? LotW : LotD;
			const float WidthLot = bFaceX ? LotD : LotW;
			const float Depth = DepthLot * Rng.FRandRange(MinFootprint, MaxFootprint);
			const float Width = WidthLot * Rng.FRandRange(MinFootprint, MaxFootprint);

			float HeightRatio;
			if (bUsePerlinNoise)
			{
				const float N = Perlin01((Center.X + OffsetX) / TargetLotSize,
					(Center.Y + OffsetY) / TargetLotSize, HeightNoiseScale);
				HeightRatio = FMath::Lerp(MinHeightRatio, MaxHeightRatio, N);
			}
			else
			{
				HeightRatio = Rng.FRandRange(MinHeightRatio, MaxHeightRatio);
			}

			Building->SetSize(FVector(Depth, Width, RefLot * HeightRatio));
			Building->Tags.Add(GetOwnTag());
#if WITH_EDITOR
			Building->SetFolderPath(TEXT("City"));
#endif
			SpawnedBuildings.Add(Building);
		}
	}
}