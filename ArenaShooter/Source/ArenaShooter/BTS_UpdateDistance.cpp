#include "BTS_UpdateDistance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"

UBTS_UpdateDistance::UBTS_UpdateDistance()
{
	NodeName = TEXT("Update Distance");
	bNotifyTick = true;
}

void UBTS_UpdateDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return;

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn) return;

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!TargetActor) return;

	float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());

	BlackboardComp->SetValueAsFloat(DistanceKey.SelectedKeyName, Distance);
}