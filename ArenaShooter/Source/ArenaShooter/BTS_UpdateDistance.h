#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTS_UpdateDistance.generated.h"

UCLASS()
class ARENASHOOTER_API UBTS_UpdateDistance : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTS_UpdateDistance();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	struct FBlackboardKeySelector DistanceKey;
};