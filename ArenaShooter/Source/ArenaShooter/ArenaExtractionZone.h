#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaExtractionZone.generated.h"

class UBoxComponent;
class UWidgetComponent;

UCLASS()
class ARENASHOOTER_API AArenaExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	AArenaExtractionZone();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* OverlapComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* WaypointWidgetComp;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};