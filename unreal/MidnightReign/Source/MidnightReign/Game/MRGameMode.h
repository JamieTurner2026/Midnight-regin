#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "mr/CheckpointRun.h"
#include "MRGameMode.generated.h"

class AMRCheckpoint;

/** Blueprint view of the run state (mirrors mr::RunState). */
UENUM(BlueprintType)
enum class EMRRunState : uint8
{
	Ready,
	Running,
	Won,
	Lost
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMRRunEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMRCheckpointEvent, int32, Order, float, SplitSeconds);

/**
 * Driving game mode: spawns the player car and the driving HUD, and runs the timed checkpoint run
 * (mr::CheckpointRun in core/): drive through every AMRCheckpoint in Order before TimeLimit runs out.
 * The run starts when play begins; RestartRun puts the car back on the Player Start and starts again.
 * Make a Blueprint subclass to swap in your vehicle Blueprint and to react to the run events.
 */
UCLASS()
class MIDNIGHTREIGN_API AMRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMRGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Seconds to pass every gate. The Solaris test block lap is 1.18 km. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MidnightReign|Run", meta = (ClampMin = "1"))
	float TimeLimit = 60.f;

	/** Start the run as soon as play begins. Off: call RestartRun yourself (e.g. after a countdown). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MidnightReign|Run")
	bool bStartRunOnPlay = true;

	/** Put the player car on the Player Start, stopped, and start a fresh run. */
	UFUNCTION(BlueprintCallable, Category = "MidnightReign|Run")
	void RestartRun();

	/** Called by AMRCheckpoint when the player car enters its trigger. Out-of-order gates are ignored. */
	void ReportCheckpointReached(AMRCheckpoint* Checkpoint);

	// --- State for HUD / UI (read-only) ---

	UFUNCTION(BlueprintPure, Category = "MidnightReign|Run")
	EMRRunState GetRunState() const { return ToRunState(Run.GetState()); }

	UFUNCTION(BlueprintPure, Category = "MidnightReign|Run")
	float GetTimeLeft() const { return Run.GetTimeLeft(); }

	UFUNCTION(BlueprintPure, Category = "MidnightReign|Run")
	float GetElapsed() const { return Run.GetElapsed(); }

	/** 0-based order of the gate to drive through next; equals GetNumCheckpoints() once finished. */
	UFUNCTION(BlueprintPure, Category = "MidnightReign|Run")
	int32 GetNextCheckpointOrder() const { return Run.GetNextCheckpoint(); }

	UFUNCTION(BlueprintPure, Category = "MidnightReign|Run")
	int32 GetNumCheckpoints() const { return Run.GetNumCheckpoints(); }

	/** Seconds from the start when the gate was passed, or -1. */
	UFUNCTION(BlueprintPure, Category = "MidnightReign|Run")
	float GetSplit(int32 Order) const { return Run.GetSplit(Order); }

	UPROPERTY(BlueprintAssignable, Category = "MidnightReign|Run")
	FMRRunEvent OnRunStarted;

	UPROPERTY(BlueprintAssignable, Category = "MidnightReign|Run")
	FMRCheckpointEvent OnCheckpointPassed;

	UPROPERTY(BlueprintAssignable, Category = "MidnightReign|Run")
	FMRRunEvent OnRunWon;

	UPROPERTY(BlueprintAssignable, Category = "MidnightReign|Run")
	FMRRunEvent OnRunLost;

	static EMRRunState ToRunState(mr::RunState State);

private:
	void GatherCheckpoints();
	void HandleEvents(const mr::CheckpointRunEvents& Events);
	void UpdateNextHighlight();

	mr::CheckpointRun Run;

	/** Gates sorted by Order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AMRCheckpoint>> Checkpoints;
};
