#include "Survivor/ExposureComponent.h"
#include "Core/MansionTuning.h"
#include "Lighting/IlluminationSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

static TAutoConsoleVariable<bool> CVarShowExposure(
	TEXT("mansion.ShowExposure"),
	false,
	TEXT("Draw each Survivor's illumination, camping timer and reveal reasons (server view)."),
	ECVF_Cheat);

// --- FExposureParams / FExposureTracker ---

FExposureParams FExposureParams::FromTuning()
{
	const UMansionTuning& Tuning = UMansionTuning::Get();
	FExposureParams Params;
	Params.MoveTolerance = Tuning.CampMoveTolerance;
	Params.CampThreshold = Tuning.CampThreshold;
	Params.DarkThreshold = Tuning.DarkThreshold;
	Params.MonsterSightLinger = Tuning.MonsterSightLinger;
	Params.MonsterSightGap = Tuning.MonsterSightGap;
	return Params;
}

void FExposureTracker::ReportMonsterSight(float Now, const FExposureParams& Params)
{
	if (Now - LastSightTime > Params.MonsterSightGap)
	{
		SightStreakStart = Now;
	}
	LastSightTime = Now;
}

uint8 FExposureTracker::Update(const FVector& Location, float Illumination, float Now, float DeltaTime, const FExposureParams& Params)
{
	if (!bHasAnchor || FVector::DistSquared(Location, Anchor) > FMath::Square(Params.MoveTolerance))
	{
		Anchor = Location;
		bHasAnchor = true;
		StationaryTime = 0.f;
	}
	else
	{
		StationaryTime += DeltaTime;
	}

	ERevealReason Reasons = ERevealReason::None;
	if (StationaryTime >= Params.CampThreshold)
	{
		Reasons |= ERevealReason::Camping;
	}
	if (Illumination < Params.DarkThreshold)
	{
		Reasons |= ERevealReason::Dark;
	}
	if (Now - LastSightTime <= Params.MonsterSightLinger)
	{
		Reasons |= ERevealReason::SeenByMonster;
	}
	if (ForcedRevealCount > 0)
	{
		Reasons |= ERevealReason::Forced;
	}
	return static_cast<uint8>(Reasons);
}

float FExposureTracker::GetContinuousMonsterSightTime(float Now, const FExposureParams& Params) const
{
	if (Now - LastSightTime > Params.MonsterSightGap)
	{
		return 0.f;
	}
	return LastSightTime - SightStreakStart;
}

// --- UExposureComponent ---

UExposureComponent::UExposureComponent()
{
	SetIsReplicatedByDefault(true);
}

void UExposureComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UExposureComponent, RevealReasons);
	DOREPLIFETIME_CONDITION(UExposureComponent, Illumination, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UExposureComponent, StationaryTime, COND_OwnerOnly);
}

void UExposureComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		LastUpdateTime = GetWorld()->GetTimeSeconds();
		const float Interval = FMath::Max(0.05f, UMansionTuning::Get().ExposureUpdateInterval);
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &UExposureComponent::UpdateExposure, Interval, /*bLoop*/ true);
	}
}

void UExposureComponent::UpdateExposure()
{
	const UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();
	const double Now = World->GetTimeSeconds();
	const float DeltaTime = static_cast<float>(Now - LastUpdateTime);
	LastUpdateTime = Now;

	const UIlluminationSubsystem* IlluminationSubsystem = World->GetSubsystem<UIlluminationSubsystem>();
	Illumination = IlluminationSubsystem ? IlluminationSubsystem->GetIlluminationAt(Owner->GetActorLocation(), Owner) : 1.f;

	const uint8 NewReasons = Tracker.Update(Owner->GetActorLocation(), Illumination, static_cast<float>(Now), DeltaTime, FExposureParams::FromTuning());
	StationaryTime = Tracker.GetStationaryTime();
	SetRevealReasons(NewReasons);

	if (CVarShowExposure.GetValueOnGameThread())
	{
		const FString Text = FString::Printf(TEXT("Light %.2f  Still %.1fs  %s%s%s%s"),
			Illumination, StationaryTime,
			HasRevealReason(ERevealReason::Camping) ? TEXT("[CAMP]") : TEXT(""),
			HasRevealReason(ERevealReason::Dark) ? TEXT("[DARK]") : TEXT(""),
			HasRevealReason(ERevealReason::SeenByMonster) ? TEXT("[SEEN]") : TEXT(""),
			HasRevealReason(ERevealReason::Forced) ? TEXT("[FORCED]") : TEXT(""));
		DrawDebugString(World, FVector(0.f, 0.f, 120.f), Text, const_cast<AActor*>(Owner),
			IsRevealed() ? FColor::Red : FColor::Green, UMansionTuning::Get().ExposureUpdateInterval + 0.05f);
	}
}

void UExposureComponent::SetRevealReasons(uint8 NewReasons)
{
	if (NewReasons != RevealReasons)
	{
		RevealReasons = NewReasons;
		OnRep_RevealReasons();
	}
}

void UExposureComponent::OnRep_RevealReasons()
{
	OnRevealChanged.Broadcast(RevealReasons);
}

void UExposureComponent::ReportSeenByMonster()
{
	Tracker.ReportMonsterSight(static_cast<float>(GetWorld()->GetTimeSeconds()), FExposureParams::FromTuning());
	SetRevealReasons(RevealReasons | static_cast<uint8>(ERevealReason::SeenByMonster));
}

void UExposureComponent::PushForcedReveal()
{
	++Tracker.ForcedRevealCount;
	SetRevealReasons(RevealReasons | static_cast<uint8>(ERevealReason::Forced));
}

void UExposureComponent::PopForcedReveal()
{
	Tracker.ForcedRevealCount = FMath::Max(0, Tracker.ForcedRevealCount - 1);
}

bool UExposureComponent::IsVoiceSnipEligible() const
{
	const float Now = static_cast<float>(GetWorld()->GetTimeSeconds());
	return HasRevealReason(ERevealReason::Camping)
		|| Tracker.GetContinuousMonsterSightTime(Now, FExposureParams::FromTuning()) >= UMansionTuning::Get().SnipMonsterSightTime;
}
