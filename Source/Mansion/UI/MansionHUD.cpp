#include "UI/MansionHUD.h"
#include "Core/MansionGameState.h"
#include "Core/MansionPlayerState.h"
#include "House/HousePawn.h"
#include "Survivor/ExposureComponent.h"
#include "Survivor/FearComponent.h"
#include "Survivor/SurvivorCharacter.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"

static TAutoConsoleVariable<bool> CVarShowFear(
	TEXT("mansion.ShowFear"),
	false,
	TEXT("Show raw Fear, roll bonus and illumination numbers on the Survivor HUD."),
	ECVF_Cheat);

void AMansionHUD::DrawLine(const FString& Text, const FLinearColor& Color)
{
	DrawText(Text, Color, 40.f, CursorY, nullptr, 1.2f);
	CursorY += 24.f;
}

void AMansionHUD::DrawHUD()
{
	Super::DrawHUD();

	CursorY = 40.f;
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (const AMansionGameState* GameState = GetWorld()->GetGameState<AMansionGameState>())
	{
		FString Line = UEnum::GetDisplayValueAsText(GameState->GetPhase()).ToString();
		const float Remaining = GameState->GetPhaseTimeRemaining();
		if (Remaining >= 0.f)
		{
			const int32 Seconds = FMath::CeilToInt(Remaining);
			Line += FString::Printf(TEXT("  %d:%02d"), Seconds / 60, Seconds % 60);
		}
		DrawLine(Line);
	}

	if (const AMansionPlayerState* PlayerState = PlayerController->GetPlayerState<AMansionPlayerState>())
	{
		DrawLine(FString::Printf(TEXT("Role: %s"), *UEnum::GetDisplayValueAsText(PlayerState->GetMansionRole()).ToString()), FLinearColor::Gray);
	}

	if (const ASurvivorCharacter* Survivor = Cast<ASurvivorCharacter>(PlayerController->GetPawn()))
	{
		DrawSurvivor(*Survivor);
	}
	else if (const AHousePawn* House = Cast<AHousePawn>(PlayerController->GetPawn()))
	{
		DrawHouse(*House);
	}
}

void AMansionHUD::DrawSurvivor(const ASurvivorCharacter& Survivor)
{
	// Crosshair dot.
	if (Canvas)
	{
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.6f), Canvas->ClipX * 0.5f - 2.f, Canvas->ClipY * 0.5f - 2.f, 4.f, 4.f);
	}

	if (const UFearComponent* Fear = Survivor.GetFear())
	{
		const float Alpha = Fear->GetFearAlpha();
		const TCHAR* Feeling = Alpha < 0.25f ? TEXT("Calm") : Alpha < 0.5f ? TEXT("Uneasy") : Alpha < 0.75f ? TEXT("Afraid") : TEXT("Terrified");
		DrawLine(FString::Printf(TEXT("You feel: %s"), Feeling), FLinearColor::LerpUsingHSV(FLinearColor::White, FLinearColor::Red, Alpha));

		if (CVarShowFear.GetValueOnGameThread())
		{
			const UExposureComponent* Exposure = Survivor.GetExposure();
			DrawLine(FString::Printf(TEXT("Fear %.0f  roll bonus +%.2f%%  light %.2f  still %.1fs"),
				Fear->GetFear(), Fear->GetRollBonus() * 100.f,
				Exposure ? Exposure->GetIllumination() : 0.f,
				Exposure ? Exposure->GetStationaryTime() : 0.f), FLinearColor::Yellow);
		}
	}

	if (const UExposureComponent* Exposure = Survivor.GetExposure())
	{
		if (Exposure->HasRevealReason(ERevealReason::SeenByMonster))
		{
			DrawLine(TEXT("Something has seen you!"), FLinearColor::Red);
		}
		if (Exposure->HasRevealReason(ERevealReason::Dark))
		{
			DrawLine(TEXT("It's too dark... the House can see you."), FLinearColor(0.7f, 0.4f, 1.f));
		}
		if (Exposure->HasRevealReason(ERevealReason::Camping))
		{
			DrawLine(TEXT("You've been still too long... you feel watched."), FLinearColor(1.f, 0.85f, 0.1f));
		}
	}

	DrawLine(TEXT("WASD move  Shift sprint  Ctrl/C crouch  F flashlight"), FLinearColor(0.5f, 0.5f, 0.5f));
}

void AMansionHUD::DrawHouse(const AHousePawn& House)
{
	DrawLine(FString::Printf(TEXT("Floor %d / %d"), House.GetCurrentFloor(), House.GetNumFloors()));
	DrawLine(TEXT("Red = seen by monster   Purple = in darkness   Yellow = camping"), FLinearColor(0.7f, 0.7f, 0.7f));
	DrawLine(TEXT("WASD pan  Wheel zoom  Q/E floor down/up"), FLinearColor(0.5f, 0.5f, 0.5f));
}
