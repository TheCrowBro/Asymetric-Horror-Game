#pragma once

#include "CoreMinimal.h"
#include "MansionTypes.generated.h"

/** Which side a player is on. Assigned by the game mode when they join. */
UENUM(BlueprintType)
enum class EMansionRole : uint8
{
	None,
	Survivor,
	House
};

/** Match flow. See AMansionGameMode for the transitions. */
UENUM(BlueprintType)
enum class EMansionPhase : uint8
{
	WaitingToStart,
	/** House places items and chooses the Magic Door; Survivors are frozen. */
	PreGame,
	Playing,
	/** Timer expired: Magic Door is open and the House is in Final Hunt. */
	MagicDoorOpen,
	Ended
};

UENUM(BlueprintType)
enum class ESurvivorStatus : uint8
{
	Alive,
	Downed,
	Eliminated,
	Escaped,
	Freed
};

/** Why the House can currently see a Survivor's exact position. Stored as a bitmask. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ERevealReason : uint8
{
	None = 0,
	Camping = 1 << 0,
	Dark = 1 << 1,
	SeenByMonster = 1 << 2,
	/** Scripted reveals, e.g. while channelling the ritual. */
	Forced = 1 << 3
};
ENUM_CLASS_FLAGS(ERevealReason);
