#include "Lighting/GameplayLightComponent.h"
#include "Lighting/IlluminationSubsystem.h"
#include "Components/LightComponent.h"
#include "Engine/World.h"

void UGameplayLightComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UIlluminationSubsystem* Illumination = GetWorld()->GetSubsystem<UIlluminationSubsystem>())
	{
		Illumination->RegisterLight(this);
	}
}

void UGameplayLightComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UIlluminationSubsystem* Illumination = GetWorld()->GetSubsystem<UIlluminationSubsystem>())
	{
		Illumination->UnregisterLight(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UGameplayLightComponent::SetLightEnabled(bool bEnabled)
{
	bLightEnabled = bEnabled;
	if (LinkedLight)
	{
		LinkedLight->SetVisibility(bEnabled);
	}
}

void UGameplayLightComponent::SetLinkedLight(ULightComponent* InLinkedLight)
{
	LinkedLight = InLinkedLight;
	SetLightEnabled(bLightEnabled);
}
