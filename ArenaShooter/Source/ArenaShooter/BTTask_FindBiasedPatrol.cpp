#include "BTTask_FindBiasedPatrol.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h" // 引入类型锁
#include "AIController.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"

UBTTask_FindBiasedPatrol::UBTTask_FindBiasedPatrol()
{
	NodeName = TEXT("Find Biased Patrol (Optimized)");

	// 默认参数调整为适应复杂地形的最佳实践
	PatrolRadius = 500.0f;
	ConeHalfAngle = 45.0f;
	NavSearchRadius = 150.0f;
	bEnableFallbackRandom = true;
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindBiasedPatrol, BlackboardKey));
}

EBTNodeResult::Type UBTTask_FindBiasedPatrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp) return EBTNodeResult::Failed;

	APawn* AIPawn = AIController->GetPawn();
	if (!AIPawn) return EBTNodeResult::Failed;

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys) return EBTNodeResult::Failed;

	FVector OriginLocation = AIPawn->GetActorLocation();
	FVector SearchDirection;

	UObject* TargetObject = BlackboardComp->GetValueAsObject(TEXT("TargetActor"));
	if (AActor* TargetActor = Cast<AActor>(TargetObject))
	{
		// 1. 如果正在战斗中，死死盯住玩家方向
		SearchDirection = (TargetActor->GetActorLocation() - OriginLocation).GetSafeNormal();
	}
	else if (BlackboardComp->IsVectorValueSet(TEXT("LastKnownLocation")))
	{
		// 2. 如果玩家刚消失，朝玩家最后已知位置的方向偏向
		FVector LastKnown = BlackboardComp->GetValueAsVector(TEXT("LastKnownLocation"));
		SearchDirection = (LastKnown - OriginLocation).GetSafeNormal();
	}
	else
	{
		SearchDirection = AIPawn->GetActorForwardVector();
	}

	// 抹平 Z 轴（高度），防止高低差导致计算出的坐标在地下或天上
	SearchDirection.Z = 0.0f;
	SearchDirection.Normalize();

	// 加入随机圆锥偏移
	float ConeHalfAngleRad = FMath::DegreesToRadians(ConeHalfAngle);
	FVector BiasedDirection = FMath::VRandCone(SearchDirection, ConeHalfAngleRad);

	// 计算最终期望达到的绝对坐标
	FVector TargetPoint = OriginLocation + (BiasedDirection * PatrolRadius);
	FNavLocation PatrolNavLocation;

	FVector ProjectionExtent(NavSearchRadius, NavSearchRadius, NavSearchRadius);
	bool bFoundPoint = NavSys->ProjectPointToNavigation(TargetPoint, PatrolNavLocation, ProjectionExtent);

	if (bFoundPoint)
	{
		BlackboardComp->SetValueAsVector(BlackboardKey.SelectedKeyName, PatrolNavLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	if (bEnableFallbackRandom)
	{
		if (NavSys->GetRandomReachablePointInRadius(OriginLocation, PatrolRadius * 0.5f, PatrolNavLocation))
		{
			BlackboardComp->SetValueAsVector(BlackboardKey.SelectedKeyName, PatrolNavLocation.Location);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}