#include "ArenaEnemyCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ArenaShooterGameMode.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "DrawDebugHelpers.h"
#include "Components/WidgetComponent.h"
// === 新增头文件 ===
#include "AIC_ArenaEnemy.h"
#include "BehaviorTree/BlackboardComponent.h"

AArenaEnemyCharacter::AArenaEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RewardScore = 20;
	MaxHealth = 100.0f;
	Health = MaxHealth;
	bIsDead = false;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	HealthBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawSize(FVector2D(150.0f, 15.0f));
}

void AArenaEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHealthBarUI(Health, MaxHealth);
}

void AArenaEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AArenaEnemyCharacter, Health);
}

void AArenaEnemyCharacter::PerformAttack(AActor* TargetPlayer)
{
	if (bIsDead || !TargetPlayer) return;

	// 🚨 双重保险：开火前发现目标已死亡，立刻停止开火并清空仇恨
	if (TargetPlayer->ActorHasTag(FName("Dead")))
	{
		if (AAIC_ArenaEnemy* AIController = Cast<AAIC_ArenaEnemy>(GetController()))
		{
			if (UBlackboardComponent* BBComp = AIController->GetBlackboardComponent())
			{
				BBComp->ClearValue(TEXT("TargetActor"));
			}
		}
		return;
	}

	UWorld* const World = GetWorld();
	if (!World) return;

	FVector MuzzleLocation = GetActorLocation();
	FRotator MuzzleRotation = GetActorRotation();

	if (GetMesh()->DoesSocketExist("Muzzle_01"))
	{
		MuzzleLocation = GetMesh()->GetSocketLocation("Muzzle_01");
		MuzzleRotation = GetMesh()->GetSocketRotation("Muzzle_01");
	}
	else if (GetMesh()->DoesSocketExist("weapon_r"))
	{
		MuzzleLocation = GetMesh()->GetSocketLocation("weapon_r");
		MuzzleRotation = GetMesh()->GetSocketRotation("weapon_r");
	}

	FVector StartLoc = MuzzleLocation;
	FVector EndLoc = TargetPlayer->GetActorLocation() + FVector(0, 0, 40.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = World->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, QueryParams);
	FVector TracerEndPoint = EndLoc;

	if (bHit)
	{
		TracerEndPoint = HitResult.ImpactPoint;
		UGameplayStatics::ApplyDamage(HitResult.GetActor(), 20.0f, GetController(), this, UDamageType::StaticClass());
	}

	Multicast_PlayAttackFX(StartLoc, TracerEndPoint);
}

void AArenaEnemyCharacter::Multicast_PlayAttackFX_Implementation(FVector StartLoc, FVector TracerEndPoint)
{
	UWorld* const World = GetWorld();
	if (!World) return;

	if (ImpactFX && StartLoc != TracerEndPoint)
	{
		FRotator ImpactRotation = (StartLoc - TracerEndPoint).Rotation();
		UGameplayStatics::SpawnEmitterAtLocation(World, ImpactFX, TracerEndPoint, ImpactRotation);
	}

	if (MuzzleFlashFX)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlashFX, GetMesh(), TEXT("Muzzle_01"));
	}

	if (TracerFX)
	{
		FRotator TrueTracerRotation = (TracerEndPoint - StartLoc).Rotation();
		UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(World, TracerFX, StartLoc, TrueTracerRotation);

		if (TracerComp)
		{
			TracerComp->SetVectorParameter(FName("ShockBeamEnd"), TracerEndPoint);
			TracerComp->SetVectorParameter(FName("Target"), TracerEndPoint);
		}
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, FireSound, StartLoc);
	}
}

float AArenaEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || !HasAuthority()) return 0.0f;

	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	Health -= ActualDamage;

	UpdateHealthBarUI(Health, MaxHealth);

	if (Health <= 0.0f)
	{
		Health = 0.0f;
		if (HealthBarWidget)
		{
			HealthBarWidget->SetVisibility(false);
		}
		Die(EventInstigator);
	}
	else if (ActualDamage > 0.0f && HurtSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HurtSound, GetActorLocation());
	}

	return ActualDamage;
}

void AArenaEnemyCharacter::OnRep_Health()
{
	UpdateHealthBarUI(Health, MaxHealth);
}

void AArenaEnemyCharacter::Die(AController* Killer)
{
	bIsDead = true;

	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
		AIController->UnPossess();
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();

	if (HasAuthority())
	{
		if (AArenaShooterGameMode* GM = Cast<AArenaShooterGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->AddScore(RewardScore);
		}
	}

	SetLifeSpan(3.0f);
}