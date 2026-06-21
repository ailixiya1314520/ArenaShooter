#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArenaEnemyCharacter.generated.h"

UCLASS()
class ARENASHOOTER_API AArenaEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AArenaEnemyCharacter();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Reward")
	int32 RewardScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", ReplicatedUsing = OnRep_Health)
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* HealthBarWidget;

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void UpdateHealthBarUI(float CurrentHealth, float MaxHP);

	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	class UParticleSystem* MuzzleFlashFX;

	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	class UParticleSystem* TracerFX;

	UPROPERTY(EditAnywhere, Category = "Combat|VFX")
	class UParticleSystem* ImpactFX;

	UPROPERTY(EditAnywhere, Category = "Combat|Audio")
	class USoundBase* FireSound;

	UPROPERTY(EditAnywhere, Category = "Combat|Audio")
	class USoundBase* HurtSound;

	UPROPERTY(EditAnywhere, Category = "Combat|Audio")
	class USoundBase* DeathSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead;

	UFUNCTION()
	void OnRep_Health();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void Die(AController* Killer);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void PerformAttack(AActor* TargetPlayer);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAttackFX(FVector StartLoc, FVector TracerEndPoint);
};