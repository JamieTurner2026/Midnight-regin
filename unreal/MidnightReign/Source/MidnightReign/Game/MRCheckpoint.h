#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MRCheckpoint.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

/**
 * One gate of the timed checkpoint run. Drive the player car through the trigger to pass it.
 * The game mode decides whether the pass counts (gates must be taken in Order); this actor only reports
 * the overlap and plays its effect. Make a Blueprint subclass (BP_MRCheckpoint) to set the arch mesh,
 * the Niagara system for PassEffect and any sound in OnPassed.
 */
UCLASS()
class MIDNIGHTREIGN_API AMRCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	AMRCheckpoint();

	/** Position in the run: 0 is the first gate to drive through, the highest is the finish. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MidnightReign|Checkpoint")
	int32 Order = 0;

	/** Called by the game mode when this gate is accepted. Plays PassEffect, then OnPassed. */
	void NotifyPassed(bool bIsFinish);

	/** Called by the game mode when this gate becomes, or stops being, the next one to drive through. */
	void NotifyIsNext(bool bIsNext);

	UFUNCTION(BlueprintPure, Category = "MidnightReign|Checkpoint")
	bool IsNext() const { return bNext; }

protected:
	virtual void BeginPlay() override;

	/** Blueprint hook: flash, sound, light change. bIsFinish is true on the last gate. */
	UFUNCTION(BlueprintImplementableEvent, Category = "MidnightReign|Checkpoint")
	void OnPassed(bool bIsFinish);

	/** Blueprint hook: highlight the gate the player should aim for (e.g. brighter neon). */
	UFUNCTION(BlueprintImplementableEvent, Category = "MidnightReign|Checkpoint")
	void OnNextChanged(bool bIsNext);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidnightReign|Checkpoint")
	TObjectPtr<UBoxComponent> Trigger;

	/** The visible arch (SM_Checkpoint_Arch). No collision: the car drives through it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidnightReign|Checkpoint")
	TObjectPtr<UStaticMeshComponent> Arch;

	/** One-shot burst when the gate is passed. Set its Niagara System in the Blueprint (NS_MR_CheckpointFlash). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MidnightReign|Checkpoint")
	TObjectPtr<UNiagaraComponent> PassEffect;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bNext = false;
};
