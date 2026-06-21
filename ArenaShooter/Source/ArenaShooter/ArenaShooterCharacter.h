#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "GameplayPrediction.h"
#include "ArenaShooterCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class UAbilitySystemComponent;
class UArenaAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UPawnNoiseEmitterComponent;
struct FOnAttributeChangeData;
class USkeletalMesh;
class AActor;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedDelegate, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedDelegate, float, CurrentAmmo, float, MaxAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadStartedDelegate, float, ReloadDuration);

USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponName = TEXT("DefaultGun");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	USkeletalMesh* WeaponMeshModel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AActor> PickupBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<class AArenaShooterProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float FireRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilPitch = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float RecoilYaw = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 BulletsPerShot = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	float SpreadAngle = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
	float MaxAmmo = 30.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
	float CurrentAmmo = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo")
	float ReloadTime = 2.0f;
};

UCLASS(config = Game)
class AArenaShooterCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* EquippedWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AI, meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* NoiseEmitterComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleSmartFireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchWeaponAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> DashAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayAbility> ReloadAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> FireCostGEClass;

public:
	AArenaShooterCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	UArenaAttributeSet* AttributeSet;

	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnHealthChangedDelegate OnHealthUpdate;

	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAmmoChangedDelegate OnAmmoUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnReloadStartedDelegate OnReloadStarted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Combat|Weapon")
	TArray<FWeaponStats> WeaponInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentWeaponIndex, Category = "Combat|Weapon")
	int32 CurrentWeaponIndex = 0;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CurrentWeaponIndex();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Fire(FPredictionKey PredictionKey);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_FireFX();

	void PlayFireFX();

	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnAmmoChanged(const FOnAttributeChangeData& Data);
	void OnMaxAmmoChanged(const FOnAttributeChangeData& Data);
	void Die();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game|Death")
	void OnPlayerDeath();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void AddWeapon(FWeaponStats NewWeapon);

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void DropCurrentWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DropCurrentWeapon();

	UFUNCTION(BlueprintCallable, Category = "Combat|Weapon")
	void ReloadWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ReloadWeapon();

	void FinishReload();
	void EquipWeapon(int32 Index);
	void SwitchNextWeapon();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SwitchNextWeapon();

	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void GiveAbilities();
	void OnDashStarted();
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	FTimerHandle TimerHandle_AutoFire;
	FTimerHandle TimerHandle_SmartFire;
	FTimerHandle TimerHandle_Reload;

	bool bSmartFireMode;

	UPROPERTY(Replicated)
	bool bIsReloading;

	void StartFire();
	void StopFire();
	void ExecuteFire();

	void ToggleSmartFire();
	void SmartFireTick();
};