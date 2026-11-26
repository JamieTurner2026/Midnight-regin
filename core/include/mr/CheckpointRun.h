// Timed checkpoint run: pass the gates in order before the clock runs out.
// Engine-independent so the rules are unit tested; AMRGameMode wraps it in Unreal.
#pragma once

#include <vector>

#include "mr/Math.h"

namespace mr
{
struct CheckpointRunConfig
{
	int NumCheckpoints = 3;   // gates in the order they must be passed; the last one is the finish
	float TimeLimit = 60.f;   // seconds from Start to the finish
};

enum class RunState
{
	Ready,    // waiting for Start
	Running,  // clock counting down
	Won,      // every gate passed in time
	Lost      // clock reached zero first
};

/** What changed during one call (for HUD flashes, sounds and effects). */
struct CheckpointRunEvents
{
	bool bStarted = false;
	bool bPassed = false;     // a gate was accepted
	int PassedIndex = -1;     // which gate, when bPassed
	bool bWon = false;
	bool bLost = false;
};

class CheckpointRun
{
public:
	CheckpointRun() { Reset(); }
	explicit CheckpointRun(const CheckpointRunConfig& InConfig) : Config(InConfig) { Reset(); }

	void SetConfig(const CheckpointRunConfig& InConfig)
	{
		Config = InConfig;
		Reset();
	}

	/** Back to Ready with a full clock and no splits. */
	void Reset()
	{
		State = RunState::Ready;
		NextIndex = 0;
		Elapsed = 0.f;
		Splits.assign(static_cast<size_t>(Config.NumCheckpoints > 0 ? Config.NumCheckpoints : 0), -1.f);
	}

	/** Start (or restart) the clock. A run with no gates or no time cannot start. */
	CheckpointRunEvents Start()
	{
		CheckpointRunEvents Ev;
		Reset();
		if (Config.NumCheckpoints <= 0 || Config.TimeLimit <= 0.f)
		{
			return Ev;
		}
		State = RunState::Running;
		Ev.bStarted = true;
		return Ev;
	}

	/** Advance the clock by DeltaTime seconds. Only a running clock moves. */
	CheckpointRunEvents Tick(float DeltaTime)
	{
		CheckpointRunEvents Ev;
		if (State != RunState::Running || DeltaTime <= 0.f)
		{
			return Ev;
		}
		Elapsed += DeltaTime;
		if (Elapsed >= Config.TimeLimit)
		{
			Elapsed = Config.TimeLimit;
			State = RunState::Lost;
			Ev.bLost = true;
		}
		return Ev;
	}

	/**
	 * The player drove through gate Index. Accepted only while running and only for the next gate in order;
	 * any other gate (already passed, skipped ahead, unknown) is ignored.
	 */
	CheckpointRunEvents PassCheckpoint(int Index)
	{
		CheckpointRunEvents Ev;
		if (State != RunState::Running || Index != NextIndex)
		{
			return Ev;
		}
		Splits[static_cast<size_t>(Index)] = Elapsed;
		Ev.bPassed = true;
		Ev.PassedIndex = Index;
		++NextIndex;
		if (NextIndex >= Config.NumCheckpoints)
		{
			State = RunState::Won;
			Ev.bWon = true;
		}
		return Ev;
	}

	RunState GetState() const { return State; }
	bool IsRunning() const { return State == RunState::Running; }
	int GetNumCheckpoints() const { return Config.NumCheckpoints; }
	/** The gate to drive through next; equals GetNumCheckpoints() once every gate is passed. */
	int GetNextCheckpoint() const { return NextIndex; }
	float GetElapsed() const { return Elapsed; }
	float GetTimeLimit() const { return Config.TimeLimit; }
	float GetTimeLeft() const { return Max(0.f, Config.TimeLimit - Elapsed); }
	/** Seconds from Start when gate Index was passed, or -1 if not passed (or out of range). */
	float GetSplit(int Index) const
	{
		return (Index >= 0 && Index < static_cast<int>(Splits.size())) ? Splits[static_cast<size_t>(Index)] : -1.f;
	}

private:
	CheckpointRunConfig Config;
	RunState State = RunState::Ready;
	int NextIndex = 0;
	float Elapsed = 0.f;
	std::vector<float> Splits;
};
} // namespace mr
