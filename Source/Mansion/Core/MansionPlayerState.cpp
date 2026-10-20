#include "Core/MansionPlayerState.h"
#include "Net/UnrealNetwork.h"

void AMansionPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMansionPlayerState, MansionRole);
	DOREPLIFETIME(AMansionPlayerState, SurvivorStatus);
}

void AMansionPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	if (AMansionPlayerState* MansionPlayerState = Cast<AMansionPlayerState>(PlayerState))
	{
		MansionPlayerState->MansionRole = MansionRole;
	}
}
