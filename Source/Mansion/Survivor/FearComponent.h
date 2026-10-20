#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FearComponent.generated.h"

/** World-free Fear rules, so they can be unit tested. */
struct MANSION_API FMansionFear
{
	/** Extra chance added to stumble/drop rolls: linear in Fear, capped at MaxBonus. */
	static float RollBonus(float Fear, float MaxFear, float MaxBonus);

	/** Light drains Fear quickly, darkness raises it, dim light leaves it alone. */
	static float Step(float Fear, float Illumination, float DeltaTime, float MaxFear,
		float LightThreshold, float DarkThreshold, float LightDecayPerSecond, float DarkGainPerSecond);
};

/**
 * Per-Survivor Fear meter (0..FearMax). Raises the chance to stumble and drop items by up
 * to FearRollBonusMax (+7.5%). Server authoritative, replicated to everyone.
 */
UCLASS(ClassGroup = Mansion, meta = (BlueprintSpawnableComponent))
class MANSION_API UFearComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFearComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Fear")
	float GetFear() const { return Fear; }

	/** Fear as 0..1. */
	UFUNCTION(BlueprintPure, Category = "Fear")
	float GetFearAlpha() const;

	UFUNCTION(BlueprintPure, Category = "Fear")
	float GetRollBonus() const;

	/** Server: monsters, House abilities and teammates going down call this. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Fear")
	void AddFear(float Amount);

	/** Server: roll a stumble / item-drop check. Returns true if it happens. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Fear")
	bool RollWithFear(float BaseChance) const;

protected:
	virtual void BeginPlay() override;

	void UpdateFear();

	UPROPERTY(Replicated)
	float Fear = 0.f;

private:
	FTimerHandle UpdateTimer;
	double LastUpdateTime = 0.0;
};
