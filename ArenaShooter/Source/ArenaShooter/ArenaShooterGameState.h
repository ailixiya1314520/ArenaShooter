#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ArenaShooterGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreUpdatedDelegate, int32, CurrentScore, int32, TargetScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdatedDelegate, int32, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchEndedDelegate, bool, bIsVictory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionUpdatedDelegate, const FString&, NewObjective);

UCLASS()
class ARENASHOOTER_API AArenaShooterGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AArenaShooterGameState();

	UPROPERTY(ReplicatedUsing = OnRep_CurrentScore, BlueprintReadOnly, Category = "Game|State")
	int32 CurrentScore;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Game|State")
	int32 TargetScore;

	UPROPERTY(ReplicatedUsing = OnRep_TimeRemaining, BlueprintReadOnly, Category = "Game|State")
	int32 TimeRemaining;

	UPROPERTY(ReplicatedUsing = OnRep_MissionObjective, BlueprintReadOnly, Category = "Game|State")
	FString MissionObjective;

	UPROPERTY(BlueprintAssignable, Category = "Game|Events")
	FOnScoreUpdatedDelegate OnScoreUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Game|Events")
	FOnTimeUpdatedDelegate OnTimeUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Game|Events")
	FOnMatchEndedDelegate OnMatchEnded;

	UPROPERTY(BlueprintAssignable, Category = "Game|Events")
	FOnMissionUpdatedDelegate OnMissionObjectiveUpdated;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentScore();

	UFUNCTION()
	void OnRep_TimeRemaining();

	UFUNCTION()
	void OnRep_MissionObjective();

	// 核心：全服强制广播游戏结束
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_MatchEnded(bool bIsVictory);
};