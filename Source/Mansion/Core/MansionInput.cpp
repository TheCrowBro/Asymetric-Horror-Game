#include "Core/MansionInput.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace MansionInput
{
	UInputAction* MakeAction(UObject* Outer, FName Name, EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = ValueType;
		return Action;
	}

	void MapKey(UInputMappingContext* Context, UInputAction* Action, const FKey& Key, bool bSwizzle, bool bNegateX, bool bNegateY)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);

		if (bSwizzle)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegateX || bNegateY)
		{
			UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Context);
			Negate->bX = bNegateX;
			Negate->bY = bNegateY;
			Negate->bZ = false;
			Mapping.Modifiers.Add(Negate);
		}
	}

	void MapWASD(UInputMappingContext* Context, UInputAction* Action)
	{
		MapKey(Context, Action, EKeys::W, /*bSwizzle*/ true);
		MapKey(Context, Action, EKeys::S, /*bSwizzle*/ true, /*bNegateX*/ false, /*bNegateY*/ true);
		MapKey(Context, Action, EKeys::D);
		MapKey(Context, Action, EKeys::A, /*bSwizzle*/ false, /*bNegateX*/ true);
	}

	void ApplyContext(const APawn* Pawn, const UInputMappingContext* Context)
	{
		const APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		if (!PlayerController || !Context)
		{
			return;
		}
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(Context, 0);
		}
	}
}
