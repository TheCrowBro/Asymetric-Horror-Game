#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/MansionTypes.h"
#include "ExposureComponent.generated.h"

struct MANSION_API FExposureParams
{
	float MoveTolerance = 150.f;
	float CampThreshold = 20.f;
	float DarkThreshold = 0.15f;
	float MonsterSightLinger = 1.5f;
	float MonsterSightGap = 0.5f;

	static FExposureParams FromTuning();
};

/** World-free exposure rules, so they can be unit tested. */
struct MANSION_API FExposureTracker
{
	/** Call every time a monster's perception confirms line of sight to this Survivor. */
	void ReportMonsterSight(float Now, const FExposureParams& Params);

	/** Advances timers and returns the current ERevealReason bitmask. */
	uint8 Update(const FVector& Location, float Illumination, float Now, float DeltaTime, const FExposureParams& Params);

	float GetStationaryTime() const { return StationaryTime; }

	/** Length of the current unbroken run of monster sightings (0 if not currently seen). */
	float GetContinuousMonsterSightTime(float Now, const FExposureParams& Params) const;

	int32 ForcedRevealCount = 0;

private:
	FVector Anchor = FVector::ZeroVector;
	bool bHasAnchor = false;
	float StationaryTime = 0.f;
	float LastSightTime = -1.e9f;
	float SightStreakStart = -1.e9f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRevealChanged, uint8, RevealReasons);

/**
 * Decides when the House can see this Survivor's exact position: camping, standing in
 * darkness, or being seen by a monster. Logic runs on the server; RevealReasons replicates
 * to everyone so the House client can draw markers and the owner can show warnings.
 */
UCLASS(ClassGroup = Mansion, meta = (BlueprintSpawnableComponent))
class MANSION_API UExposureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UExposureComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Exposure")
	bool IsRevealed() const { return RevealReasons != 0; }

	UFUNCTION(BlueprintPure, Category = "Exposure")
	bool HasRevealReason(ERevealReason Reason) const { return (RevealReasons & static_cast<uint8>(Reason)) != 0; }

	UFUNCTION(BlueprintPure, Category = "Exposure")
	uint8 GetRevealReasons() const { return RevealReasons; }

	/** Replicated to the owning client only. */
	UFUNCTION(BlueprintPure, Category = "Exposure")
	float GetIllumination() const { return Illumination; }

	/** Replicated to the owning client only. */
	UFUNCTION(BlueprintPure, Category = "Exposure")
	float GetStationaryTime() const { return StationaryTime; }

	/** Server: a monster currently has line of sight to us. */
	void ReportSeenByMonster();

	/** Server: scripted reveals (ritual channel etc.). Calls must be balanced. */
	void PushForcedReveal();
	void PopForcedReveal();

	/** Server: may the House Voice Snip this Survivor right now? */
	bool IsVoiceSnipEligible() const;

	UPROPERTY(BlueprintAssignable, Category = "Exposure")
	FOnRevealChanged OnRevealChanged;

protected:
	virtual void BeginPlay() override;

	void UpdateExposure();
	void SetRevealReasons(uint8 NewReasons);

	UFUNCTION()
	void OnRep_RevealReasons();

	UPROPERTY(ReplicatedUsing = OnRep_RevealReasons)
	uint8 RevealReasons = 0;

	UPROPERTY(Replicated)
	float Illumination = 1.f;

	UPROPERTY(Replicated)
	float StationaryTime = 0.f;

private:
	FExposureTracker Tracker;
	FTimerHandle UpdateTimer;
	double LastUpdateTime = 0.0;
};
