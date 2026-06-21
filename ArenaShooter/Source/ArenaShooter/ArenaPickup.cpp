#include "ArenaPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

AArenaPickup::AArenaPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(50.0f);
	CollisionComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 视觉模型不需要碰撞

	CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AArenaPickup::OnOverlapBegin);
}

void AArenaPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 检查碰到道具的 Actor 是否接入了 GAS 系统（玩家和敌人都接入了）
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OtherActor))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			// 如果配置了效果，直接应用给拾取者
			if (PickupEffect && HasAuthority())
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				Context.AddInstigator(this, this);
				ASC->ApplyGameplayEffectToSelf(PickupEffect->GetDefaultObject<UGameplayEffect>(), 1.0f, Context);
			}

			// 播放表现效果
			if (PickupSound) UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
			if (PickupVFX) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), PickupVFX, GetActorLocation());

			// 销毁道具
			Destroy();
		}
	}
}