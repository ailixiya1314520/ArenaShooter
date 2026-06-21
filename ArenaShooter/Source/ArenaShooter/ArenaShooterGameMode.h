#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaShooterGameMode.generated.h"

UCLASS(minimalapi)
class AArenaShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaShooterGameMode();

	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|Score")
	int32 CurrentScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Score")
	int32 TargetScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode|Time")
	int32 MatchTimeLimit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|Time")
	int32 TimeRemaining;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|State")
	bool bIsMatchEnded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|Spawn")
	int32 SpawnedPlayerCount;

	UFUNCTION(BlueprintCallable, Category = "GameMode|Score")
	void AddScore(int32 Points);

	UFUNCTION(BlueprintCallable, Category = "GameMode|State")
	void NotifyPlayerDeath(AController* DeadPlayer);

	UFUNCTION(BlueprintCallable, Category = "GameMode|State")
	void TriggerExtractionVictory();

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode|Events")
	void OnMatchVictory();

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode|Events")
	void OnMatchDefeat();

protected:
	void OnVictory();
	void OnDefeat();
	void OnMatchTimerTick();
	void CheckAlivePlayers();

	FTimerHandle MatchTimerHandle;
};