#include "BTTask_FindSearchPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "AIController.h"
#include "NavigationSystem.h"

UBTTask_FindSearchPoint::UBTTask_FindSearchPoint()
{
	NodeName = TEXT("Find Search Point");
	SearchRadius = 600.0f;
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindSearchPoint, BlackboardKey));
}

EBTNodeResult::Type UBTTask_FindSearchPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp) return EBTNodeResult::Failed;

	FVector OriginLocation = AIController->GetPawn()->GetActorLocation();
	if (BlackboardComp->IsVectorValueSet(TEXT("LastKnownLocation")))
	{
		OriginLocation = BlackboardComp->GetValueAsVector(TEXT("LastKnownLocation"));
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	FNavLocation SearchNavLocation;
	// 在锚点周围获取一个随机的、可到达的导航网格坐标
	bool bFoundPoint = NavSys->GetRandomReachablePointInRadius(OriginLocation, SearchRadius, SearchNavLocation);

	if (bFoundPoint)
	{
		BlackboardComp->SetValueAsVector(BlackboardKey.SelectedKeyName, SearchNavLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}