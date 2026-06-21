#include "ArenaShooterGameState.h"
#include "Net/UnrealNetwork.h"

AArenaShooterGameState::AArenaShooterGameState()
{
	CurrentScore = 0;
	TargetScore = 500;
	TimeRemaining = 300;
	MissionObjective = TEXT("任务：击杀敌人获取积分，或前往撤离点！");
}

void AArenaShooterGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArenaShooterGameState, CurrentScore);
	DOREPLIFETIME(AArenaShooterGameState, TargetScore);
	DOREPLIFETIME(AArenaShooterGameState, TimeRemaining);
	DOREPLIFETIME(AArenaShooterGameState, MissionObjective);
}

void AArenaShooterGameState::OnRep_CurrentScore()
{
	OnScoreUpdated.Broadcast(CurrentScore, TargetScore);
}

void AArenaShooterGameState::OnRep_TimeRemaining()
{
	OnTimeUpdated.Broadcast(TimeRemaining);
}

void AArenaShooterGameState::OnRep_MissionObjective()
{
	OnMissionObjectiveUpdated.Broadcast(MissionObjective);
}

void AArenaShooterGameState::Multicast_MatchEnded_Implementation(bool bIsVictory)
{
	OnMatchEnded.Broadcast(bIsVictory);
}