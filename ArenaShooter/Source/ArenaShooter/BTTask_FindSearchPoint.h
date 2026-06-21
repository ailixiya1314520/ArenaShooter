#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_FindSearchPoint.generated.h"

UCLASS()
class ARENASHOOTER_API UBTTask_FindSearchPoint : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_FindSearchPoint();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	// 扫荡的半径范围（建议设为 600 - 800）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SearchConfig")
	float SearchRadius;
};