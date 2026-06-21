#include "ArenaShooterCharacter.h"
#include "ArenaShooterProjectile.h"
#include "ArenaEnemyCharacter.h" 
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystemComponent.h"
#include "ArenaAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "ArenaShooterGameMode.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemGlobals.h"
#include "Kismet/GameplayStatics.h"
#include "ArenaShooterPickUpComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AArenaShooterCharacter::AArenaShooterCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->bCastHiddenShadow = true;
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	EquippedWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("EquippedWeaponMesh"));
	EquippedWeaponMesh->SetupAttachment(FirstPersonCameraComponent);
	EquippedWeaponMesh->SetRelativeLocation(FVector(45.0f, 15.0f, -20.0f));
	EquippedWeaponMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	EquippedWeaponMesh->bCastDynamicShadow = false;
	EquippedWeaponMesh->CastShadow = false;

	NoiseEmitterComponent = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("NoiseEmitterComponent"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UArenaAttributeSet>(TEXT("AttributeSet"));

	bSmartFireMode = false;
	bIsReloading = false;
	CurrentWeaponIndex = 0;
}

void AArenaShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaShooterCharacter, WeaponInventory);
	DOREPLIFETIME(AArenaShooterCharacter, CurrentWeaponIndex);
	DOREPLIFETIME(AArenaShooterCharacter, bIsReloading);
}

void AArenaShooterCharacter::OnRep_CurrentWeaponIndex()
{
	EquipWeapon(CurrentWeaponIndex);
}

void AArenaShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent && AttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AArenaShooterCharacter::OnHealthChanged);

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetAmmoAttribute()).AddUObject(this, &AArenaShooterCharacter::OnAmmoChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxAmmoAttribute()).AddUObject(this, &AArenaShooterCharacter::OnMaxAmmoChanged);
	}

	if (HasAuthority())
	{
		EquipWeapon(CurrentWeaponIndex);
	}
}

void AArenaShooterCharacter::OnMaxAmmoChanged(const FOnAttributeChangeData& Data)
{
	if (AttributeSet)
	{
		OnAmmoUpdate.Broadcast(AttributeSet->GetAmmo(), Data.NewValue);
	}
}

void AArenaShooterCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (AttributeSet) OnHealthUpdate.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());
	if (Data.NewValue <= 0.0f) Die();
}

void AArenaShooterCharacter::OnAmmoChanged(const FOnAttributeChangeData& Data)
{
	if (AttributeSet) OnAmmoUpdate.Broadcast(Data.NewValue, AttributeSet->GetMaxAmmo());
}

void AArenaShooterCharacter::Die()
{
	Tags.Add(FName("Dead"));

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetVisibility(false);
	if (Mesh1P) Mesh1P->SetVisibility(false);
	if (EquippedWeaponMesh) EquippedWeaponMesh->SetVisibility(false);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC && HasAuthority())
	{
		PC->ChangeState(NAME_Spectating);
		PC->ClientGotoState(NAME_Spectating);
	}

	if (HasAuthority())
	{
		if (AArenaShooterGameMode* GM = Cast<AArenaShooterGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->NotifyPlayerDeath(PC);
		}
	}

	OnPlayerDeath();
}

UAbilitySystemComponent* AArenaShooterCharacter::GetAbilitySystemComponent() const { return AbilitySystemComponent; }

void AArenaShooterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		GiveAbilities();
	}
}

void AArenaShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (AbilitySystemComponent) AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void AArenaShooterCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AArenaShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArenaShooterCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AArenaShooterCharacter::Look);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AArenaShooterCharacter::OnDashStarted);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AArenaShooterCharacter::StartFire);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AArenaShooterCharacter::StopFire);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Canceled, this, &AArenaShooterCharacter::StopFire);
		EnhancedInputComponent->BindAction(ToggleSmartFireAction, ETriggerEvent::Started, this, &AArenaShooterCharacter::ToggleSmartFire);
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AArenaShooterCharacter::SwitchNextWeapon);
	}
}

void AArenaShooterCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AArenaShooterCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AArenaShooterCharacter::EquipWeapon(int32 Index)
{
	if (bIsReloading)
	{
		bIsReloading = false;
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Reload);
	}

	if (WeaponInventory.IsValidIndex(Index) && EquippedWeaponMesh)
	{
		EquippedWeaponMesh->SetSkeletalMesh(WeaponInventory[Index].WeaponMeshModel);

		if (AbilitySystemComponent && HasAuthority())
		{
			AbilitySystemComponent->SetNumericAttributeBase(UArenaAttributeSet::GetMaxAmmoAttribute(), WeaponInventory[Index].MaxAmmo);
			AbilitySystemComponent->SetNumericAttributeBase(UArenaAttributeSet::GetAmmoAttribute(), WeaponInventory[Index].CurrentAmmo);
		}

	}
	else if (EquippedWeaponMesh)
	{
		EquippedWeaponMesh->SetSkeletalMesh(nullptr);
	}
}

void AArenaShooterCharacter::DropCurrentWeapon()
{
	if (!WeaponInventory.IsValidIndex(CurrentWeaponIndex)) return;

	if (HasAuthority())
	{
		if (AttributeSet)
		{
			WeaponInventory[CurrentWeaponIndex].CurrentAmmo = AttributeSet->GetAmmo();
		}

		FWeaponStats& WepToDrop = WeaponInventory[CurrentWeaponIndex];

		if (WepToDrop.PickupBlueprintClass && GetWorld())
		{
			FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 120.0f;

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* DroppedWeapon = GetWorld()->SpawnActor<AActor>(WepToDrop.PickupBlueprintClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);

			if (DroppedWeapon)
			{
				if (UArenaShooterPickUpComponent* PickUpComp = DroppedWeapon->FindComponentByClass<UArenaShooterPickUpComponent>())
				{
					PickUpComp->WeaponStatsToGive = WepToDrop;
				}
			}
		}

		WeaponInventory.RemoveAt(CurrentWeaponIndex);

		if (WeaponInventory.Num() > 0)
		{
			CurrentWeaponIndex = 0;
			EquipWeapon(CurrentWeaponIndex);
		}
		else
		{
			EquipWeapon(-1);
		}
	}
	else
	{
		Server_DropCurrentWeapon();
	}
}

bool AArenaShooterCharacter::Server_DropCurrentWeapon_Validate()
{
	return true;
}

void AArenaShooterCharacter::Server_DropCurrentWeapon_Implementation()
{
	DropCurrentWeapon();
}

void AArenaShooterCharacter::AddWeapon(FWeaponStats NewWeapon)
{
	if (!HasAuthority()) return;

	if (WeaponInventory.IsValidIndex(CurrentWeaponIndex) && AttributeSet)
	{
		WeaponInventory[CurrentWeaponIndex].CurrentAmmo = AttributeSet->GetAmmo();
	}

	if (WeaponInventory.Num() >= 2)
	{
		DropCurrentWeapon();
	}

	WeaponInventory.Add(NewWeapon);
	CurrentWeaponIndex = WeaponInventory.Num() - 1;

	EquipWeapon(CurrentWeaponIndex);
	StopFire();
}

void AArenaShooterCharacter::SwitchNextWeapon()
{
	if (WeaponInventory.Num() <= 1) return;

	if (HasAuthority())
	{
		if (WeaponInventory.IsValidIndex(CurrentWeaponIndex) && AttributeSet)
		{
			WeaponInventory[CurrentWeaponIndex].CurrentAmmo = AttributeSet->GetAmmo();
		}

		CurrentWeaponIndex++;
		if (CurrentWeaponIndex >= WeaponInventory.Num())
		{
			CurrentWeaponIndex = 0;
		}

		EquipWeapon(CurrentWeaponIndex);
		StopFire();

		if (bSmartFireMode)
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_SmartFire, this, &AArenaShooterCharacter::SmartFireTick, WeaponInventory[CurrentWeaponIndex].FireRate, true);
		}
	}
	else
	{
		Server_SwitchNextWeapon();
	}
}

bool AArenaShooterCharacter::Server_SwitchNextWeapon_Validate()
{
	return true;
}

void AArenaShooterCharacter::Server_SwitchNextWeapon_Implementation()
{
	SwitchNextWeapon();
}

void AArenaShooterCharacter::StartFire()
{
	if (bIsReloading || !WeaponInventory.IsValidIndex(CurrentWeaponIndex)) return;

	if (AttributeSet && AttributeSet->GetAmmo() <= 0.0f)
	{
		ReloadWeapon();
		return;
	}

	ExecuteFire();
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_AutoFire, this, &AArenaShooterCharacter::ExecuteFire, WeaponInventory[CurrentWeaponIndex].FireRate, true);
}

void AArenaShooterCharacter::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);
}

void AArenaShooterCharacter::ExecuteFire()
{
	if (bIsReloading || !WeaponInventory.IsValidIndex(CurrentWeaponIndex)) return;

	if (AttributeSet && AttributeSet->GetAmmo() <= 0.0f)
	{
		StopFire();
		ReloadWeapon();
		return;
	}

	if (Controller != nullptr)
	{
		AddControllerPitchInput(-WeaponInventory[CurrentWeaponIndex].RecoilPitch);
		AddControllerYawInput(FMath::RandRange(-WeaponInventory[CurrentWeaponIndex].RecoilYaw, WeaponInventory[CurrentWeaponIndex].RecoilYaw));
	}

	FPredictionKey PredictionKey;

	if (IsLocallyControlled())
	{
		PlayFireFX();

		if (AbilitySystemComponent)
		{
			if (!HasAuthority())
			{
				FScopedPredictionWindow PredictionWindow(AbilitySystemComponent, true);
				PredictionKey = AbilitySystemComponent->ScopedPredictionKey;

				if (FireCostGEClass)
				{
					FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
					AbilitySystemComponent->ApplyGameplayEffectToSelf(FireCostGEClass->GetDefaultObject<UGameplayEffect>(), 1.0f, Context);
				}
			}
		}
	}

	Server_Fire(PredictionKey);
}

void AArenaShooterCharacter::PlayFireFX()
{
	FWeaponStats& CurrentWeapon = WeaponInventory[CurrentWeaponIndex];
	UWorld* const World = GetWorld();

	if (World != nullptr && CurrentWeapon.ProjectileClass != nullptr)
	{
		FVector SpawnLocation = FirstPersonCameraComponent->GetComponentLocation() + FirstPersonCameraComponent->GetForwardVector() * 50.0f;
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActorSpawnParams.Instigator = this;

		for (int32 i = 0; i < CurrentWeapon.BulletsPerShot; i++)
		{
			FRotator SpawnRotation = FirstPersonCameraComponent->GetComponentRotation();
			if (CurrentWeapon.SpreadAngle > 0.0f)
			{
				SpawnRotation.Pitch += FMath::RandRange(-CurrentWeapon.SpreadAngle, CurrentWeapon.SpreadAngle);
				SpawnRotation.Yaw += FMath::RandRange(-CurrentWeapon.SpreadAngle, CurrentWeapon.SpreadAngle);
			}
			World->SpawnActor<AArenaShooterProjectile>(CurrentWeapon.ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}
}

void AArenaShooterCharacter::Multicast_FireFX_Implementation()
{
	if (!IsLocallyControlled())
	{
		PlayFireFX();
	}
}

bool AArenaShooterCharacter::Server_Fire_Validate(FPredictionKey PredictionKey)
{
	return true;
}

void AArenaShooterCharacter::Server_Fire_Implementation(FPredictionKey PredictionKey)
{
	if (bIsReloading || !HasAuthority() || !WeaponInventory.IsValidIndex(CurrentWeaponIndex)) return;
	if (AttributeSet && AttributeSet->GetAmmo() <= 0.0f) return;

	FWeaponStats& CurrentWeapon = WeaponInventory[CurrentWeaponIndex];
	UWorld* const World = GetWorld();

	if (World != nullptr)
	{
		if (AbilitySystemComponent && FireCostGEClass)
		{
			if (PredictionKey.IsValidKey())
			{
				FScopedPredictionWindow PredictionWindow(AbilitySystemComponent, PredictionKey);
				FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
				AbilitySystemComponent->ApplyGameplayEffectToSelf(FireCostGEClass->GetDefaultObject<UGameplayEffect>(), 1.0f, Context);
			}
			else
			{
				FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
				AbilitySystemComponent->ApplyGameplayEffectToSelf(FireCostGEClass->GetDefaultObject<UGameplayEffect>(), 1.0f, Context);
			}
		}

		FVector StartLocation = FirstPersonCameraComponent->GetComponentLocation();
		FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();

		for (int32 i = 0; i < CurrentWeapon.BulletsPerShot; i++)
		{
			FVector ShootDir = ForwardVector;
			if (CurrentWeapon.SpreadAngle > 0.0f)
			{
				ShootDir = FMath::VRandCone(ForwardVector, FMath::DegreesToRadians(CurrentWeapon.SpreadAngle), FMath::DegreesToRadians(CurrentWeapon.SpreadAngle));
			}

			FVector EndLocation = StartLocation + (ShootDir * 10000.0f);
			FHitResult HitResult;
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			if (World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params))
			{
				AActor* HitActor = HitResult.GetActor();
				if (HitActor && CurrentWeapon.ProjectileClass)
				{
					AArenaShooterProjectile* ProjCDO = CurrentWeapon.ProjectileClass->GetDefaultObject<AArenaShooterProjectile>();
					if (ProjCDO)
					{
						UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
						if (TargetASC && ProjCDO->DamageEffectClass)
						{
							FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
							ContextHandle.AddInstigator(GetInstigator(), this);
							FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(ProjCDO->DamageEffectClass, 1.0f, ContextHandle);
							TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
						else
						{
							UGameplayStatics::ApplyDamage(HitActor, 25.0f, GetController(), this, UDamageType::StaticClass());
						}
					}
				}
			}
		}

		MakeNoise(1.0f, this, StartLocation);
		Multicast_FireFX();
	}
}

void AArenaShooterCharacter::ReloadWeapon()
{
	if (bIsReloading || !WeaponInventory.IsValidIndex(CurrentWeaponIndex)) return;

	bIsReloading = true;
	StopFire();

	float RTime = WeaponInventory[CurrentWeaponIndex].ReloadTime;
	OnReloadStarted.Broadcast(RTime);

	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_Reload, this, &AArenaShooterCharacter::FinishReload, RTime, false);
	}
	else
	{
		Server_ReloadWeapon();
	}
}

bool AArenaShooterCharacter::Server_ReloadWeapon_Validate()
{
	return true;
}

void AArenaShooterCharacter::Server_ReloadWeapon_Implementation()
{
	if (!HasAuthority() || !WeaponInventory.IsValidIndex(CurrentWeaponIndex)) return;

	bIsReloading = true;

	float RTime = WeaponInventory[CurrentWeaponIndex].ReloadTime;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_Reload, this, &AArenaShooterCharacter::FinishReload, RTime, false);
}

void AArenaShooterCharacter::FinishReload()
{
	bIsReloading = false;

	if (WeaponInventory.IsValidIndex(CurrentWeaponIndex) && AbilitySystemComponent && HasAuthority())
	{
		float MaxA = WeaponInventory[CurrentWeaponIndex].MaxAmmo;
		WeaponInventory[CurrentWeaponIndex].CurrentAmmo = MaxA;
		AbilitySystemComponent->SetNumericAttributeBase(UArenaAttributeSet::GetAmmoAttribute(), MaxA);
	}
}

void AArenaShooterCharacter::ToggleSmartFire()
{
	bSmartFireMode = !bSmartFireMode;
	if (bSmartFireMode)
	{
		float CurrentFireRate = WeaponInventory.IsValidIndex(CurrentWeaponIndex) ? WeaponInventory[CurrentWeaponIndex].FireRate : 0.1f;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_SmartFire, this, &AArenaShooterCharacter::SmartFireTick, CurrentFireRate, true);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_SmartFire);
	}
}

void AArenaShooterCharacter::SmartFireTick()
{
	if (!bSmartFireMode || bIsReloading || !WeaponInventory.IsValidIndex(CurrentWeaponIndex)) return;

	if (AttributeSet && AttributeSet->GetAmmo() <= 0.0f)
	{
		ReloadWeapon();
		return;
	}

	FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	FVector End = Start + (FirstPersonCameraComponent->GetForwardVector() * 8000.0f);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (HitActor->IsA(AArenaEnemyCharacter::StaticClass()))
			{
				ExecuteFire();
			}
		}
	}
}

void AArenaShooterCharacter::GiveAbilities()
{
	if (HasAuthority() && AbilitySystemComponent)
	{
		if (DashAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DashAbilityClass, 1, 0, this));
		if (ReloadAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ReloadAbilityClass, 1, 0, this));
	}
}

void AArenaShooterCharacter::OnDashStarted()
{
	if (AbilitySystemComponent && DashAbilityClass) AbilitySystemComponent->TryActivateAbilityByClass(DashAbilityClass);
}

float AArenaShooterCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (AbilitySystemComponent && ActualDamage > 0.0f)
	{
		UGameplayEffect* DamageEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("TempDamageBridge")));
		DamageEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
		int32 Idx = DamageEffect->Modifiers.Num();
		DamageEffect->Modifiers.SetNum(Idx + 1);
		FGameplayModifierInfo& Info = DamageEffect->Modifiers[Idx];
		Info.ModifierMagnitude = FScalableFloat(-ActualDamage);
		Info.ModifierOp = EGameplayModOp::Additive;
		Info.Attribute = UArenaAttributeSet::GetHealthAttribute();

		AbilitySystemComponent->ApplyGameplayEffectToSelf(DamageEffect, 1.0f, AbilitySystemComponent->MakeEffectContext());
	}
	return ActualDamage;
}