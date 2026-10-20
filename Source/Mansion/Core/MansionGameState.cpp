#include "Core/MansionGameState.h"
#include "Net/UnrealNetwork.h"

void AMansionGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMansionGameState, Phase);
	DOREPLIFETIME(AMansionGameState, PhaseEndServerTime);
}

float AMansionGameState::GetPhaseTimeRemaining() const
{
	if (PhaseEndServerTime < 0.0)
	{
		return -1.f;
	}
	return static_cast<float>(FMath::Max(0.0, PhaseEndServerTime - GetServerWorldTimeSeconds()));
}

void AMansionGameState::SetPhase(EMansionPhase NewPhase, float Duration)
{
	check(HasAuthority());

	PhaseEndServerTime = Duration > 0.f ? GetServerWorldTimeSeconds() + Duration : -1.0;
	Phase = NewPhase;

	// RepNotifies don't fire on the server, so broadcast locally too.
	OnRep_Phase();
}

void AMansionGameState::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(Phase);
}
