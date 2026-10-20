#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/MansionTypes.h"
#include "MansionPlayerState.generated.h"

UCLASS()
class MANSION_API AMansionPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintPure, Category = "Mansion")
	EMansionRole GetMansionRole() const { return MansionRole; }

	UFUNCTION(BlueprintPure, Category = "Mansion")
	ESurvivorStatus GetSurvivorStatus() const { return SurvivorStatus; }

	/** Server only. */
	void SetMansionRole(EMansionRole NewRole) { MansionRole = NewRole; }

	/** Server only. */
	void SetSurvivorStatus(ESurvivorStatus NewStatus) { SurvivorStatus = NewStatus; }

protected:
	// Named MansionRole because AActor already has a "Role" member (net role).
	UPROPERTY(Replicated)
	EMansionRole MansionRole = EMansionRole::None;

	UPROPERTY(Replicated)
	ESurvivorStatus SurvivorStatus = ESurvivorStatus::Alive;
};
