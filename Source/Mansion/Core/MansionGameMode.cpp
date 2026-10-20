#include "Core/MansionGameMode.h"
#include "Mansion.h"
#include "Core/MansionGameState.h"
#include "Core/MansionPlayerState.h"
#include "Core/MansionTuning.h"
#include "House/HousePawn.h"
#include "Survivor/SurvivorCharacter.h"
#include "UI/MansionHUD.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<int32> CVarHouseSlot(
	TEXT("mansion.HouseSlot"),
	1,
	TEXT("Join index of the player who becomes the House (0 = listen-server host). Ignored if someone joins with ?Role=House."),
	ECVF_Default);

const FName AMansionGameMode::HouseStartTag(TEXT("House"));

AMansionGameMode::AMansionGameMode()
{
	GameStateClass = AMansionGameState::StaticClass();
	PlayerStateClass = AMansionPlayerState::StaticClass();
	HUDClass = AMansionHUD::StaticClass();
	SurvivorPawnClass = ASurvivorCharacter::StaticClass();
	HousePawnClass = AHousePawn::StaticClass();
	DefaultPawnClass = SurvivorPawnClass;
}

void AMansionGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (ErrorMessage.IsEmpty() && GameState && GameState->PlayerArray.Num() >= MaxPlayers)
	{
		ErrorMessage = TEXT("The mansion is full.");
	}
}

FString AMansionGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	// Role must be set before Super, which picks a start spot based on it.
	if (AMansionPlayerState* PlayerState = NewPlayerController ? NewPlayerController->GetPlayerState<AMansionPlayerState>() : nullptr)
	{
		const FString RequestedRole = UGameplayStatics::ParseOption(Options, TEXT("Role"));
		const bool bWantsHouse = RequestedRole.Equals(TEXT("House"), ESearchCase::IgnoreCase)
			|| (RequestedRole.IsEmpty() && NumPlayersJoined == CVarHouseSlot.GetValueOnGameThread());

		const EMansionRole Role = (bWantsHouse && !HasHouse()) ? EMansionRole::House : EMansionRole::Survivor;
		PlayerState->SetMansionRole(Role);

		UE_LOG(LogMansion, Log, TEXT("Player %d (%s) joined as %s"),
			NumPlayersJoined, *PlayerState->GetPlayerName(), *UEnum::GetValueAsString(Role));
	}
	++NumPlayersJoined;

	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

UClass* AMansionGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	const AMansionPlayerState* PlayerState = InController ? InController->GetPlayerState<AMansionPlayerState>() : nullptr;
	if (PlayerState && PlayerState->GetMansionRole() == EMansionRole::House)
	{
		return HousePawnClass;
	}
	return SurvivorPawnClass;
}

AActor* AMansionGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const AMansionPlayerState* PlayerState = Player ? Player->GetPlayerState<AMansionPlayerState>() : nullptr;
	const bool bIsHouse = PlayerState && PlayerState->GetMansionRole() == EMansionRole::House;

	TArray<APlayerStart*> Candidates;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if ((It->PlayerStartTag == HouseStartTag) == bIsHouse)
		{
			Candidates.Add(*It);
		}
	}

	if (Candidates.IsEmpty())
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}
	if (bIsHouse)
	{
		return Candidates[0];
	}

	// Prefer a Survivor start nobody is standing on.
	const int32 Offset = FMath::RandRange(0, Candidates.Num() - 1);
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		APlayerStart* Start = Candidates[(Index + Offset) % Candidates.Num()];
		bool bOccupied = false;
		for (TActorIterator<APawn> PawnIt(GetWorld()); PawnIt; ++PawnIt)
		{
			if (FVector::DistSquared(PawnIt->GetActorLocation(), Start->GetActorLocation()) < FMath::Square(100.f))
			{
				bOccupied = true;
				break;
			}
		}
		if (!bOccupied)
		{
			return Start;
		}
	}
	return Candidates[Offset];
}

void AMansionGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	const float PreGameDuration = UMansionTuning::Get().PreGameDuration;
	GetGameState<AMansionGameState>()->SetPhase(EMansionPhase::PreGame, PreGameDuration);

	if (PreGameDuration > 0.f)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &AMansionGameMode::BeginPlayingPhase, PreGameDuration, false);
	}
	else
	{
		BeginPlayingPhase();
	}
}

void AMansionGameMode::BeginPlayingPhase()
{
	const float MatchDuration = UMansionTuning::Get().MatchDuration;
	GetGameState<AMansionGameState>()->SetPhase(EMansionPhase::Playing, MatchDuration);
	GetWorldTimerManager().SetTimer(PhaseTimer, this, &AMansionGameMode::OnMatchTimerExpired, MatchDuration, false);

	UE_LOG(LogMansion, Log, TEXT("Match started: %.0f seconds until the Magic Door opens"), MatchDuration);
}

void AMansionGameMode::OnMatchTimerExpired()
{
	// Magic Door actor and Final Hunt Dread effect hook in here (milestones 6 and 9).
	GetGameState<AMansionGameState>()->SetPhase(EMansionPhase::MagicDoorOpen, 0.f);

	UE_LOG(LogMansion, Log, TEXT("Timer expired: Magic Door open, Final Hunt begins"));
}

bool AMansionGameMode::HasHouse() const
{
	if (!GameState)
	{
		return false;
	}
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AMansionPlayerState* MansionPlayerState = Cast<AMansionPlayerState>(PlayerState);
		if (MansionPlayerState && MansionPlayerState->GetMansionRole() == EMansionRole::House)
		{
			return true;
		}
	}
	return false;
}
