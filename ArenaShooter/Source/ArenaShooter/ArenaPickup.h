#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UGameplayEffect;

UCLASS()
class ARENASHOOTER_API AArenaPickup : public AActor
{
	GENERATED_BODY()

public:
	AArenaPickup();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	// 核心：这个道具被吃掉时，给玩家施加什么效果？
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> PickupEffect;

	// 拾取时的音效和特效
	UPROPERTY(EditAnywhere, Category = "Effects")
	class USoundBase* PickupSound;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UParticleSystem* PickupVFX;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};