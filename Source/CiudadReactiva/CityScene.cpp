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
#include "MaterialShared.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Math/RandomStream.h"

namespace
{
	constexpr float SlabHeight = 10.f;

	// Reparte un rectangulo en baldosas para que la textura se repita (el cubo solo tiene un UV 0-1 por cara).
	void AddTiledSlab(UInstancedStaticMeshComponent* Target, const FVector2D& Center, const FVector2D& Size, float Bottom, float Height, float Tile)
	{
		const int32 NX = FMath::Max(1, FMath::RoundToInt(Size.X / Tile));
		const int32 NY = FMath::Max(1, FMath::RoundToInt(Size.Y / Tile));
		const float TX = Size.X / NX;
		const float TY = Size.Y / NY;
		for (int32 IX = 0; IX < NX; ++IX)
		{
			for (int32 IY = 0; IY < NY; ++IY)
			{
				const FVector Loc(Center.X - Size.X * 0.5f + TX * (IX + 0.5f), Center.Y - Size.Y * 0.5f + TY * (IY + 0.5f), Bottom + Height * 0.5f);
				Target->AddInstance(FTransform(FRotator::ZeroRotator, Loc, FVector(TX / 100.f, TY / 100.f, Height / 100.f)));
			}
		}
	}

	void EnsureInstancedUsage(UMaterialInterface* M)
	{
		if (M)
		{
			M->CheckMaterialUsage(MATUSAGE_InstancedStaticMeshes);
		}
	}


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

	RoadTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("RoadTiles"));
	RoadTiles->SetupAttachment(Root);

	PCG = CreateDefaultSubobject<UPCGComponent>(TEXT("PCG"));
	PCG->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;

	// Assets de CARLA. FObjectFinderOptional no da error si todavia no existen en el proyecto.
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh1(TEXT("/Game/Carla/Static/Building/Houses/SM_House17_A.SM_House17_A"));
	if (UStaticMesh* M = Mesh1.Get()) { HouseMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh2(TEXT("/Game/Carla/Static/Building/Houses/SM_House17_B.SM_House17_B"));
	if (UStaticMesh* M = Mesh2.Get()) { HouseMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh3(TEXT("/Game/Carla/Static/Building/Houses/SM_House18_C.SM_House18_C"));
	if (UStaticMesh* M = Mesh3.Get()) { HouseMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh4(TEXT("/Game/Carla/Static/Building/Houses/SM_House18_E.SM_House18_E"));
	if (UStaticMesh* M = Mesh4.Get()) { HouseMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh5(TEXT("/Game/Carla/Static/Building/Buildings/BuildingComplete/SM_Apt32_A.SM_Apt32_A"));
	if (UStaticMesh* M = Mesh5.Get()) { ApartmentMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh7(TEXT("/Game/Carla/Static/Building/Buildings/BuildingComplete/SM_Apt33_A.SM_Apt33_A"));
	if (UStaticMesh* M = Mesh7.Get()) { ApartmentMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh8(TEXT("/Game/Carla/Static/Building/Buildings/BuildingComplete/SM_Apt34_A.SM_Apt34_A"));
	if (UStaticMesh* M = Mesh8.Get()) { ApartmentMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh9(TEXT("/Game/Carla/Static/Building/Buildings/BuildingComplete/SM_Apt35_A.SM_Apt35_A"));
	if (UStaticMesh* M = Mesh9.Get()) { ApartmentMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh10(TEXT("/Game/Carla/Static/Vegetation/Trees/SM_AcerSaccharum01_A.SM_AcerSaccharum01_A"));
	if (UStaticMesh* M = Mesh10.Get()) { TreeMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh11(TEXT("/Game/Carla/Static/Vegetation/Trees/SM_Cypress02_A.SM_Cypress02_A"));
	if (UStaticMesh* M = Mesh11.Get()) { TreeMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Mesh12(TEXT("/Game/Carla/Static/Vegetation/Trees/SM_Pine03_A.SM_Pine03_A"));
	if (UStaticMesh* M = Mesh12.Get()) { TreeMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> W2(TEXT("/Game/Carla/Static/Static/Town_C/Building/SM_SIAC_Building.SM_SIAC_Building"));
	if (UStaticMesh* M = W2.Get()) { WorkMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> W3(TEXT("/Game/Carla/Static/Static/Town_C/Building/SM_SorlHotel.SM_SorlHotel"));
	if (UStaticMesh* M = W3.Get()) { WorkMeshes.Add(M); }
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> RoadMat(TEXT("/Game/Carla/Static/Road/RoadsTown07/Asphalt1_Road.Asphalt1_Road"));
	RoadMaterial = RoadMat.Get();
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> SidewalkMat(TEXT("/Game/Carla/Static/SideWalk/M_SideWalk_d2.M_SideWalk_d2"));
	SidewalkMaterial = SidewalkMat.Get();
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> GrassMaterial(TEXT("/Game/Carla/Static/Road/RoadsTown10HD/Grass1_Ground.Grass1_Ground"));
	ParkMaterial = GrassMaterial.Get();

	if (CubeMesh.Succeeded())
	{
		Ground->SetStaticMesh(CubeMesh.Object);
		BlockSlabs->SetStaticMesh(CubeMesh.Object);
		ParkSlabs->SetStaticMesh(CubeMesh.Object);
		RoadTiles->SetStaticMesh(CubeMesh.Object);
	}
	if (BaseMaterial.Succeeded())
	{
		Ground->SetMaterial(0, BaseMaterial.Object);
		BlockSlabs->SetMaterial(0, BaseMaterial.Object);
		ParkSlabs->SetMaterial(0, BaseMaterial.Object);
	}
}

// ---------------------------------------------------------------------------
// Layout (la logica vive en el plugin EnekoMartinTools: FCityLayoutBuilder)
// ---------------------------------------------------------------------------

FCityLayoutParams ACityScene::GetCityLayoutParams() const
{
	FCityLayoutParams P;
	P.BlocksX = BlocksX;
	P.BlocksY = BlocksY;
	P.StreetWidth = StreetWidth;
	P.MinBlockSize = MinBlockSize;
	P.MaxBlockSize = MaxBlockSize;
	P.TargetLotSize = TargetLotSize;
	P.MinFootprint = MinFootprint;
	P.MaxFootprint = MaxFootprint;
	P.MinBuildingHeight = MinBuildingHeight;
	P.MaxBuildingHeight = MaxBuildingHeight;
	P.SurfaceHeight = (bUsePCGStreets && CityGraph) ? SidewalkHeight : SlabHeight;
	P.bUsePerlinNoise = bUsePerlinNoise;
	P.BlockSizeNoiseScale = BlockSizeNoiseScale;
	P.ParkNoiseScale = ParkNoiseScale;
	P.ParkThreshold = ParkThreshold;
	P.HeightNoiseScale = HeightNoiseScale;
	P.ParkChance = ParkChance;
	P.Seed = Seed;
	return P;
}

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

FName ACityScene::GetOwnTag() const
{
	return FName(*FString::Printf(TEXT("CityOf_%s"), *GetName()));
}

void ACityScene::RebuildGround()
{
	SanitizeParams();
	Layout = FCityLayoutBuilder::Build(GetCityLayoutParams());

	SetActorScale3D(FVector::OneVector);

	// El cubo gigante de antes solo sirve de base oscura; las calles se pintan con baldosas para que la textura se repita.
	Ground->SetRelativeScale3D(FVector(Layout.TotalSize.X / 100.f, Layout.TotalSize.Y / 100.f, 1.0f));
	Ground->SetRelativeLocation(FVector(0.f, 0.f, -52.f));

	BlockSlabs->SetRelativeScale3D(FVector::OneVector);
	ParkSlabs->SetRelativeScale3D(FVector::OneVector);
	RoadTiles->SetRelativeScale3D(FVector::OneVector);
	BlockSlabs->ClearInstances();
	ParkSlabs->ClearInstances();
	RoadTiles->ClearInstances();

	const float Tile = FMath::Max(GroundTileSize, 100.f);
	AddTiledSlab(RoadTiles, FVector2D(0.f, 0.f), FVector2D(Layout.TotalSize.X, Layout.TotalSize.Y), -42.f, 50.f, Tile);

	const bool bPCGSurfaces = bUsePCGStreets && CityGraph != nullptr;

	for (const FCityBlock& Block : Layout.Blocks)
	{
		// Con PCG las aceras de las manzanas edificables las genera el grafo; los parques siguen siendo losas.
		if (!Block.bPark && bPCGSurfaces)
		{
			continue;
		}

		UInstancedStaticMeshComponent* Target = Block.bPark ? ParkSlabs.Get() : BlockSlabs.Get();
		AddTiledSlab(Target, FVector2D(Block.Center.X, Block.Center.Y), FVector2D(Block.Size.X, Block.Size.Y), 0.f, SlabHeight, Tile);
	}

	ApplyColor(GroundMID, Ground.Get(), FLinearColor(0.02f, 0.02f, 0.02f));

	if (RoadMaterial)
	{
		EnsureInstancedUsage(RoadMaterial);
		RoadTiles->SetMaterial(0, RoadMaterial);
	}
	if (SidewalkMaterial)
	{
		EnsureInstancedUsage(SidewalkMaterial);
		BlockSlabs->SetMaterial(0, SidewalkMaterial);
	}
	else
	{
		ApplyColor(BlockMID, BlockSlabs.Get(), FLinearColor(0.25f, 0.25f, 0.25f));
	}
	if (ParkMaterial)
	{
		EnsureInstancedUsage(ParkMaterial);
		ParkSlabs->SetMaterial(0, ParkMaterial);
	}
	else
	{
		ApplyColor(ParkMID, ParkSlabs.Get(), FLinearColor(0.05f, 0.35f, 0.07f));
	}

	RebuildTrees();
}

void ACityScene::RebuildTrees()
{
	for (UInstancedStaticMeshComponent* Comp : TreeComponents)
	{
		if (Comp)
		{
			Comp->DestroyComponent();
		}
	}
	TreeComponents.Reset();

	TArray<UStaticMesh*> Meshes;
	for (UStaticMesh* M : TreeMeshes)
	{
		if (M)
		{
			Meshes.Add(M);
		}
	}
	if (Meshes.IsEmpty())
	{
		return;
	}

	for (UStaticMesh* M : Meshes)
	{
		UInstancedStaticMeshComponent* Comp = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		Comp->SetStaticMesh(M);
		Comp->SetupAttachment(Root);
		Comp->RegisterComponent();
		Comp->SetRelativeTransform(FTransform::Identity);
		TreeComponents.Add(Comp);
	}

	const float Spacing = FMath::Max(TreeSpacing, 100.f);
	for (const FCityBlock& Block : Layout.Blocks)
	{
		if (!Block.bPark)
		{
			continue;
		}

		FRandomStream Rng(static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(Block.X * 100 + Block.Y))));
		const float Margin = FMath::Min(150.f, FMath::Min(Block.Size.X, Block.Size.Y) * 0.2f);
		const float HalfX = Block.Size.X * 0.5f - Margin;
		const float HalfY = Block.Size.Y * 0.5f - Margin;
		if (HalfX <= 0.f || HalfY <= 0.f)
		{
			continue;
		}

		const int32 Count = FMath::Clamp(FMath::RoundToInt((Block.Size.X * Block.Size.Y) / (Spacing * Spacing)), 1, 40);
		for (int32 I = 0; I < Count; ++I)
		{
			const int32 MeshIndex = Rng.RandRange(0, TreeComponents.Num() - 1);
			const FVector Location(
				Block.Center.X + Rng.FRandRange(-HalfX, HalfX),
				Block.Center.Y + Rng.FRandRange(-HalfY, HalfY),
				SlabHeight);
			const UStaticMesh* TreeMesh = TreeComponents[MeshIndex]->GetStaticMesh();
			const float MeshHeight = TreeMesh ? FMath::Max(TreeMesh->GetBounds().BoxExtent.Z * 2.f, 1.f) : 1000.f;
			const float Scale = (FMath::Max(TreeHeight, 100.f) / MeshHeight) * Rng.FRandRange(0.8f, 1.2f);
			TreeComponents[MeshIndex]->AddInstance(
				FTransform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Location, FVector(Scale)));
		}
	}
}

bool ACityScene::IsWorkLot(int32 LotIndex) const
{
	return WorkLots.Contains(LotIndex);
}

UStaticMesh* ACityScene::PickBuildingMesh(const FCityLot& Lot, int32 LotIndex, bool bWork) const
{
	FRandomStream Rng(static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(LotIndex))));

	if (bWork)
	{
		return WorkMeshes[Rng.RandRange(0, WorkMeshes.Num() - 1)];
	}

	const float Range = FMath::Max(MaxBuildingHeight - MinBuildingHeight, 1.f);
	const float Alpha = (Lot.Height - MinBuildingHeight) / Range;
	const TArray<TObjectPtr<UStaticMesh>>& Pool =
		(Alpha >= ApartmentHeightThreshold && !ApartmentMeshes.IsEmpty()) ? ApartmentMeshes : HouseMeshes;
	const TArray<TObjectPtr<UStaticMesh>>& Fallback = ApartmentMeshes.IsEmpty() ? HouseMeshes : ApartmentMeshes;
	const TArray<TObjectPtr<UStaticMesh>>& Use = Pool.IsEmpty() ? Fallback : Pool;
	if (Use.IsEmpty())
	{
		return nullptr;
	}
	return Use[Rng.RandRange(0, Use.Num() - 1)];
}

void ACityScene::RebuildStreets()
{
	if (!PCG)
	{
		return;
	}

	if (!bUsePCGStreets || !CityGraph)
	{
		PCG->CleanupLocal(/*bRemoveComponents=*/true);
		return;
	}

	// El nodo City Layout lee los parametros de este actor (ICityLayoutSource) al ejecutarse.
	PCG->SetGraph(CityGraph);
	PCG->GenerateLocal(/*bForce=*/true);
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
	RebuildStreets();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient; // no se guardan con el nivel

	const FTransform SceneTransform(GetActorRotation(), GetActorLocation(), FVector::OneVector);

	// Mismas parcelas que recibe el grafo PCG por el pin Lots: calles, aceras y edificios no pueden desalinearse.
	// Los rascacielos (trabajo) son las parcelas mas altas; el resto son casas y apartamentos de los ciudadanos.
	WorkLots.Reset();
	if (!WorkMeshes.IsEmpty() && Layout.Lots.Num() > 1)
	{
		TArray<int32> Order;
		for (int32 I = 0; I < Layout.Lots.Num(); ++I) { Order.Add(I); }
		Order.Sort([this](int32 A, int32 B) { return Layout.Lots[A].Height > Layout.Lots[B].Height; });
		const int32 NumWork = FMath::Clamp(FMath::RoundToInt(Layout.Lots.Num() * WorkRatio), 1, Layout.Lots.Num() - 1);
		for (int32 I = 0; I < NumWork; ++I) { WorkLots.Add(Order[I]); }
	}

	int32 LotIndex = 0;
	for (const FCityLot& Lot : Layout.Lots)
	{
		const FTransform LocalTransform(FRotator(0.f, Lot.Yaw, 0.f), Lot.Location, FVector::OneVector);

		ACityBuilding* Building = World->SpawnActor<ACityBuilding>(BuildingClass, LocalTransform * SceneTransform, Params);
		if (!Building)
		{
			continue;
		}

		const FVector LotSize(Lot.Depth, Lot.Width, Lot.Height);
		const bool bWork = IsWorkLot(LotIndex);
		// Casas con color propio (pastel segun la parcela); los edificios de trabajo conservan su textura original.
		static const FLinearColor HomeColors[] = {
			FLinearColor(0.95f, 0.78f, 0.55f), FLinearColor(0.85f, 0.45f, 0.38f), FLinearColor(0.60f, 0.78f, 0.90f),
			FLinearColor(0.70f, 0.88f, 0.62f), FLinearColor(0.95f, 0.90f, 0.55f), FLinearColor(0.88f, 0.68f, 0.82f),
			FLinearColor(0.98f, 0.70f, 0.60f), FLinearColor(0.55f, 0.75f, 0.72f) };
		FRandomStream ColorRng(static_cast<int32>(HashCombine(GetTypeHash(Seed), GetTypeHash(LotIndex * 31 + 5))));
		const FLinearColor Tint = bWork ? FLinearColor::White : HomeColors[ColorRng.RandRange(0, UE_ARRAY_COUNT(HomeColors) - 1)];
		Building->SetBuildingMesh(PickBuildingMesh(Lot, LotIndex, bWork), LotSize, BuildingMeshYawOffset, Tint, 1.0f);
		Building->SetBuildingType(bWork ? EBuildingType::Work : EBuildingType::Home);
		++LotIndex;
		Building->Tags.Add(GetOwnTag());
#if WITH_EDITOR
		Building->SetFolderPath(TEXT("City"));
#endif
		SpawnedBuildings.Add(Building);
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
	StreetWidth = InStreetWidth * WidgetUnitScale;
	MinBlockSize = InMinBlockSize * WidgetUnitScale;
	MaxBlockSize = InMaxBlockSize * WidgetUnitScale;
	TargetLotSize = InLotSize * WidgetUnitScale;
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
		return;
	}

	// El tipo (casa/trabajo) ya lo fijo BuildLayout segun la malla: oficinas = trabajo, casas y pisos = casa.
	TArray<ACityBuilding*> Homes;
	TArray<ACityBuilding*> Works;
	for (ACityBuilding* Building : SpawnedBuildings)
	{
		(Building->GetBuildingType() == EBuildingType::Work ? Works : Homes).Add(Building);
	}
	if (Homes.IsEmpty() || Works.IsEmpty())
	{
		return;
	}
	for (int32 I = Homes.Num() - 1; I > 0; --I)
	{
		Homes.Swap(I, Rng.RandRange(0, I));
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

}
