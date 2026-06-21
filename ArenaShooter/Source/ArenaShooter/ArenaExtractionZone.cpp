#include "ArenaExtractionZone.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "ArenaShooterCharacter.h"
#include "ArenaShooterGameMode.h"

AArenaExtractionZone::AArenaExtractionZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	OverlapComp = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapComp"));
	OverlapComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OverlapComp->SetBoxExtent(FVector(200.0f));
	RootComponent = OverlapComp;

	WaypointWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WaypointWidgetComp"));
	WaypointWidgetComp->SetupAttachment(RootComponent);
	WaypointWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WaypointWidgetComp->SetDrawAtDesiredSize(true);
}

void AArenaExtractionZone::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OverlapComp->OnComponentBeginOverlap.AddDynamic(this, &AArenaExtractionZone::HandleOverlap);
	}
}

void AArenaExtractionZone::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AArenaShooterCharacter* PlayerChar = Cast<AArenaShooterCharacter>(OtherActor))
	{
		if (AArenaShooterGameMode* GM = Cast<AArenaShooterGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GM->TriggerExtractionVictory();
		}
	}
}