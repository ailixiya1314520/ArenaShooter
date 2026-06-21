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

	// 【核心优化】强行限制该任务在行为树编辑器中，只能选择 Vector 类型的黑板键！
	// 这样你就绝对不会再因为选错成 TargetActor 或 SelfActor 而导致寻路崩溃了。
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

	// 【逻辑优化】获取偏向目标的优先级判定
	UObject* TargetObject = BlackboardComp->GetValueAsObject(TEXT("TargetActor"));
	if (AActor* TargetActor = Cast<AActor>(TargetObject))
	{
		// 1. 如果正在战斗中，死死盯住玩家方向
		SearchDirection = (TargetActor->GetActorLocation() - OriginLocation).GetSafeNormal();
	}
	else if (BlackboardComp->IsVectorValueSet(TEXT("LastKnownLocation")))
	{
		// 2. 如果玩家刚消失，朝玩家最后已知位置的方向偏向（智能索敌）
		FVector LastKnown = BlackboardComp->GetValueAsVector(TEXT("LastKnownLocation"));
		SearchDirection = (LastKnown - OriginLocation).GetSafeNormal();
	}
	else
	{
		// 3. 彻底日常巡逻状态，朝自身前方偏向
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

	// 【空间优化】使用精确投影代替大范围随机，Z轴容差严格限制为 NavSearchRadius
	FVector ProjectionExtent(NavSearchRadius, NavSearchRadius, NavSearchRadius);
	bool bFoundPoint = NavSys->ProjectPointToNavigation(TargetPoint, PatrolNavLocation, ProjectionExtent);

	if (bFoundPoint)
	{
		BlackboardComp->SetValueAsVector(BlackboardKey.SelectedKeyName, PatrolNavLocation.Location);
		return EBTNodeResult::Succeeded;
	}

	// 【兜底优化】如果算出的点在悬崖外，或者被墙卡死了，为了防止行为树在一帧内死循环
	// 我们直接在 AI 原地脚边绝对安全的范围内，找一个落脚点敷衍过去。
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