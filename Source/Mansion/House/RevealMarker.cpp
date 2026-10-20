#include "House/RevealMarker.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ARevealMarker::ARevealMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetStaticMesh(CylinderMesh.Object);
	Beacon->SetRelativeScale3D(FVector(0.7f, 0.7f, 2.5f));
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetCastShadow(false);
	RootComponent = Beacon;
}

void ARevealMarker::BeginPlay()
{
	Super::BeginPlay();

	Material = Beacon->CreateDynamicMaterialInstance(0);
}

void ARevealMarker::SetLive(const FVector& Location, const FLinearColor& Color)
{
	bGhost = false;
	SetActorLocation(Location);
	SetActorScale3D(FVector::OneVector);

	if (Material && Color != CurrentColor)
	{
		// BasicShapeMaterial exposes a "Color" vector parameter.
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
	CurrentColor = Color;
}

void ARevealMarker::StartGhost(float Duration)
{
	bGhost = true;
	GhostDuration = FMath::Max(Duration, 0.01f);
	GhostElapsed = 0.f;

	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), CurrentColor * 0.35f);
	}
}

void ARevealMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bGhost)
	{
		return;
	}

	GhostElapsed += DeltaSeconds;
	const float Alpha = 1.f - GhostElapsed / GhostDuration;
	if (Alpha <= 0.f)
	{
		Destroy();
		return;
	}
	SetActorScale3D(FVector(Alpha));
}
