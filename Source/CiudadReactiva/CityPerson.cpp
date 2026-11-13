#include "CityPerson.h"
#include "CityBuilding.h"
#include "CityClock.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
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

	CurrentTarget = nullptr;
	bArrived = false;
	RetryTimer = 0.0f;
}

void ACityPerson::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Home || !Work || !Clock)
	{
		return;
	}

	const float Hour = Clock->getHour();
	const bool bWorkTime = (Hour >= WorkStartTime && Hour < WorkEndTime);
	ACityBuilding* Target = bWorkTime ? Work.Get() : Home.Get();
	
	if (Target != CurrentTarget)
	{
		CurrentTarget = Target;
		bArrived = false;
		RetryTimer = 0.f;
	}

	if (bArrived)
	{
		return;
	}

	const FVector Destination = Target->GetEntranceLocation();

	FVector ToTarget = Destination - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.SizeSquared() < FMath::Square(BuildingRadius))
	{
		bArrived = true;
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}
		return;
	}

	RetryTimer -= DeltaTime;
	if (RetryTimer > 0.0f)
	{
		return;
	}
	RetryTimer = RetryInterval;


	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		return;
	}

	if (AI->GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}
	const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(
		Destination,
		AcceptanceRadius,
		/*bStopOnOverlap*/ true,
		/*bUsePathfinding*/ true,
		/*bProjectDestinationToNavigation*/ true,
		/*bCanStrafe*/ false,
		/*FilterClass*/ nullptr,
		/*bAllowPartialPath*/ true);

	if (Result == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		bArrived = true;
	}
	else if (Result == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Verbose, TEXT("%s: MoveTo fallido, reintentando (NavMesh listo?)"), *GetName());
	}
}

void ACityPerson::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}