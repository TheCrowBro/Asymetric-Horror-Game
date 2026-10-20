#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Core/MansionTypes.h"
#include "MansionGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMansionPhaseChanged, EMansionPhase, NewPhase);

UCLASS()
class MANSION_API AMansionGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Mansion")
	EMansionPhase GetPhase() const { return Phase; }

	/** Seconds left in the current phase, or -1 if the phase has no timer. */
	UFUNCTION(BlueprintPure, Category = "Mansion")
	float GetPhaseTimeRemaining() const;

	/** Server only. Duration <= 0 means the phase has no timer. */
	void SetPhase(EMansionPhase NewPhase, float Duration);

	UPROPERTY(BlueprintAssignable, Category = "Mansion")
	FOnMansionPhaseChanged OnPhaseChanged;

protected:
	UFUNCTION()
	void OnRep_Phase();

	UPROPERTY(ReplicatedUsing = OnRep_Phase)
	EMansionPhase Phase = EMansionPhase::WaitingToStart;

	/** In server world time (GetServerWorldTimeSeconds). */
	UPROPERTY(Replicated)
	double PhaseEndServerTime = -1.0;
};
