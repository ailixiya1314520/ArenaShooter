#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindBiasedPatrol.generated.h"

UCLASS()
class ARENASHOOTER_API UBTTask_FindBiasedPatrol : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindBiasedPatrol();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	// 基础巡逻移动半径
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatrolConfig")
	float PatrolRadius;

	// 偏向锥体半角 (越小越精确指向玩家，默认45度)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatrolConfig", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ConeHalfAngle;

	// 导航网格探测容差 (防止掉下悬崖或跨楼层，建议设为 100~200)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatrolConfig")
	float NavSearchRadius;

	// 是否开启悬崖防卡死兜底？(强烈建议开启)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PatrolConfig")
	bool bEnableFallbackRandom;
};