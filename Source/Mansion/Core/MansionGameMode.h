#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MansionGameMode.generated.h"

/**
 * Assigns roles (1 House, up to 5 Survivors), spawns the right pawn for each role
 * and drives the match phases: PreGame -> Playing -> MagicDoorOpen.
 *
 * House selection, in priority order:
 *   1. A player joining with "?Role=House" in their URL options.
 *   2. The player whose join index equals the cvar mansion.HouseSlot (default 1,
 *      i.e. the first client, so the listen-server host is a Survivor).
 */
UCLASS()
class MANSION_API AMansionGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMansionGameMode();

	static const FName HouseStartTag;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override { return false; }

protected:
	virtual void HandleMatchHasStarted() override;

	UPROPERTY(EditDefaultsOnly, Category = "Mansion")
	TSubclassOf<APawn> SurvivorPawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Mansion")
	TSubclassOf<APawn> HousePawnClass;

	UPROPERTY(EditDefaultsOnly, Category = "Mansion")
	int32 MaxPlayers = 6;

private:
	void BeginPlayingPhase();
	void OnMatchTimerExpired();
	bool HasHouse() const;

	FTimerHandle PhaseTimer;
	int32 NumPlayersJoined = 0;
};
