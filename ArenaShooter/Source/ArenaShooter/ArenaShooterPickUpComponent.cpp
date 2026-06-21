#include "ArenaShooterPickUpComponent.h"
#include "TimerManager.h"

UArenaShooterPickUpComponent::UArenaShooterPickUpComponent()
{
	SphereRadius = 60.f;
}

void UArenaShooterPickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWorld()->GetTimerManager().SetTimer(EnableCollisionTimer, this, &UArenaShooterPickUpComponent::EnablePickup, 0.5f, false);
}

void UArenaShooterPickUpComponent::EnablePickup()
{
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OnComponentBeginOverlap.AddDynamic(this, &UArenaShooterPickUpComponent::OnSphereBeginOverlap);
}

void UArenaShooterPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	AArenaShooterCharacter* Character = Cast<AArenaShooterCharacter>(OtherActor);
	if (Character != nullptr)
	{
		Character->AddWeapon(WeaponStatsToGive);
		OnPickUp.Broadcast(Character);
		OnComponentBeginOverlap.RemoveAll(this);

		if (GetOwner())
		{
			GetOwner()->Destroy();
		}
	}
}