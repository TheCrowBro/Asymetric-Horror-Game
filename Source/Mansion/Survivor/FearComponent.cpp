#include "Survivor/FearComponent.h"
#include "Core/MansionTuning.h"
#include "Survivor/ExposureComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

// --- FMansionFear ---

float FMansionFear::RollBonus(float Fear, float MaxFear, float MaxBonus)
{
	if (MaxFear <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Fear / MaxFear, 0.f, 1.f) * MaxBonus;
}

float FMansionFear::Step(float Fear, float Illumination, float DeltaTime, float MaxFear,
	float LightThreshold, float DarkThreshold, float LightDecayPerSecond, float DarkGainPerSecond)
{
	if (Illumination >= LightThreshold)
	{
		Fear -= LightDecayPerSecond * DeltaTime;
	}
	else if (Illumination < DarkThreshold)
	{
		Fear += DarkGainPerSecond * DeltaTime;
	}
	return FMath::Clamp(Fear, 0.f, MaxFear);
}

// --- UFearComponent ---

UFearComponent::UFearComponent()
{
	SetIsReplicatedByDefault(true);
}

void UFearComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UFearComponent, Fear);
}

void UFearComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		LastUpdateTime = GetWorld()->GetTimeSeconds();
		const float Interval = FMath::Max(0.05f, UMansionTuning::Get().ExposureUpdateInterval);
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UFearComponent::UpdateFear, Interval, /*bLoop*/ true);
	}
}

void UFearComponent::UpdateFear()
{
	const double Now = GetWorld()->GetTimeSeconds();
	const float DeltaTime = static_cast<float>(Now - LastUpdateTime);
	LastUpdateTime = Now;

	// Exposure already samples illumination on the same interval; reuse it.
	const UExposureComponent* Exposure = GetOwner()->FindComponentByClass<UExposureComponent>();
	if (!Exposure)
	{
		return;
	}

	const UMansionTuning& Tuning = UMansionTuning::Get();
	Fear = FMansionFear::Step(Fear, Exposure->GetIllumination(), DeltaTime, Tuning.FearMax,
		Tuning.FearLightThreshold, Tuning.DarkThreshold, Tuning.FearLightDecayPerSecond, Tuning.FearDarkGainPerSecond);
}

float UFearComponent::GetFearAlpha() const
{
	const float MaxFear = UMansionTuning::Get().FearMax;
	return MaxFear > 0.f ? FMath::Clamp(Fear / MaxFear, 0.f, 1.f) : 0.f;
}

float UFearComponent::GetRollBonus() const
{
	const UMansionTuning& Tuning = UMansionTuning::Get();
	return FMansionFear::RollBonus(Fear, Tuning.FearMax, Tuning.FearRollBonusMax);
}

void UFearComponent::AddFear(float Amount)
{
	Fear = FMath::Clamp(Fear + Amount, 0.f, UMansionTuning::Get().FearMax);
}

bool UFearComponent::RollWithFear(float BaseChance) const
{
	return FMath::FRand() < BaseChance + GetRollBonus();
}
