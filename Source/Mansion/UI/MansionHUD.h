#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MansionHUD.generated.h"

class AHousePawn;
class ASurvivorCharacter;

/**
 * Greybox HUD drawn with the canvas: phase timer, role, and either the Survivor's
 * exposure warnings / fear or the House's floor and legend. Replace with UMG later.
 */
UCLASS()
class MANSION_API AMansionHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawLine(const FString& Text, const FLinearColor& Color = FLinearColor::White);
	void DrawSurvivor(const ASurvivorCharacter& Survivor);
	void DrawHouse(const AHousePawn& House);

	float CursorY = 0.f;
};
