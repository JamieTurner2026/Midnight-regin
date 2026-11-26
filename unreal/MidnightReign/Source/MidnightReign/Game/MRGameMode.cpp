#include "Game/MRGameMode.h"

#include "EngineUtils.h"
#include "Game/MRCheckpoint.h"
#include "GameFramework/PlayerController.h"
#include "MidnightReign.h"
#include "UI/MRDrivingHUD.h"
#include "Vehicles/MRVehiclePawn.h"

AMRGameMode::AMRGameMode()
{
	DefaultPawnClass = AMRVehiclePawn::StaticClass();
	HUDClass = AMRDrivingHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;

	// No gates until StartPlay finds them in the level, so the HUD shows free roam until then.
	mr::CheckpointRunConfig Empty;
	Empty.NumCheckpoints = 0;
	Empty.TimeLimit = TimeLimit;
	Run.SetConfig(Empty);
}

EMRRunState AMRGameMode::ToRunState(mr::RunState State)
{
	switch (State)
	{
	case mr::RunState::Running: return EMRRunState::Running;
	case mr::RunState::Won: return EMRRunState::Won;
	case mr::RunState::Lost: return EMRRunState::Lost;
	default: return EMRRunState::Ready;
	}
}

void AMRGameMode::StartPlay()
{
	Super::StartPlay();
	GatherCheckpoints();
	if (bStartRunOnPlay)
	{
		HandleEvents(Run.Start());
	}
}

void AMRGameMode::GatherCheckpoints()
{
	Checkpoints.Reset();
	for (TActorIterator<AMRCheckpoint> It(GetWorld()); It; ++It)
	{
		Checkpoints.Add(*It);
	}
	Checkpoints.Sort([](const AMRCheckpoint& A, const AMRCheckpoint& B) { return A.Order < B.Order; });

	// Orders must be 0..N-1 with no gaps or repeats, or the run could never be finished.
	for (int32 i = 0; i < Checkpoints.Num(); ++i)
	{
		if (Checkpoints[i]->Order != i)
		{
			UE_LOG(LogMidnightReign, Error,
				TEXT("Checkpoint %s has Order %d; expected %d. Give the gates Orders 0..%d with no gaps."),
				*Checkpoints[i]->GetName(), Checkpoints[i]->Order, i, Checkpoints.Num() - 1);
		}
	}
	if (Checkpoints.Num() == 0)
	{
		UE_LOG(LogMidnightReign, Warning, TEXT("No MRCheckpoint in this level: free roam only."));
	}

	mr::CheckpointRunConfig Config;
	Config.NumCheckpoints = Checkpoints.Num();
	Config.TimeLimit = TimeLimit;
	Run.SetConfig(Config);
}

void AMRGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	HandleEvents(Run.Tick(DeltaSeconds));
}

void AMRGameMode::ReportCheckpointReached(AMRCheckpoint* Checkpoint)
{
	if (Checkpoint)
	{
		HandleEvents(Run.PassCheckpoint(Checkpoint->Order));
	}
}

void AMRGameMode::RestartRun()
{
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		AActor* Start = FindPlayerStart(PC);
		AMRVehiclePawn* Car = Cast<AMRVehiclePawn>(PC->GetPawn());
		if (Start && Car)
		{
			Car->ResetVehicleAt(Start->GetActorTransform());
		}
	}
	if (Checkpoints.Num() == 0)
	{
		GatherCheckpoints();
	}
	HandleEvents(Run.Start());
}

void AMRGameMode::HandleEvents(const mr::CheckpointRunEvents& Events)
{
	if (Events.bStarted)
	{
		OnRunStarted.Broadcast();
	}
	if (Events.bPassed && Checkpoints.IsValidIndex(Events.PassedIndex))
	{
		Checkpoints[Events.PassedIndex]->NotifyPassed(Events.bWon);
		OnCheckpointPassed.Broadcast(Events.PassedIndex, Run.GetSplit(Events.PassedIndex));
	}
	if (Events.bWon)
	{
		OnRunWon.Broadcast();
	}
	if (Events.bLost)
	{
		OnRunLost.Broadcast();
	}
	if (Events.bStarted || Events.bPassed || Events.bWon || Events.bLost)
	{
		UpdateNextHighlight();
	}
}

void AMRGameMode::UpdateNextHighlight()
{
	const int32 Next = Run.IsRunning() ? Run.GetNextCheckpoint() : INDEX_NONE;
	for (int32 i = 0; i < Checkpoints.Num(); ++i)
	{
		if (Checkpoints[i])
		{
			Checkpoints[i]->NotifyIsNext(i == Next);
		}
	}
}
