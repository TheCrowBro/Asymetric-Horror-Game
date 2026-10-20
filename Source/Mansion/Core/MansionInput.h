#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"

class UInputAction;
class UInputMappingContext;
class APawn;
struct FKey;

/**
 * Builds Enhanced Input actions and mapping contexts at runtime so the greybox
 * prototype needs no input assets. Swap for authored IA_/IMC_ assets whenever you like.
 */
namespace MansionInput
{
	MANSION_API UInputAction* MakeAction(UObject* Outer, FName Name, EInputActionValueType ValueType);

	/**
	 * Maps a key. For 2D actions driven by digital keys: bSwizzle moves the value onto Y
	 * (forward/back), bNegateX / bNegateY flip the respective axis.
	 */
	MANSION_API void MapKey(UInputMappingContext* Context, UInputAction* Action, const FKey& Key,
		bool bSwizzle = false, bool bNegateX = false, bool bNegateY = false);

	/** Standard WASD mapping onto a 2D action (W/S = Y, D/A = X). */
	MANSION_API void MapWASD(UInputMappingContext* Context, UInputAction* Action);

	/** Replaces all mappings on the pawn's local player with this context. */
	MANSION_API void ApplyContext(const APawn* Pawn, const UInputMappingContext* Context);
}
