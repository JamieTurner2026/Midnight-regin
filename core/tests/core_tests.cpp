// Dependency-free tests for the engine-independent core.
#include <cmath>
#include <cstdio>

#include "mr/CheckpointRun.h"
#include "mr/DrivingModel.h"
#include "mr/Nitrous.h"
#include "mr/Steering.h"

static int GFailures = 0;
static int GChecks = 0;

#define CHECK(Cond) \
	do { ++GChecks; if (!(Cond)) { ++GFailures; std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #Cond); } } while (0)
#define CHECK_NEAR(A, B, Tol) \
	do { ++GChecks; const double A_ = (A), B_ = (B); if (std::fabs(A_ - B_) > (Tol)) { ++GFailures; \
		std::printf("FAIL %s:%d  %s = %g, expected %g\n", __FILE__, __LINE__, #A, A_, B_); } } while (0)

static void TestNitrousDrainAndEvents()
{
	mr::NitrousTank Tank;
	CHECK_NEAR(Tank.GetFraction(), 1.0, 1e-6);

	mr::NitrousEvents Ev = Tank.Update(true, true, 0.01f);
	CHECK(Ev.bBoosting && Ev.bActivated && !Ev.bDeactivated && !Ev.bDepleted);
	Ev = Tank.Update(true, true, 0.01f);
	CHECK(Ev.bBoosting && !Ev.bActivated);

	float T = 0.02f;
	bool bSawDepleted = false;
	for (int i = 0; i < 1000 && !bSawDepleted; ++i)
	{
		Ev = Tank.Update(true, true, 0.01f);
		T += 0.01f;
		bSawDepleted = Ev.bDepleted;
	}
	CHECK(bSawDepleted);
	CHECK(Ev.bDeactivated);
	CHECK_NEAR(T, 4.0, 0.05); // 100 / 25 per second
	CHECK_NEAR(Tank.GetAmount(), 0.0, 1e-6);
	CHECK(!Tank.IsActive());

	Ev = Tank.Update(true, true, 0.01f);
	CHECK(!Ev.bBoosting && !Ev.bActivated && !Ev.bDeactivated);
}

static void TestNitrousRecharge()
{
	mr::NitrousTank Tank;
	for (int i = 0; i < 500; ++i) Tank.Update(true, true, 0.01f); // drain fully

	// Holding the input never refills.
	for (int i = 0; i < 1000; ++i) Tank.Update(true, true, 0.01f);
	CHECK_NEAR(Tank.GetAmount(), 0.0, 1e-6);

	// Released: nothing during the delay, then a steady refill to capacity.
	for (int i = 0; i < 140; ++i) Tank.Update(false, true, 0.01f);
	CHECK_NEAR(Tank.GetAmount(), 0.0, 1e-6);
	for (int i = 0; i < 110; ++i) Tank.Update(false, true, 0.01f);
	CHECK_NEAR(Tank.GetAmount(), 8.0, 0.2); // 1 s at 8 per second
	for (int i = 0; i < 2000; ++i) Tank.Update(false, true, 0.01f);
	CHECK_NEAR(Tank.GetAmount(), 100.0, 1e-4);
}

static void TestNitrousGating()
{
	mr::NitrousTank Tank;
	// Not allowed (e.g. stationary / reversing): no boost, no drain.
	mr::NitrousEvents Ev = Tank.Update(true, false, 0.5f);
	CHECK(!Ev.bBoosting);
	CHECK_NEAR(Tank.GetAmount(), 100.0, 1e-6);

	Tank.SetAmount(5.f);
	CHECK(!Tank.Update(true, true, 0.01f).bBoosting); // below MinToActivate
	Tank.SetAmount(10.f);
	CHECK(Tank.Update(true, true, 0.01f).bBoosting);

	// Releasing reports a deactivation exactly once.
	Ev = Tank.Update(false, true, 0.01f);
	CHECK(Ev.bDeactivated && !Ev.bBoosting);
	Ev = Tank.Update(false, true, 0.01f);
	CHECK(!Ev.bDeactivated);
}

static void TestNitrousUpgradeKeepsFraction()
{
	mr::NitrousTank Tank;
	Tank.SetAmount(50.f);
	mr::NitrousConfig Bigger;
	Bigger.Capacity = 200.f;
	Tank.SetConfig(Bigger);
	CHECK_NEAR(Tank.GetAmount(), 100.0, 1e-4);
	CHECK_NEAR(Tank.GetFraction(), 0.5, 1e-6);
}

static void TestNitrousFrameRateIndependent()
{
	auto DrainTime = [](float Dt) {
		mr::NitrousTank Tank;
		float T = 0.f;
		while (Tank.Update(true, true, Dt).bBoosting) T += Dt;
		return T;
	};
	CHECK_NEAR(DrainTime(1.f / 30.f), DrainTime(1.f / 240.f), 0.05);
}

static void TestSteeringScale()
{
	mr::SteeringConfig C;
	CHECK_NEAR(mr::SteeringScaleForSpeed(0.f, C), 1.0, 1e-6);
	CHECK_NEAR(mr::SteeringScaleForSpeed(40.f, C), 1.0, 1e-6);
	CHECK_NEAR(mr::SteeringScaleForSpeed(220.f, C), 0.25, 1e-6);
	CHECK_NEAR(mr::SteeringScaleForSpeed(400.f, C), 0.25, 1e-6);
	CHECK_NEAR(mr::SteeringScaleForSpeed(-130.f, C), mr::SteeringScaleForSpeed(130.f, C), 1e-6);
	float Prev = 1.f;
	for (float Kmh = 0.f; Kmh <= 300.f; Kmh += 5.f) // never increases with speed
	{
		const float S = mr::SteeringScaleForSpeed(Kmh, C);
		CHECK(S <= Prev + 1e-6f);
		Prev = S;
	}
	mr::SteeringConfig Degenerate;
	Degenerate.MinLockAboveKmh = Degenerate.FullLockBelowKmh;
	CHECK_NEAR(mr::SteeringScaleForSpeed(500.f, Degenerate), 1.0, 1e-6);
}

static void TestSteeringSmoothing()
{
	mr::SteeringConfig C; // rise 4/s, return 6/s
	float V = 0.f;
	V = mr::SmoothSteeringInput(V, 1.f, C, 0.1f);
	CHECK_NEAR(V, 0.4, 1e-6);
	for (int i = 0; i < 10; ++i) V = mr::SmoothSteeringInput(V, 1.f, C, 0.1f);
	CHECK_NEAR(V, 1.0, 1e-6); // reaches and never overshoots
	V = mr::SmoothSteeringInput(V, 0.f, C, 0.1f);
	CHECK_NEAR(V, 0.4, 1e-6); // returns faster
	V = mr::SmoothSteeringInput(0.5f, -1.f, C, 0.05f);
	CHECK_NEAR(V, 0.2, 1e-6); // reversing direction uses the return rate
	CHECK_NEAR(mr::SmoothSteeringInput(0.f, 5.f, C, 10.f), 1.0, 1e-6); // target clamped
}

static mr::ArcadeDriveState Run(mr::ArcadeDriveInput In, float Seconds, float Dt, mr::ArcadeDriveState S = {})
{
	const mr::ArcadeDriveConfig C;
	const int Steps = static_cast<int>(std::lround(Seconds / Dt));
	for (int i = 0; i < Steps; ++i) mr::StepArcadeDrive(S, In, C, Dt);
	return S;
}

static void TestArcadeDrive()
{
	mr::ArcadeDriveInput Throttle; Throttle.bThrottle = true;
	mr::ArcadeDriveInput Brake;    Brake.bBrake = true;
	mr::ArcadeDriveInput Coast;

	// 0 -> max takes MaxSpeed / Acceleration = 0.667 s (40 frames in the prototype).
	CHECK_NEAR(Run(Throttle, 0.5f, 1.f / 60.f).Speed, 450.0, 0.5);
	mr::ArcadeDriveState Top = Run(Throttle, 5.f, 1.f / 60.f);
	CHECK_NEAR(Top.Speed, 600.0, 1e-3);
	CHECK_NEAR(Run(Brake, 5.f, 1.f / 60.f).Speed, -300.0, 1e-3);
	CHECK_NEAR(Run(Coast, 1.f, 1.f / 60.f, Top).Speed, 420.0, 0.5);
	CHECK_NEAR(Run(Coast, 10.f, 1.f / 60.f, Top).Speed, 0.0, 1e-6);

	// Boost raises the cap to 1.5x, then bleeds back without snapping.
	mr::ArcadeDriveInput Boost = Throttle; Boost.bBoosting = true;
	mr::ArcadeDriveState Boosted = Run(Boost, 5.f, 1.f / 60.f);
	CHECK_NEAR(Boosted.Speed, 900.0, 1e-3);
	const float After = Run(Throttle, 1.f / 60.f, 1.f / 60.f, Boosted).Speed;
	CHECK(After < 900.f && After > 890.f);
	CHECK_NEAR(Run(Throttle, 5.f, 1.f / 60.f, Boosted).Speed, 600.0, 1e-3);

	// Boost does nothing while stationary or reversing.
	mr::ArcadeDriveInput BoostOnly; BoostOnly.bBoosting = true;
	CHECK_NEAR(Run(BoostOnly, 1.f, 1.f / 60.f).Speed, 0.0, 1e-6);

	// Steering: none at rest, reversed when reversing.
	mr::ArcadeDriveInput SteerOnly; SteerOnly.Steer = 1.f;
	CHECK_NEAR(Run(SteerOnly, 1.f, 1.f / 60.f).AngleDeg, 0.0, 1e-6);
	mr::ArcadeDriveInput Fwd = Throttle; Fwd.Steer = 1.f;
	mr::ArcadeDriveInput Rev = Brake;    Rev.Steer = 1.f;
	CHECK(Run(Fwd, 1.f, 1.f / 60.f).AngleDeg > 0.f);
	CHECK(Run(Rev, 1.f, 1.f / 60.f).AngleDeg < 0.f);

	// Frame-rate independence of speed.
	CHECK_NEAR(Run(Boost, 0.4f, 1.f / 30.f).Speed, Run(Boost, 0.4f, 1.f / 240.f).Speed, 1.0);
}

static void TestCheckpointRunWinInOrder()
{
	mr::CheckpointRun Run(mr::CheckpointRunConfig{3, 60.f});
	CHECK(Run.GetState() == mr::RunState::Ready);
	CHECK(!Run.PassCheckpoint(0).bPassed);            // not started: ignored
	CHECK(Run.Start().bStarted);
	CHECK(Run.IsRunning());
	Run.Tick(10.f);
	mr::CheckpointRunEvents Ev = Run.PassCheckpoint(0);
	CHECK(Ev.bPassed && Ev.PassedIndex == 0 && !Ev.bWon);
	CHECK_NEAR(Run.GetSplit(0), 10.0, 1e-5);
	CHECK(Run.GetNextCheckpoint() == 1);
	Run.Tick(15.f);
	CHECK(Run.PassCheckpoint(1).bPassed);
	Run.Tick(20.f);
	Ev = Run.PassCheckpoint(2);
	CHECK(Ev.bPassed && Ev.bWon);
	CHECK(Run.GetState() == mr::RunState::Won);
	CHECK_NEAR(Run.GetElapsed(), 45.0, 1e-4);
	CHECK_NEAR(Run.GetTimeLeft(), 15.0, 1e-4);
	CHECK(Run.GetNextCheckpoint() == 3);
	// Finished: the clock stops and no more events.
	CHECK(!Run.Tick(30.f).bLost);
	CHECK_NEAR(Run.GetElapsed(), 45.0, 1e-4);
	CHECK(!Run.PassCheckpoint(2).bPassed);
}

static void TestCheckpointRunOrderEnforced()
{
	mr::CheckpointRun Run(mr::CheckpointRunConfig{3, 60.f});
	Run.Start();
	CHECK(!Run.PassCheckpoint(1).bPassed);            // skipped ahead
	CHECK(!Run.PassCheckpoint(2).bPassed);
	CHECK(!Run.PassCheckpoint(-1).bPassed);           // unknown
	CHECK(!Run.PassCheckpoint(7).bPassed);
	CHECK(Run.GetNextCheckpoint() == 0);
	CHECK(Run.PassCheckpoint(0).bPassed);
	CHECK(!Run.PassCheckpoint(0).bPassed);            // the same gate twice
	CHECK(Run.GetNextCheckpoint() == 1);
	CHECK_NEAR(Run.GetSplit(1), -1.0, 1e-6);
	CHECK_NEAR(Run.GetSplit(9), -1.0, 1e-6);
}

static void TestCheckpointRunTimeout()
{
	mr::CheckpointRun Run(mr::CheckpointRunConfig{3, 5.f});
	Run.Start();
	Run.PassCheckpoint(0);
	bool bLost = false;
	int LostEvents = 0;
	for (int i = 0; i < 1000; ++i)
	{
		const mr::CheckpointRunEvents Ev = Run.Tick(1.f / 60.f);
		if (Ev.bLost) { bLost = true; ++LostEvents; }
	}
	CHECK(bLost && LostEvents == 1);                  // reported once
	CHECK(Run.GetState() == mr::RunState::Lost);
	CHECK_NEAR(Run.GetTimeLeft(), 0.0, 1e-6);
	CHECK_NEAR(Run.GetElapsed(), 5.0, 1e-6);          // clamped at the limit
	CHECK(!Run.PassCheckpoint(1).bPassed);            // too late
}

static void TestCheckpointRunRestartAndBadConfig()
{
	mr::CheckpointRun Run(mr::CheckpointRunConfig{2, 30.f});
	Run.Start();
	Run.Tick(3.f);
	Run.PassCheckpoint(0);
	CHECK(Run.Start().bStarted);                      // restart clears everything
	CHECK(Run.GetNextCheckpoint() == 0);
	CHECK_NEAR(Run.GetElapsed(), 0.0, 1e-6);
	CHECK_NEAR(Run.GetSplit(0), -1.0, 1e-6);
	Run.Tick(-1.f);                                   // negative time is ignored
	CHECK_NEAR(Run.GetElapsed(), 0.0, 1e-6);
	Run.Reset();
	CHECK(Run.GetState() == mr::RunState::Ready);

	mr::CheckpointRun Empty(mr::CheckpointRunConfig{0, 30.f});
	CHECK(!Empty.Start().bStarted);
	CHECK(Empty.GetState() == mr::RunState::Ready);
	mr::CheckpointRun NoTime(mr::CheckpointRunConfig{3, 0.f});
	CHECK(!NoTime.Start().bStarted);
}

int main()
{
	TestNitrousDrainAndEvents();
	TestNitrousRecharge();
	TestNitrousGating();
	TestNitrousUpgradeKeepsFraction();
	TestNitrousFrameRateIndependent();
	TestSteeringScale();
	TestSteeringSmoothing();
	TestArcadeDrive();
	TestCheckpointRunWinInOrder();
	TestCheckpointRunOrderEnforced();
	TestCheckpointRunTimeout();
	TestCheckpointRunRestartAndBadConfig();
	std::printf("%d checks, %d failures\n", GChecks, GFailures);
	return GFailures == 0 ? 0 : 1;
}
