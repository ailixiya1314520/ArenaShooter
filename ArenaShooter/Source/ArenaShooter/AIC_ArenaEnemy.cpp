#include "AIC_ArenaEnemy.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "ArenaShooterCharacter.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"
#include "ArenaEnemyCharacter.h" 
AAIC_ArenaEnemy::AAIC_ArenaEnemy()
{
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 900.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 3000.0f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->ConfigureSense(*HearingConfig);
	AIPerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AAIC_ArenaEnemy::OnTargetDetected);
}

void AAIC_ArenaEnemy::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		BlackboardComp->InitializeBlackboard(*BehaviorTreeAsset->BlackboardAsset);
		BehaviorTreeComponent->StartTree(*BehaviorTreeAsset);
	}
}

void AAIC_ArenaEnemy::OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus)
{
	if (Actor && Actor->ActorHasTag(FName("Dead")))
	{
		if (BlackboardComp->GetValueAsObject(TEXT("TargetActor")) == Actor)
		{
			BlackboardComp->ClearValue(TEXT("TargetActor"));
		}
		return;
	}

	if (AArenaShooterCharacter* Player = Cast<AArenaShooterCharacter>(Actor))
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
			{
				GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ForgetPlayer);
				BlackboardComp->SetValueAsObject(TEXT("TargetActor"), Player);

				AlertNearbyEnemies(Player);
			}
			else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
			{
				if (BlackboardComp->GetValueAsObject(TEXT("TargetActor")) == nullptr)
				{
					BlackboardComp->SetValueAsVector(TEXT("LastKnownLocation"), Stimulus.StimulusLocation);
				}
			}
		}
		else
		{
			if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
			{
				BlackboardComp->SetValueAsVector(TEXT("LastKnownLocation"), Stimulus.StimulusLocation);
				GetWorld()->GetTimerManager().SetTimer(TimerHandle_ForgetPlayer, this, &AAIC_ArenaEnemy::ForgetPlayer, 3.0f, false);
			}
		}
	}
}

void AAIC_ArenaEnemy::AlertNearbyEnemies(AActor* SpottedTarget)
{
	if (!SpottedTarget || !GetPawn()) return;

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AArenaEnemyCharacter::StaticClass(), FoundEnemies);

	FVector MyLocation = GetPawn()->GetActorLocation();

	for (AActor* EnemyActor : FoundEnemies)
	{
		AArenaEnemyCharacter* EnemyChar = Cast<AArenaEnemyCharacter>(EnemyActor);
		// 过滤掉自己和已经死亡的敌人
		if (EnemyChar && EnemyChar != GetPawn() && !EnemyChar->bIsDead)
		{
			float Distance = FVector::Dist(MyLocation, EnemyChar->GetActorLocation());
			if (Distance <= AlertRadius)
			{
				if (AAIC_ArenaEnemy* FriendAI = Cast<AAIC_ArenaEnemy>(EnemyChar->GetController()))
				{
					if (UBlackboardComponent* FriendBB = FriendAI->GetBlackboardComponent())
					{
						if (FriendBB->GetValueAsObject(TEXT("TargetActor")) == nullptr)
						{
							FriendBB->SetValueAsObject(TEXT("TargetActor"), SpottedTarget);
							FriendBB->SetValueAsVector(TEXT("LastKnownLocation"), SpottedTarget->GetActorLocation());
						}
					}
				}
			}
		}
	}
}

void AAIC_ArenaEnemy::ForgetPlayer()
{
	if (BlackboardComp)
	{
		BlackboardComp->ClearValue(TEXT("TargetActor"));
	}
}