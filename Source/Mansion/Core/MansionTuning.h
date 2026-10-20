#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MansionTuning.generated.h"

/**
 * Every balance number in one place. Edit under Project Settings > Game > Mansion Tuning;
 * values are saved to Config/DefaultGame.ini so no recompile is needed.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mansion Tuning"))
class MANSION_API UMansionTuning : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UMansionTuning& Get() { return *GetDefault<UMansionTuning>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// --- Match ---

	UPROPERTY(Config, EditAnywhere, Category = "Match", meta = (ClampMin = "0", Units = "s"))
	float PreGameDuration = 60.f;

	/** Length of the Playing phase. When it runs out the Magic Door opens (Final Hunt). */
	UPROPERTY(Config, EditAnywhere, Category = "Match", meta = (ClampMin = "1", Units = "s"))
	float MatchDuration = 900.f;

	// --- Exposure ---

	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0.05", Units = "s"))
	float ExposureUpdateInterval = 0.2f;

	/** Moving further than this from the anchor point resets the camping timer. */
	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0", Units = "cm"))
	float CampMoveTolerance = 150.f;

	/** Seconds standing within CampMoveTolerance before the House sees you. */
	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0", Units = "s"))
	float CampThreshold = 20.f;

	/** Illumination below this reveals the Survivor to the House. */
	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0"))
	float DarkThreshold = 0.15f;

	/** How long a monster sighting keeps the Survivor revealed after line of sight breaks. */
	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0", Units = "s"))
	float MonsterSightLinger = 1.5f;

	/** Sighting reports further apart than this break a "continuous" sight streak. */
	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0", Units = "s"))
	float MonsterSightGap = 0.5f;

	/** Continuous monster targeting required before the House may Voice Snip. */
	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0", Units = "s"))
	float SnipMonsterSightTime = 3.f;

	/** How long the last-known-position ghost lingers on the House's board. */
	UPROPERTY(Config, EditAnywhere, Category = "Exposure", meta = (ClampMin = "0", Units = "s"))
	float RevealGhostDuration = 6.f;

	// --- Fear ---

	UPROPERTY(Config, EditAnywhere, Category = "Fear", meta = (ClampMin = "1"))
	float FearMax = 100.f;

	/** Maximum bonus Fear adds to stumble / drop-item chances (0.075 = +7.5%). */
	UPROPERTY(Config, EditAnywhere, Category = "Fear", meta = (ClampMin = "0", ClampMax = "1"))
	float FearRollBonusMax = 0.075f;

	UPROPERTY(Config, EditAnywhere, Category = "Fear", meta = (ClampMin = "0"))
	float FearDarkGainPerSecond = 4.f;

	UPROPERTY(Config, EditAnywhere, Category = "Fear", meta = (ClampMin = "0"))
	float FearLightDecayPerSecond = 15.f;

	/** Illumination at or above this counts as "in light" and drains Fear quickly. */
	UPROPERTY(Config, EditAnywhere, Category = "Fear", meta = (ClampMin = "0"))
	float FearLightThreshold = 0.4f;

	// --- Survivor movement ---

	UPROPERTY(Config, EditAnywhere, Category = "Survivor", meta = (ClampMin = "0", Units = "cm/s"))
	float WalkSpeed = 300.f;

	UPROPERTY(Config, EditAnywhere, Category = "Survivor", meta = (ClampMin = "0", Units = "cm/s"))
	float SprintSpeed = 550.f;

	UPROPERTY(Config, EditAnywhere, Category = "Survivor", meta = (ClampMin = "0", Units = "cm/s"))
	float CrouchSpeed = 150.f;
};
