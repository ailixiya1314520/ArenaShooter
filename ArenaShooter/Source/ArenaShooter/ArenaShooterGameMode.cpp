#include "ArenaShooterGameMode.h"
#include "ArenaShooterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "ArenaShooterGameState.h"
#include "GameFramework/PlayerStart.h"

AArenaShooterGameMode::AArenaShooterGameMode()
	: Super()
{
	CurrentScore = 0;
	TargetScore = 400;
	MatchTimeLimit = 200;
	TimeRemaining = MatchTimeLimit;
	bIsMatchEnded = false;
	SpawnedPlayerCount = 0;
}

void AArenaShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	CurrentScore = 0;
	TimeRemaining = MatchTimeLimit;
	bIsMatchEnded = false;
	SpawnedPlayerCount = 0;

	if (AArenaShooterGameState* GS = GetGameState<AArenaShooterGameState>())
	{
		GS->TargetScore = TargetScore;
		GS->CurrentScore = CurrentScore;
		GS->TimeRemaining = TimeRemaining;
		GS->MissionObjective = TEXT("Fight Or Go");
	}

	GetWorldTimerManager().SetTimer(MatchTimerHandle, this, &AArenaShooterGameMode::OnMatchTimerTick, 1.0f, true);
}

AActor* AArenaShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	FName SpawnTag = (SpawnedPlayerCount == 0) ? FName("Spawn_Player1") : FName("Spawn_Player2");
	SpawnedPlayerCount++;

	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), SpawnTag, FoundStarts);

	if (FoundStarts.Num() > 0)
	{
		return FoundStarts[0];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void AArenaShooterGameMode::AddScore(int32 Points)
{
	if (bIsMatchEnded) return;

	CurrentScore += Points;

	if (AArenaShooterGameState* GS = GetGameState<AArenaShooterGameState>())
	{
		GS->CurrentScore = CurrentScore;
		GS->OnScoreUpdated.Broadcast(GS->CurrentScore, GS->TargetScore);
	}

	if (CurrentScore >= TargetScore)
	{
		OnVictory();
	}
}

void AArenaShooterGameMode::OnMatchTimerTick()
{
	if (bIsMatchEnded) return;

	TimeRemaining--;

	if (AArenaShooterGameState* GS = GetGameState<AArenaShooterGameState>())
	{
		GS->TimeRemaining = TimeRemaining;
		GS->OnTimeUpdated.Broadcast(GS->TimeRemaining);
	}

	if (TimeRemaining <= 0)
	{
		OnDefeat();
	}
}

void AArenaShooterGameMode::NotifyPlayerDeath(AController* DeadPlayer)
{
	if (bIsMatchEnded) return;
	CheckAlivePlayers();
}

void AArenaShooterGameMode::TriggerExtractionVictory()
{
	if (bIsMatchEnded) return;

	int32 PointsNeeded = TargetScore - CurrentScore;
	if (PointsNeeded > 0)
	{
		AddScore(PointsNeeded);
	}
	else
	{
		OnVictory();
	}
}

void AArenaShooterGameMode::CheckAlivePlayers()
{
	if (bIsMatchEnded) return;

	bool bHasAlivePlayer = false;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->GetPawn())
		{
			AArenaShooterCharacter* PlayerChar = Cast<AArenaShooterCharacter>(PC->GetPawn());
			if (PlayerChar && PlayerChar->InputEnabled())
			{
				bHasAlivePlayer = true;
				break;
			}
		}
	}

	if (!bHasAlivePlayer)
	{
		OnDefeat();
	}
}

void AArenaShooterGameMode::OnVictory()
{
	bIsMatchEnded = true;
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);

	if (AArenaShooterGameState* GS = GetGameState<AArenaShooterGameState>())
	{
		GS->Multicast_MatchEnded(true);
	}
	OnMatchVictory();
}

void AArenaShooterGameMode::OnDefeat()
{
	bIsMatchEnded = true;
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);

	if (AArenaShooterGameState* GS = GetGameState<AArenaShooterGameState>())
	{
		GS->Multicast_MatchEnded(false);
	}
	OnMatchDefeat();
}