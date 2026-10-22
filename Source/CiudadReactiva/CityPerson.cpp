

#include "CityPerson.h"
#include "CityBuilding.h"
//#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"


ACityPerson::ACityPerson()
{
	PrimaryActorTick.bCanEverTick = true;

	//AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.7f));
	if (CylinderMesh.Succeeded())
	{
		Body->SetStaticMesh(CylinderMesh.Object);
	}
}

void ACityPerson::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ACityPerson::SetRoute(ACityBuilding* InHome, ACityBuilding* InWork)
{
	Home = InHome;
	Work = InWork;
	TargetBuilding = Work;
	bWaiting = false;
	WaitRemaining = 0.f;
}

void ACityPerson::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetBuilding)
	{
		return;
	}

	if (bWaiting)
	{
		WaitRemaining -= DeltaTime;
		if (WaitRemaining <= 0.f)
		{
			bWaiting = false;
			TargetBuilding = (TargetBuilding == Work) ? Home : Work;
		}
		return;
	}

	FVector ToTarget = TargetBuilding->GetEntranceLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.Size2D() <= BuildingRadius)
	{
		bWaiting = true;
		WaitRemaining = WaitTime;
		return;
	}

	AddMovementInput(ToTarget.GetSafeNormal(), 1.f);
}

void ACityPerson::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}