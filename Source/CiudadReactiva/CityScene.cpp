#include "CityScene.h"

#include "CityBuilding.h"
#include "CityClock.h"
#include "CityPerson.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float SlabHeight = 10.f;
	constexpr int32 MaxLotsPerAxis = 6;

	// Celda fija para muestrear el ruido de altura: asi el tamaño de parcela no cambia el patron de alturas.
	constexpr float HeightNoiseCell = 500.f;

	// Crea el MID una sola vez y lo reutiliza; si el slot de material cambio, lo recrea.
	void ApplyColor(TObjectPtr<UMaterialInstanceDynamic>& MID, UPrimitiveComponent* Comp, const FLinearColor& Color)
	{
		if (!Comp)
		{
			return;
		}
		if (!MID || Comp->GetMaterial(0) != MID.Get())
		{
			MID = Comp->CreateAndSetMaterialInstanceDynamic(0);
		}
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
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

// ---------------------------------------------------------------------------
// Ruido / aleatoriedad
// ---------------------------------------------------------------------------

float ACityScene::Perlin01(float X, float Y, float Scale) const
{
	const float Offset = (Seed % 1000) * 17.31f;
	const FVector2D P((X + 0.37f) * Scale + Offset, (Y + 0.37f) * Scale + Offset * 0.73f);
	return FMath::Clamp((FMath::PerlinNoise2D(P) + 1.f) * 0.5f, 0.f, 1.f);
}

float ACityScene::NoiseNorm(float X, float Y, float Scale) const
{
	return FMath::Clamp((Perlin01(X, Y, Scale) - 0.25f) / 0.5f, 0.f, 1.f);
}

float ACityScene::GetBlockExtent(int32 Index, int32 Axis) const
{
	float N;

	if (bUsePerlinNoise)
	{
		N = NoiseNorm(Index, Axis * 37.f, BlockSizeNoiseScale);
	}
	else
	{
		const int32 Hash = static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(Index * 2 + Axis)));
		N = FRandomStream(Hash).FRand();
	}
	return FMath::Lerp(MinBlockSize, MaxBlockSize, N);
}

// Parques
bool ACityScene::IsParkBlock(int32 X, int32 Y) const
{
	if (bUsePerlinNoise)
	{
		if (ParkThreshold >= 1.f)
		{
			return true;
		}
		return NoiseNorm(X, Y, ParkNoiseScale) < ParkThreshold;
	}
	const int32 Hash = static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(X * 1000 + Y)));
	return FRandomStream(Hash).FRand() < ParkChance;
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void ACityScene::SanitizeParams()
{
	BlocksX = FMath::Clamp(BlocksX, 1, 8);
	BlocksY = FMath::Clamp(BlocksY, 1, 8);

	StreetWidth = FMath::Max(StreetWidth, 200.f);
	TargetLotSize = FMath::Max(TargetLotSize, 200.f);

	MinBlockSize = FMath::Max(MinBlockSize, 200.f);
	MaxBlockSize = FMath::Max(MaxBlockSize, MinBlockSize);

	MinFootprint = FMath::Clamp(MinFootprint, 0.2f, 0.9f);
	MaxFootprint = FMath::Clamp(MaxFootprint, MinFootprint, 0.9f);

	MinBuildingHeight = FMath::Max(MinBuildingHeight, 100.f);
	MaxBuildingHeight = FMath::Max(MaxBuildingHeight, MinBuildingHeight);

	BlockSizeNoiseScale = FMath::Clamp(BlockSizeNoiseScale, 0.01f, 2.f);
	ParkNoiseScale = FMath::Clamp(ParkNoiseScale, 0.01f, 1.f);
	HeightNoiseScale = FMath::Clamp(HeightNoiseScale, 0.01f, 1.f);
	ParkThreshold = FMath::Clamp(ParkThreshold, 0.f, 1.f);
	ParkChance = FMath::Clamp(ParkChance, 0.f, 1.f);
}

void ACityScene::BuildLayoutCache()
{
	SanitizeParams();

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

	ApplyColor(GroundMID, Ground.Get(), FLinearColor(0.02f, 0.02f, 0.02f));
	ApplyColor(BlockMID, BlockSlabs.Get(), FLinearColor(0.25f, 0.25f, 0.25f));
	ApplyColor(ParkMID, ParkSlabs.Get(), FLinearColor(0.05f, 0.35f, 0.07f));
}

void ACityScene::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildGround();
}

#if WITH_EDITOR
void ACityScene::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Mientras se arrastra un slider solo se actualiza el suelo (OnConstruction); al soltar se regeneran los edificios.
	if (!bAutoRegenerateInEditor || PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (World && World->WorldType == EWorldType::Editor)
	{
		RegenerateLayout();
	}
}
#endif

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

void ACityScene::BuildLayout()
{
	UWorld* World = GetWorld();
	if (!World || !BuildingClass)
	{
		return;
	}

	// Necesario aqui: desde el widget los parametros cambian sin pasar por OnConstruction.
	RebuildGround();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient; // no se guardan con el nivel

	for (int32 X = 0; X < BlocksX; ++X)
	{
		for (int32 Y = 0; Y < BlocksY; ++Y)
		{
			if (!IsParkBlock(X, Y))
			{
				SpawnBuildingsInBlock(X, Y, Params);
			}
		}
	}
}

void ACityScene::GenerateLayoutInternal(bool bNewSeed)
{
	ClearScene();

	if (!GetWorld() || !BuildingClass)
	{
		return;
	}

	if (bNewSeed && bRandomizeSeed)
	{
		Seed = FMath::RandRange(0, MAX_int32 - 1);
	}
	BuildLayout();

	UE_LOG(LogTemp, Log, TEXT("CityScene: layout generado, %d edificios. Seed=%d Bloques=%dx%d Calle=%.0f Lote=%.0f Manzana=%.0f-%.0f"),
		SpawnedBuildings.Num(), Seed, BlocksX, BlocksY, StreetWidth, TargetLotSize, MinBlockSize, MaxBlockSize);
}

void ACityScene::GenerateLayout()
{
	GenerateLayoutInternal(true);
}

void ACityScene::RegenerateLayout()
{
	GenerateLayoutInternal(false);
}

void ACityScene::ApplyParamsAndRegenerate(
	int32 InBlocksX, int32 InBlocksY,
	float InStreetWidth,
	float InMinBlockSize, float InMaxBlockSize,
	float InLotSize,
	float InBlockNoise, float InParkNoise, float InParkThreshold, float InHeightNoise)
{
	BlocksX = InBlocksX;
	BlocksY = InBlocksY;
	StreetWidth = InStreetWidth;
	MinBlockSize = InMinBlockSize;
	MaxBlockSize = InMaxBlockSize;
	TargetLotSize = InLotSize;
	BlockSizeNoiseScale = InBlockNoise;
	ParkNoiseScale = InParkNoise;
	ParkThreshold = InParkThreshold;
	HeightNoiseScale = InHeightNoise;

	RegenerateLayout();
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
	Params.ObjectFlags |= RF_Transient;

	BuildLayout();

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

// ---------------------------------------------------------------------------
// Edificios
// ---------------------------------------------------------------------------

void ACityScene::SpawnBuildingsInBlock(int32 BlockX, int32 BlockY, const FActorSpawnParameters& Params)
{
	UWorld* World = GetWorld();
	const FTransform SceneTransform(GetActorRotation(), GetActorLocation(), FVector::OneVector);

	const FVector2D Center = GetBlockCenter(BlockX, BlockY);
	const FVector2D Size = GetBlockSize(BlockX, BlockY);

	const int32 LotsX = FMath::Clamp(FMath::RoundToInt(Size.X / TargetLotSize), 1, MaxLotsPerAxis);
	const int32 LotsY = FMath::Clamp(FMath::RoundToInt(Size.Y / TargetLotSize), 1, MaxLotsPerAxis);
	const float LotW = Size.X / LotsX;
	const float LotD = Size.Y / LotsY;

	const uint32 BlockHash = HashCombine(GetTypeHash(Seed), GetTypeHash(BlockX * 100 + BlockY));

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

			// Aleatoriedad propia de cada parcela: cambiar un parametro no re-randomiza el resto de edificios.
			FRandomStream LotRng(static_cast<int32>(HashCombine(BlockHash, GetTypeHash(I * 10 + J))));

			const float DepthLot = bFaceX ? LotW : LotD;
			const float WidthLot = bFaceX ? LotD : LotW;
			const float Depth = DepthLot * LotRng.FRandRange(MinFootprint, MaxFootprint);
			const float Width = WidthLot * LotRng.FRandRange(MinFootprint, MaxFootprint);

			float HeightAlpha;
			if (bUsePerlinNoise)
			{
				HeightAlpha = NoiseNorm((Center.X + OffsetX) / HeightNoiseCell,
					(Center.Y + OffsetY) / HeightNoiseCell, HeightNoiseScale);
			}
			else
			{
				HeightAlpha = LotRng.FRand();
			}
			const float Height = FMath::Lerp(MinBuildingHeight, MaxBuildingHeight, HeightAlpha);

			Building->SetSize(FVector(Depth, Width, Height));
			Building->Tags.Add(GetOwnTag());
#if WITH_EDITOR
			Building->SetFolderPath(TEXT("City"));
#endif
			SpawnedBuildings.Add(Building);
		}
	}
}