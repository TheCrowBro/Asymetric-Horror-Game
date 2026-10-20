#include "World/LightFixture.h"
#include "Lighting/GameplayLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ALightFixture::ALightFixture()
{
	bReplicates = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Bulb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bulb"));
	Bulb->SetStaticMesh(SphereMesh.Object);
	Bulb->SetRelativeScale3D(FVector(0.2f));
	Bulb->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bulb->SetCastShadow(false);
	RootComponent = Bulb;

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(Bulb);
	PointLight->SetIntensityUnits(ELightUnits::Lumens);
	PointLight->SetIntensity(800.f);
	PointLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.6f));

	GameplayLight = CreateDefaultSubobject<UGameplayLightComponent>(TEXT("GameplayLight"));
	GameplayLight->SetupAttachment(Bulb);
	GameplayLight->Intensity = 1.f;
	GameplayLight->Radius = 1000.f;
}

void ALightFixture::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALightFixture, bIsOn);
}

void ALightFixture::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Keep the visual reach roughly matching the gameplay reach.
	PointLight->SetAttenuationRadius(GameplayLight->Radius);
}

void ALightFixture::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GameplayLight->SetLinkedLight(PointLight);
	OnRep_IsOn();
}

void ALightFixture::SetLightOn(bool bOn)
{
	bIsOn = bOn;
	OnRep_IsOn();
}

void ALightFixture::OnRep_IsOn()
{
	GameplayLight->SetLightEnabled(bIsOn);
}
