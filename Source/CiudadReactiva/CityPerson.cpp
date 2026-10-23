

#include "CityPerson.h"
#include "CityBuilding.h"
#include "CityClock.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"


ACityPerson::ACityPerson()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AAIController::StaticClass();
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

void ACityPerson::SetRoute(ACityBuilding* InHome, ACityBuilding* InWork, ACityClock* InClock)
{
	Home = InHome;
	Work = InWork;
	Clock = InClock;
}

void ACityPerson::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Home || !Work || !Clock)
	{
		return;
	}

	const float Hour = Clock->getHour();
	const bool workTime = (Hour >= WorkStartTime && Hour < WorkEndTime);
	const ACityBuilding* Target = workTime ? Work : Home;	
	
	FVector ToTarget = Target->GetEntranceLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.SizeSquared() < FMath::Square(BuildingRadius))
	{
		return;
	}

	AddMovementInput(ToTarget.GetSafeNormal());
}

void ACityPerson::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}