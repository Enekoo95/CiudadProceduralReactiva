// Fill out your copyright notice in the Description page of Project Settings.


#include "CityBuilding.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/PackageName.h"
#include "Engine/StaticMesh.h"


namespace
{
	UMaterialInterface* LoadCarlaMaterial(const FString& PathNoExt)
	{
		const FString Name = FPackageName::GetShortName(PathNoExt);
		return LoadObject<UMaterialInterface>(nullptr, *(PathNoExt + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	// Los .uasset de CARLA importados fuera de su proyecto pierden la asignacion de materiales (salen con WorldGridMaterial).
	// Aqui se reasignan buscando el material por familia de malla y por nombre de slot.
	UMaterialInterface* FindMaterialForSlot(const FString& MeshName, const FString& SlotName)
	{
		const FString Houses = TEXT("/Game/Carla/Static/Building/Houses/00_IndividualComponents/");
		const FString Generic = TEXT("/Game/Carla/Static/Building/Buildings/BuildingComplete/00_GenericComponents/Apt32_to_35/");
		const FString Individual = TEXT("/Game/Carla/Static/Building/Buildings/BuildingComplete/00_IndividualComponents/");
		const FString TownC = TEXT("/Game/Carla/Static/Static/Town_C/Building/Material/");
		const FString Slot = SlotName.ToLower();

		// 1) Un material con el mismo nombre que el slot (Town_C usa MI_<edificio>_<parte>).
		if (SlotName.StartsWith(TEXT("MI_")) || SlotName.StartsWith(TEXT("M_")))
		{
			for (const FString& Dir : { TownC, Houses + TEXT("House17/"), Houses + TEXT("House18/"), Generic })
			{
				if (UMaterialInterface* M = LoadCarlaMaterial(Dir + SlotName)) { return M; }
			}
		}

		// 1b) Edificios de Town_C: los slots se llaman SM_windos, M_addpart... y los materiales MI_<Edificio>_<Parte>.
		{
			FString Family;
			if (MeshName.Contains(TEXT("SIAC"))) { Family = TEXT("MI_ShangHaiAutoBuild"); }
			else if (MeshName.Contains(TEXT("SorlHotel"))) { Family = TEXT("MI_RuiLiHotel"); }
			else if (MeshName.Contains(TEXT("AutoMuseum"))) { Family = TEXT("MI_ShangHaiAutoMuseum"); }
			else if (MeshName.Contains(TEXT("ExhibitionCenter"))) { Family = TEXT("MI_ShangHaiAutoShowCenter"); }
			else if (MeshName.Contains(TEXT("Theatre"))) { Family = TEXT("MI_ShanghaiPolyGrandTheatre"); }
			else if (MeshName.Contains(TEXT("YuTian"))) { Family = TEXT("MI_YuTian"); }
			if (!Family.IsEmpty())
			{
				FString Part = SlotName;
				for (const TCHAR* Prefix : { TEXT("MI_"), TEXT("SM_"), TEXT("M_") })
				{
					if (Part.StartsWith(Prefix)) { Part = Part.RightChop(FCString::Strlen(Prefix)); break; }
				}
				const FString Low = Part.ToLower();
				FString Alias;
				if (Low == TEXT("windos")) { Alias = TEXT("Window"); }
				else if (Low == TEXT("metalframe")) { Alias = TEXT("MetalFrame"); }
				else if (Low == TEXT("fulou")) { Alias = TEXT("Fulou"); }
				else if (Low == TEXT("zhuti")) { Alias = TEXT("Zhuti"); }
				else if (Low == TEXT("addpart")) { Alias = TEXT("AddPart"); }
				else if (Low == TEXT("part1")) { Alias = TEXT("Part01"); }
				else if (Low == TEXT("partglass") || Low == TEXT("rlglass")) { Alias = TEXT("Glass"); }
				else if (Low == TEXT("mainoutside")) { Alias = TEXT("MainOutSide"); }
				else if (Low == TEXT("sidebuilding")) { Alias = TEXT("Side"); }
				else if (Low == TEXT("center")) { Alias = FString(); }
				for (const FString& Candidate : { Family + TEXT("_") + Part, Family + TEXT("_") + Alias, Family })
				{
					if (Candidate.EndsWith(TEXT("_"))) { continue; }
					if (UMaterialInterface* M = LoadCarlaMaterial(TownC + Candidate)) { return M; }
				}
			}
		}

		// 2) Por familia de malla.
		if (MeshName.Contains(TEXT("House17"))) { return LoadCarlaMaterial(Houses + TEXT("House17/MI_House17")); }
		if (MeshName.Contains(TEXT("House18"))) { return LoadCarlaMaterial(Houses + TEXT("House18/MI_House18")); }
		if (MeshName.Contains(TEXT("Apt33"))) { return LoadCarlaMaterial(Individual + TEXT("Apt33/MI_Apt33")); }
		if (MeshName.Contains(TEXT("Apt34"))) { return LoadCarlaMaterial(Individual + TEXT("Apt34/MI_Apt34")); }
		if (MeshName.Contains(TEXT("Apt32")) || MeshName.Contains(TEXT("Apt35")))
		{
			if (Slot.Contains(TEXT("brick"))) { return LoadCarlaMaterial(Generic + TEXT("MI_Apt_Bricks")); }
			if (Slot.Contains(TEXT("detail"))) { return LoadCarlaMaterial(Generic + TEXT("MI_Apt_Details_A")); }
			if (Slot.Contains(TEXT("frame"))) { return LoadCarlaMaterial(Generic + TEXT("MI_Apt_Frames")); }
			if (Slot.Contains(TEXT("roof"))) { return LoadCarlaMaterial(Generic + TEXT("MI_Apt_Roof")); }
			return LoadCarlaMaterial(Generic + TEXT("MI_Apt_Wall"));
		}
		return nullptr;
	}

	bool IsUntintedSlot(const FString& SlotName)
	{
		const FString S = SlotName.ToLower();
		return S.Contains(TEXT("glass")) || S.Contains(TEXT("window")) || S.Contains(TEXT("windos")) || S.Contains(TEXT("frame"))
			|| S.Contains(TEXT("roof")) || S.Contains(TEXT("interior")) || S.Contains(TEXT("detail"));
	}

	void ApplyAssetMaterials(UStaticMeshComponent* Comp, UStaticMesh* InMesh, const FLinearColor& Tint)
	{
		UMaterialInterface* TintMaster = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_CityTint.M_CityTint"), nullptr, LOAD_NoWarn | LOAD_Quiet);
		const bool bTint = TintMaster && !Tint.Equals(FLinearColor::White);

		const FString MeshName = InMesh->GetName();
		const TArray<FStaticMaterial>& Slots = InMesh->GetStaticMaterials();
		for (int32 I = 0; I < Slots.Num(); ++I)
		{
			UMaterialInterface* Current = Slots[I].MaterialInterface;
			const bool bMissing = !Current || Current->GetName().Contains(TEXT("WorldGrid")) || Current->GetName().Contains(TEXT("DefaultMaterial"));
			UMaterialInterface* Found = bMissing ? FindMaterialForSlot(MeshName, Slots[I].MaterialSlotName.ToString()) : nullptr;
			UMaterialInterface* Source = Found ? Found : Current;

			if (bTint && Source && !IsUntintedSlot(Slots[I].MaterialSlotName.ToString()))
			{
				UTexture* Base = nullptr;
				if (Source->GetTextureParameterValue(FHashedMaterialParameterInfo(FName(TEXT("BaseColor"))), Base) && Base)
				{
					UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(TintMaster, Comp);
					MID->SetTextureParameterValue(TEXT("BaseColor"), Base);
					UTexture* Normal = nullptr;
					if (Source->GetTextureParameterValue(FHashedMaterialParameterInfo(FName(TEXT("Normal"))), Normal) && Normal)
					{
						MID->SetTextureParameterValue(TEXT("Normal"), Normal);
					}
					MID->SetVectorParameterValue(TEXT("Tint"), Tint);
					Comp->SetMaterial(I, MID);
					continue;
				}
			}
			Comp->SetMaterial(I, Found); // null = material propio de la malla
		}
	}
}

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

	Entrance = CreateDefaultSubobject<USceneComponent>(TEXT("Entrance"));
	Entrance->SetupAttachment(Root);

	SetSize(FVector(300.0f, 300.0f, 500.0f));
}



void ACityBuilding::SetBuildingType(EBuildingType NewType)
{
	BuildingType = NewType;
	const bool bIsHome = (BuildingType == EBuildingType::Home);

	// Con malla de los assets se respetan sus materiales; solo el cubo se tiñe por tipo.
	if (!bUsesAssetMesh)
	{
		if (UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), bIsHome ? FLinearColor::Blue : FLinearColor::Green);
		}
	}
	if (bShowLabel)
	{
		DrawDebugString(GetWorld(), GetActorLocation() + FVector(0.f, 0.f, 600.f),
			bIsHome ? TEXT("CASA") : TEXT("TRABAJO"), nullptr, FColor::White, 100000.f, false, 2.f);
	}
}

FVector ACityBuilding::GetEntranceLocation() const
{
	return Entrance->GetComponentLocation();
}

void ACityBuilding::SetSize(const FVector& NewSize)
{
	Mesh->SetRelativeScale3D(NewSize / 100.0f);
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, NewSize.Z * 0.5f));
	Entrance->SetRelativeLocation(FVector(NewSize.X * 0.5f + 100.f, 0.f, 0.f));
}

void ACityBuilding::SetBuildingMesh(UStaticMesh* InMesh, const FVector& TargetSize, float YawOffset, const FLinearColor& Tint, float MaxUniformScale)
{
	if (!InMesh)
	{
		bUsesAssetMesh = false;
		SetSize(TargetSize);
		return;
	}

	const FBoxSphereBounds Bounds = InMesh->GetBounds();
	const FVector Ext = Bounds.BoxExtent;
	if (Ext.X < KINDA_SMALL_NUMBER || Ext.Y < KINDA_SMALL_NUMBER || Ext.Z < KINDA_SMALL_NUMBER)
	{
		bUsesAssetMesh = false;
		SetSize(TargetSize);
		return;
	}

	// Escala uniforme que hace caber la huella en la parcela, probando la malla girada 0 y 90 grados.
	auto FitScale = [&](bool bSwap)
	{
		const float EX = bSwap ? Ext.Y : Ext.X;
		const float EY = bSwap ? Ext.X : Ext.Y;
		return FMath::Min(TargetSize.X / (2.f * EX), TargetSize.Y / (2.f * EY));
	};
	const float Scale0 = FitScale(false);
	const float Scale90 = FitScale(true);
	const bool bSwap = Scale90 > Scale0;
	const float XYScale = bSwap ? Scale90 : Scale0;
	const float Yaw = YawOffset + (bSwap ? 90.f : 0.f);

	// Escala uniforme (sin deformar la fachada): cabe en la huella y no pasa mucho de la altura de la parcela.
	const float ZLimit = (TargetSize.Z * 1.3f) / (2.f * Ext.Z);
	const float Uniform = FMath::Min(FMath::Min(XYScale, ZLimit), MaxUniformScale);
	const FVector Scale3(Uniform, Uniform, Uniform);

	bUsesAssetMesh = true;
	Mesh->SetStaticMesh(InMesh);
	ApplyAssetMaterials(Mesh, InMesh, Tint);

	const FRotator Rot(0.f, Yaw, 0.f);
	// Centro de la base de la malla en su espacio local -> al origen del actor.
	const FVector BaseCenter(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - Ext.Z);
	Mesh->SetRelativeScale3D(Scale3);
	Mesh->SetRelativeRotation(Rot);
	Mesh->SetRelativeLocation(-Rot.RotateVector(BaseCenter * Scale3));

	const float HalfDepth = (bSwap ? Ext.Y : Ext.X) * Uniform;
	Entrance->SetRelativeLocation(FVector(HalfDepth + 100.f, 0.f, 0.f));
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

