#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AIC_ArenaEnemy.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTree;

UCLASS()
class ARENASHOOTER_API AAIC_ArenaEnemy : public AAIController
{
	GENERATED_BODY()

public:
	AAIC_ArenaEnemy();
	virtual void OnPossess(APawn* InPawn) override;

protected:
	FTimerHandle TimerHandle_ForgetPlayer;

	UFUNCTION()
	void ForgetPlayer();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBehaviorTreeComponent* BehaviorTreeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBlackboardComponent* BlackboardComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	UAISenseConfig_Hearing* HearingConfig;

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Team")
	float AlertRadius = 2000.0f; // ºô½Ð¶ÓÓÑµÄ°ë¾¶·¶Î§

	UFUNCTION()
	void AlertNearbyEnemies(AActor* SpottedTarget);
};