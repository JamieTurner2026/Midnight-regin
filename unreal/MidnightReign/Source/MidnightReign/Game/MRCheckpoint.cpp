#include "Game/MRCheckpoint.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Game/MRGameMode.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"

AMRCheckpoint::AMRCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// Road is 14 m wide; the trigger spans it with margin, 2 m deep, 6 m tall, bottom on the road.
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->InitBoxExtent(FVector(100.f, 900.f, 300.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 300.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);
	RootComponent = Trigger;

	Arch = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arch"));
	Arch->SetupAttachment(Trigger);
	Arch->SetRelativeLocation(FVector(0.f, 0.f, -300.f));
	Arch->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PassEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PassEffect"));
	PassEffect->SetupAttachment(Trigger);
	PassEffect->SetAutoActivate(false);
}

void AMRCheckpoint::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMRCheckpoint::HandleOverlap);
}

void AMRCheckpoint::HandleOverlap(UPrimitiveComponent* /*OverlappedComponent*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}
	if (AMRGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMRGameMode>() : nullptr)
	{
		GameMode->ReportCheckpointReached(this);
	}
}

void AMRCheckpoint::NotifyPassed(bool bIsFinish)
{
	if (PassEffect && PassEffect->GetAsset())
	{
		PassEffect->Activate(true);
	}
	OnPassed(bIsFinish);
}

void AMRCheckpoint::NotifyIsNext(bool bIsNext)
{
	bNext = bIsNext;
	OnNextChanged(bIsNext);
}
